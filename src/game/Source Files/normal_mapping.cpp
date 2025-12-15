#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/random.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <vector>
#include <algorithm>

// --- Function Prototypes ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
unsigned int loadTexture(const char* path);

struct Particle {
    glm::vec3 Position, Velocity;
    glm::vec4 Color;
    float Life;
    Particle() : Position(0.0f), Velocity(0.0f), Color(1.0f), Life(0.0f) {}
};

// --- Settings ---
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(8.0f, 2.0f, 4.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --- Physics ---
float playerVelocityY = 0.0f;
float gravity = 25.0f;
bool isGrounded = false;
float floorHeight = 0.0f;
float playerHeight = 1.8f;

bool isShooting = false;
float laserLength = 100.0f;
float laserTimer = 0.0f;
float laserDuration = 0.1f;

// --- HUNTER SETTINGS ---
struct Hunter {
    glm::vec3 Position;
    float Speed;
    bool HasLOS;
    float RunCycle; // Bobbing animation

    // Jump Logic
    bool IsJumping;
    float JumpTimer;
    float JumpDuration;
    glm::vec3 JumpDirection; // <--- The "Locked" vector he commits to

    Hunter() : Position(glm::vec3(4.0f, 0.0f, 4.0f)), Speed(9.5f), HasLOS(false), RunCycle(0.0f),
        IsJumping(false), JumpTimer(0.0f), JumpDuration(1.0f), JumpDirection(0.0f) {
    }
};

Hunter hunter;
std::vector<glm::vec3> playerTrail;
float trailTimer = 0.0f;
bool isGameOver = false;

// --- Barrels ---
std::vector<glm::vec3> barrelPositions;
std::vector<bool> barrelVisible;
const float barrelModelScale = 0.04f;
const float barrelRadius = 0.8f;

// --- Particles ---
std::vector<Particle> particles;
unsigned int nr_new_particles = 100;
void SpawnParticles(glm::vec3 position);
unsigned int particleVAO;

const float TILE_SIZE = 4.0f;

// --- MAP LAYOUT ---
std::vector<std::string> levelLayout = {
    "#---------------#",
    "|.......|.......|",
    "|.......|.......|",
    "|.......|.......|",
    "|.BBBBB...BBBBB.|",
    "|.......|.......|",
    "|.......|.......|",
    "#---.-------.---#",
    "|.......|.......|",
    "|.......|.......|",
    "|...B.......B...|",
    "|.......B.......|",
    "#-------D-------#",
    "|...............|",
    "#------------#..|",
    "|------------|..|",
    "#------------#..|",
    "|...B...B...B...|",
    "|..#------------#",
    "|..|------------|",
    "|..#------------#",
    "|...B...B...B...|",
    "#------------#..|",
    "|------------|..|",
    "#------------#..|",
    "|...B...B...B...|",
    "|..#------------#",
    "|...............|",
    "#-------D-------#",
    "|...............|",
    "|.B.B.B.B.B.B.B.|",
    "|...............|",
    "|...#.......#...|",
    "|...#...B...#...|",
    "|...#.......#...|",
    "|.......B.......|",
    "|.B.B.B.B.B.B.B.|",
    "|...............|",
    "|...............|",
    "|...............|",
    "#-------D-------#",
    "|...............|",
    "|BBBBBBBBBBBBBBB|",
    "|...............|",
    "|...............|",
    "|BBBBBBBBBBBBBBB|",
    "|...............|",
    "|...............|",
    "|BBBBBB.B.BBBBBB|",
    "|...............|",
    "#-------D-------#",
    "|...............|",
    "|.#####.|.#####.|",
    "|.#####.|.#####.|",
    "|.B...#.|...B...|",
    "|.B...#.|...B...|",
    "|.#####.|.#####.|",
    "|...............|",
    "#-------D-------#",
    "#-------.-------#",
    "|-------.-------|",
    "|-------B-------|",
    "|-------.-------|",
    "|-------.-------|",
    "|-------B-------|",
    "|-------.-------|",
    "|-------.-------|",
    "|-------B-------|",
    "|-------.-------|",
    "#-------D-------#",
    "|...............|",
    "|.B.B.B.|.B.B.B.|",
    "|.......|.......|",
    "|BBBBBBBBBBBBBBB|",
    "|...............|",
    "|...B.......B...|",
    "|.......|.......|",
    "|BBBBBBBBBBBBBBB|",
    "|...............|",
    "|.B...B...B...B.|",
    "#---------------#"
};

bool CheckLineOfSight(glm::vec3 start, glm::vec3 end, const std::vector<std::string>& map) {
    glm::vec3 dir = glm::normalize(end - start);
    float dist = glm::distance(start, end);
    for (float i = 1.0f; i < dist; i += 2.0f) {
        glm::vec3 checkPos = start + (dir * i);
        int gridX = (int)((checkPos.x + TILE_SIZE / 2) / TILE_SIZE);
        int gridZ = (int)((checkPos.z + TILE_SIZE / 2) / TILE_SIZE);
        if (gridZ >= 0 && gridZ < map.size() && gridX >= 0 && gridX < map[0].size()) {
            char tile = map[gridZ][gridX];
            if (tile == '#' || tile == '-' || tile == '|' || tile == 'D') return false;
        }
    }
    return true;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Shoot or Die", NULL, NULL);
    if (window == NULL) { std::cout << "Failed to create window" << std::endl; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { return -1; }
    glEnable(GL_DEPTH_TEST);

    // --- SHADERS ---
    Shader ourShader(FileSystem::getPath("resources/shaders/4.normal_mapping.vs").c_str(), FileSystem::getPath("resources/shaders/4.normal_mapping.fs").c_str());
    Shader barrelShader(FileSystem::getPath("resources/shaders/4.normal_mapping.vs").c_str(), FileSystem::getPath("resources/shaders/4.normal_mapping.fs").c_str());
    Shader floorShader(FileSystem::getPath("resources/shaders/4.normal_mapping.vs").c_str(), FileSystem::getPath("resources/shaders/4.normal_mapping.fs").c_str());
    Shader laserShader(FileSystem::getPath("resources/shaders/laser.vs").c_str(), FileSystem::getPath("resources/shaders/laser.fs").c_str());
    Shader crosshairShader(FileSystem::getPath("resources/shaders/crosshair.vs").c_str(), FileSystem::getPath("resources/shaders/crosshair.fs").c_str());
    Shader particleShader(FileSystem::getPath("resources/shaders/particle.vs").c_str(), FileSystem::getPath("resources/shaders/particle.fs").c_str());

    // --- LOAD MODELS ---
    Model doorFrameModel(FileSystem::getPath("resources/objects/kit/doorframe.obj"));
    Model barrelModel(FileSystem::getPath("resources/objects/Barrel/Barrels_OBJ.obj"));

    // --- HUNTER MODEL (Static Loading) ---
    Model hunterModel(FileSystem::getPath("resources/objects/hunter/Ch43_nonPBR.dae"));

    // --- TEXTURES ---
    unsigned int floorTexture = loadTexture(FileSystem::getPath("resources/textures/brickwall.jpg").c_str());
    unsigned int wallTexture = loadTexture(FileSystem::getPath("resources/textures/brickwall.jpg").c_str());
    unsigned int doorTexture = loadTexture(FileSystem::getPath("resources/textures/brickwall.jpg").c_str());
    unsigned int barrelTexture = loadTexture(FileSystem::getPath("resources/textures/container.jpg").c_str());

    // --- CUBE VAO ---
    float cubeVertices[] = {
        // ... (Standard Cube Data) ...
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO); glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO); glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    // Laser
    unsigned int laserVAO, laserVBO; glGenVertexArrays(1, &laserVAO); glGenBuffers(1, &laserVBO); glBindVertexArray(laserVAO); glBindBuffer(GL_ARRAY_BUFFER, laserVBO);
    float laserVertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -laserLength }; glBufferData(GL_ARRAY_BUFFER, sizeof(laserVertices), laserVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);

    // Crosshair
    unsigned int crosshairVAO, crosshairVBO; float crosshairVertices[] = { -0.05f, 0.0f, 0.05f, 0.0f, 0.0f, -0.05f, 0.0f, 0.05f };
    glGenVertexArrays(1, &crosshairVAO); glGenBuffers(1, &crosshairVBO); glBindVertexArray(crosshairVAO); glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);

    // Particles
    glGenVertexArrays(1, &particleVAO);

    // --- INIT LOGIC ---
    barrelPositions.clear(); barrelVisible.clear();
    for (int z = 0; z < levelLayout.size(); z++) { for (int x = 0; x < levelLayout[z].size(); x++) { if (levelLayout[z][x] == 'B') { barrelPositions.push_back(glm::vec3(x * TILE_SIZE, 0.0f, z * TILE_SIZE)); barrelVisible.push_back(true); } } }

    glm::vec3 lightDirection = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 ambientLight = glm::vec3(0.5f);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        processInput(window);

        if (!isGameOver) {
            // --- SMART AI LOGIC ---
            hunter.HasLOS = CheckLineOfSight(hunter.Position + glm::vec3(0, 1.5, 0), camera.Position, levelLayout);

            trailTimer += deltaTime;
            if (trailTimer > 0.1f) {
                if (playerTrail.empty() || glm::distance(camera.Position, playerTrail.back()) > 1.0f) {
                    playerTrail.push_back(camera.Position);
                }
                trailTimer = 0.0f;
            }

            // TARGET SELECTION
            glm::vec3 targetPos;
            if (hunter.HasLOS) {
                targetPos = camera.Position;
                hunter.Speed = 12.0f;
                if (playerTrail.size() > 2) playerTrail.erase(playerTrail.begin(), playerTrail.end() - 1);
            }
            else if (!playerTrail.empty()) {
                targetPos = playerTrail.front();
                hunter.Speed = 10.0f;
                float distToCrumb = glm::length(glm::vec2(hunter.Position.x, hunter.Position.z) - glm::vec2(targetPos.x, targetPos.z));
                if (distToCrumb < 1.0f) {
                    playerTrail.erase(playerTrail.begin());
                }
            }
            else {
                targetPos = camera.Position;
            }

            // --- JUMP ATTACK LOGIC (LOCKED TRAJECTORY) ---
            float distToPlayer = glm::distance(hunter.Position, glm::vec3(camera.Position.x, 0.0f, camera.Position.z));

            // TRIGGER: Close range (5.0f)
            if (distToPlayer < 5.0f && !hunter.IsJumping) {
                hunter.IsJumping = true;
                hunter.JumpTimer = 0.0f;

                // [FIX] LOCK THE JUMP DIRECTION:
                // He commits to jumping at your CURRENT location.
                // If you move after he jumps, he will miss.
                hunter.JumpDirection = glm::normalize(camera.Position - hunter.Position);
            }

            // MOVEMENT
            if (!hunter.IsJumping) {
                // RUNNING: Track player actively
                targetPos.y = 0.0f;
                glm::vec3 direction = glm::normalize(targetPos - hunter.Position);
                hunter.Position += direction * hunter.Speed * deltaTime;
                hunter.RunCycle += deltaTime * hunter.Speed * 2.0f;
            }
            else {
                // JUMPING: Use Locked Direction
                hunter.JumpTimer += deltaTime;

                // Fly straight in the locked direction
                hunter.Position += hunter.JumpDirection * (hunter.Speed * 1.5f) * deltaTime;

                if (hunter.JumpTimer >= hunter.JumpDuration) {
                    hunter.IsJumping = false;
                    hunter.JumpTimer = 0.0f;
                    hunter.Position.y = 0.0f;
                }
            }

            if (distToPlayer < 1.0f) {
                isGameOver = true;
                glClearColor(0.8f, 0.0f, 0.0f, 1.0f);
            }

            // Gravity
            playerVelocityY -= gravity * deltaTime;
            camera.Position.y += playerVelocityY * deltaTime;
            if (camera.Position.y < floorHeight + playerHeight) {
                camera.Position.y = floorHeight + playerHeight;
                playerVelocityY = 0.0f;
                isGrounded = true;
            }
        }

        if (!isGameOver) glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 500.0f);
        glm::mat4 view = camera.GetViewMatrix();

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        // --- RENDER MAP ---
        ourShader.use();
        ourShader.setInt("diffuseMap", 0);
        ourShader.setInt("normalMap", 0);
        ourShader.setVec3("light.direction", lightDirection);
        ourShader.setVec3("viewPos", camera.Position);
        ourShader.setVec3("light.ambient", ambientLight);
        ourShader.setVec3("light.diffuse", glm::vec3(0.8f));
        ourShader.setVec3("light.specular", glm::vec3(0.2f));

        floorShader.use(); floorShader.setInt("diffuseMap", 0); floorShader.setInt("normalMap", 0); floorShader.setVec3("light.direction", lightDirection); floorShader.setVec3("viewPos", camera.Position); floorShader.setVec3("light.ambient", ambientLight);
        barrelShader.use(); barrelShader.setVec3("light.direction", lightDirection); barrelShader.setVec3("viewPos", camera.Position); barrelShader.setVec3("light.ambient", ambientLight); barrelShader.setVec3("light.diffuse", glm::vec3(0.8f)); barrelShader.setVec3("light.specular", glm::vec3(0.8f));

        for (int z = 0; z < levelLayout.size(); z++) {
            for (int x = 0; x < levelLayout[z].size(); x++) {
                char tile = levelLayout[z][x];
                glm::vec3 tilePos(x * TILE_SIZE, 0.0f, z * TILE_SIZE);
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, tilePos);

                // Floor
                floorShader.use(); floorShader.setMat4("projection", projection); floorShader.setMat4("view", view);
                glm::mat4 floorModelMat = model; floorModelMat = glm::translate(floorModelMat, glm::vec3(0.0f, -0.1f, 0.0f)); floorModelMat = glm::scale(floorModelMat, glm::vec3(TILE_SIZE, 0.2f, TILE_SIZE));
                floorShader.setMat4("model", floorModelMat); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, floorTexture); glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);

                // Objects
                if (tile == '-' || tile == '|') {
                    ourShader.use(); ourShader.setMat4("projection", projection); ourShader.setMat4("view", view);
                    if (tile == '|') model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f)); model = glm::scale(model, glm::vec3(TILE_SIZE, 4.0f, 0.5f));
                    ourShader.setMat4("model", model); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, wallTexture); glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                else if (tile == '#') {
                    ourShader.use(); ourShader.setMat4("projection", projection); ourShader.setMat4("view", view);
                    model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f)); model = glm::scale(model, glm::vec3(4.1f, 4.0f, 4.1f));
                    ourShader.setMat4("model", model); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, wallTexture); glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                else if (tile == 'D') {
                    ourShader.use(); ourShader.setMat4("projection", projection); ourShader.setMat4("view", view); ourShader.setMat4("model", model);
                    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, doorTexture); doorFrameModel.Draw(ourShader);
                }
                else if (tile == 'B') {
                    bool isVisible = false; for (size_t i = 0; i < barrelPositions.size(); i++) { if (glm::distance(tilePos, barrelPositions[i]) < 0.5f) { isVisible = barrelVisible[i]; break; } }
                    if (isVisible) {
                        barrelShader.use(); barrelShader.setMat4("projection", projection); barrelShader.setMat4("view", view);
                        glm::mat4 barrelMatrix = glm::scale(model, glm::vec3(barrelModelScale)); barrelShader.setMat4("model", barrelMatrix);
                        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, barrelTexture);
                        barrelModel.Draw(barrelShader);
                    }
                }
            }
        }

        if (!isGameOver) {
            // --- RENDER HUNTER (STATIC + PROCEDURAL BOBBING) ---
            ourShader.use();
            ourShader.setMat4("projection", projection);
            ourShader.setMat4("view", view);
            glm::mat4 model = glm::mat4(1.0f);

            // 1. Position
            model = glm::translate(model, hunter.Position);

            // 2. Procedural Animation (The "Wobble")
            if (hunter.IsJumping) {
                // Parabolic Arc
                float jumpProgress = hunter.JumpTimer / hunter.JumpDuration;
                float jumpHeight = sin(jumpProgress * 3.14159f) * 2.5f;
                model = glm::translate(model, glm::vec3(0.0f, jumpHeight, 0.0f));
            }
            else {
                // Run bob
                float bobHeight = sin(hunter.RunCycle) * 0.1f;
                model = glm::translate(model, glm::vec3(0.0f, bobHeight, 0.0f));
                float rockAngle = cos(hunter.RunCycle) * 0.1f;
                model = glm::rotate(model, rockAngle, glm::vec3(0.0f, 0.0f, 1.0f));
            }

            // 3. Face Direction (LOCKED if jumping)
            glm::vec3 faceDir;
            if (hunter.IsJumping) {
                faceDir = hunter.JumpDirection; // Look where he is jumping
            }
            else {
                faceDir = glm::normalize(camera.Position - hunter.Position); // Look at player
            }

            if (glm::length(faceDir) > 0.01f) {
                float angle = atan2(faceDir.x, faceDir.z);
                model = glm::rotate(model, angle, glm::vec3(0, 1, 0));
            }

            // Scale & Flip (Mixamo fix)
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(0.01f));

            ourShader.setMat4("model", model);
            hunterModel.Draw(ourShader);
        }

        // ... [Particles & Shooting] ...
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (auto& p : particles) { p.Life -= deltaTime; if (p.Life > 0.0f) { p.Position += p.Velocity * deltaTime; p.Velocity.y -= 5.0f * deltaTime; p.Color.a = p.Life * 2.0f; } }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.Life <= 0.0f; }), particles.end());
        if (!particles.empty()) {
            glDepthMask(GL_FALSE); particleShader.use(); particleShader.setMat4("projection", projection); particleShader.setMat4("view", view); glBindVertexArray(particleVAO);
            for (const auto& particle : particles) { particleShader.setVec3("offset", particle.Position); particleShader.setVec4("color", particle.Color); glDrawArrays(GL_POINTS, 0, 1); }
            glBindVertexArray(0); glDepthMask(GL_TRUE);
        }

        if (isShooting && !isGameOver) {
            laserTimer += deltaTime;
            if (laserTimer >= laserDuration) isShooting = false;
            else {
                laserShader.use(); laserShader.setMat4("projection", projection); laserShader.setMat4("view", view);
                glm::mat4 model = glm::mat4(1.0f); glm::vec3 laserOrigin = camera.Position + (camera.Front * 0.5f) - (camera.Up * 0.2f);
                model = glm::translate(model, laserOrigin); model = model * glm::mat4_cast(glm::rotation(glm::vec3(0.0f, 0.0f, -1.0f), camera.Front));
                laserShader.setMat4("model", model); glLineWidth(5.0f); glBindVertexArray(laserVAO); glDrawArrays(GL_LINES, 0, 2); glBindVertexArray(0); glLineWidth(1.0f);
            }
        }
        glDisable(GL_DEPTH_TEST); crosshairShader.use(); glBindVertexArray(crosshairVAO); glDrawArrays(GL_LINES, 0, 4); glBindVertexArray(0); glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

// ... [Input Callbacks] ...
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (isGameOver) { if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) { camera.Position = glm::vec3(8.0f, 2.0f, 4.0f); hunter.Position = glm::vec3(4.0f, 0.0f, 4.0f); playerTrail.clear(); isGameOver = false; } return; }
    glm::vec3 front = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
    glm::vec3 right = glm::normalize(glm::vec3(camera.Right.x, 0.0f, camera.Right.z));
    float velocity = 15.0f * deltaTime; if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) velocity *= 2.0f;
    glm::vec3 nextPos = camera.Position;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) nextPos += front * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) nextPos -= front * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) nextPos -= right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) nextPos += right * velocity;
    int gridX = (int)((nextPos.x + TILE_SIZE / 2) / TILE_SIZE); int gridZ = (int)((nextPos.z + TILE_SIZE / 2) / TILE_SIZE);
    if (gridZ >= 0 && gridZ < levelLayout.size() && gridX >= 0 && gridX < levelLayout[0].size()) { char tile = levelLayout[gridZ][gridX]; if (tile == '#' || tile == '-' || tile == '|') nextPos = camera.Position; }
    else { nextPos = camera.Position; }
    for (size_t i = 0; i < barrelPositions.size(); ++i) { if (barrelVisible[i]) { float dist = glm::distance(glm::vec2(nextPos.x, nextPos.z), glm::vec2(barrelPositions[i].x, barrelPositions[i].z)); if (dist < barrelRadius + 0.5f) { nextPos = camera.Position; break; } } }
    camera.Position.x = nextPos.x; camera.Position.z = nextPos.z;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded) { playerVelocityY = 8.0f; isGrounded = false; }
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) { if (isGameOver) return; if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !isShooting) { isShooting = true; laserTimer = 0.0f; glm::vec3 rayOrigin = camera.Position; glm::vec3 rayDir = glm::normalize(camera.Front); for (size_t i = 0; i < barrelPositions.size(); ++i) { if (barrelVisible[i]) { glm::vec3 barrelPos = barrelPositions[i]; glm::vec2 barrelXZ = glm::vec2(barrelPos.x, barrelPos.z); glm::vec2 rayOriginXZ = glm::vec2(rayOrigin.x, rayOrigin.z); glm::vec2 rayDirXZ = glm::normalize(glm::vec2(rayDir.x, rayDir.z)); glm::vec2 toBarrel = barrelXZ - rayOriginXZ; float t = glm::dot(toBarrel, rayDirXZ); if (t > 0.0f) { glm::vec2 closestPointXZ = rayOriginXZ + (rayDirXZ * t); float distXZ = glm::distance(closestPointXZ, barrelXZ); if (distXZ < barrelRadius) { float t3D = glm::dot(barrelPos - rayOrigin, rayDir); glm::vec3 hitPoint3D = rayOrigin + (rayDir * t3D); if (hitPoint3D.y >= 0.0f && hitPoint3D.y <= 3.0f) { std::cout << "HIT Barrel " << i << "!" << std::endl; barrelVisible[i] = false; SpawnParticles(barrelPos); break; } } } } } } }
void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) { if (isGameOver) return; float xpos = static_cast<float>(xposIn); float ypos = static_cast<float>(yposIn); if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; } float xoffset = xpos - lastX; float yoffset = lastY - ypos; lastX = xpos; lastY = ypos; camera.ProcessMouseMovement(xoffset, yoffset); }
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) { camera.ProcessMouseScroll(static_cast<float>(yoffset)); }
unsigned int loadTexture(char const* path) { unsigned int textureID; glGenTextures(1, &textureID); int width, height, nrComponents; unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0); if (data) { GLenum format; if (nrComponents == 1) format = GL_RED; else if (nrComponents == 3) format = GL_RGB; else if (nrComponents == 4) format = GL_RGBA; glBindTexture(GL_TEXTURE_2D, textureID); glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data); glGenerateMipmap(GL_TEXTURE_2D); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); stbi_image_free(data); } else { std::cout << "Texture failed to load at path: " << path << std::endl; stbi_image_free(data); } return textureID; }
void SpawnParticles(glm::vec3 position) { for (unsigned int i = 0; i < nr_new_particles; ++i) { Particle p; p.Position = position + glm::vec3(0.0f, barrelRadius, 0.0f); p.Velocity = glm::ballRand(5.0f); p.Color = glm::vec4(1.0f, glm::linearRand(0.2f, 0.6f), 0.0f, 1.0f); p.Life = glm::linearRand(0.5f, 1.5f); particles.push_back(p); } }
