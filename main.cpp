#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/random.hpp>

 // --- Game Engine Headers (LearnOpenGL) ---
#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <learnopengl/animator.h>
#include <learnopengl/animation.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream> // For time string formatting
#include <iomanip> // For time precision

// ==========================================================================================
// GLOBAL VARIABLES
// ==========================================================================================

// --- Animation Controls ---
// Pointers to handle gun animation state from input callbacks
Animator* globalGunAnimator = nullptr;
Animation* globalGunFireAnim = nullptr;

// --- Window & Graphics Settings ---
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const float TILE_SIZE = 4.0f; // Size of each map block (4x4 meters)

// --- Camera & Input ---
Camera camera(glm::vec3(8.0f, 2.0f, 4.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// --- Timing ---
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float gameTime = 0.0f; // Tracks how long the current run has lasted

// --- Physics ---
float playerVelocityY = 0.0f;
float gravity = 40.0f; // Snappy gravity for arcade feel
bool isGrounded = false;
float floorHeight = 0.0f;
float playerHeight = 1.8f;

// --- Gameplay State ---
float playerHealth = 1.0f;
bool isGameOver = false;
bool isGameWon = false;

// --- Weapon State (Recoil System) ---
bool isShooting = false;
float laserLength = 100.0f;
float laserTimer = 0.0f;
float laserDuration = 0.05f;

// Dynamic Recoil Variables (Lerped back to 0 over time)
float recoilTimer = 0.0f;
float currentRecoilZ = 0.0f; // Backward kick
float currentRecoilX = 0.0f; // Upward muzzle climb

// --- Map Objects ---
int totalBarrels = 0;
int destroyedBarrels = 0;

// ==========================================================================================
// DATA STRUCTURES
// ==========================================================================================

struct Particle {
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec4 Color;
    float Life;
    Particle() : Position(0.0f), Velocity(0.0f), Color(1.0f), Life(0.0f) {}
};

struct Hunter {
    glm::vec3 Position;
    float BaseSpeed;
    float CurrentSpeed;
    bool HasLOS; // Line of Sight status

    // Jump / AI Logic
    bool IsJumping;
    float JumpTimer;
    float JumpDuration;
    glm::vec3 JumpDirection;
    float CurrentJumpSpeed;

    Hunter()
        : Position(glm::vec3(4.0f, 0.0f, 4.0f)),
        BaseSpeed(50.0f),    // Very Fast
        CurrentSpeed(50.0f),
        HasLOS(false),
        IsJumping(false),
        JumpTimer(0.0f),
        JumpDuration(0.6f),
        JumpDirection(0.0f),
        CurrentJumpSpeed(0.0f)
    {
    }
};

// ==========================================================================================
// ENTITIES & MAP
// ==========================================================================================

Hunter hunter;
std::vector<glm::vec3> playerTrail; // Breadcrumbs for AI pathfinding
float trailTimer = 0.0f;

std::vector<glm::vec3> barrelPositions;
std::vector<bool> barrelVisible;
const float barrelModelScale = 0.04f;
const float barrelRadius = 0.8f; // Hitbox size

std::vector<Particle> particles;
unsigned int nr_new_particles = 100;
unsigned int particleVAO, particleVBO;

// --- Level Design (6-Layer Gauntlet) ---
// '#' = Wall, '.' = Floor, 'B' = Destructible Barrel
std::vector<std::string> levelLayout = {
    "#####################################################################################",
    "#...................................................................................#",
    "#.......BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB.......#",
    "#...................................................................................#",
    "##################################################################################..#",
    "##################################################################################..#",
    "#...BBBBBBBB...................######...............................................#",
    "#...........###BBBB...................BBBBBBB###BBBBBBBB............##BBBBBBBB......#",
    "#.....###..............###BBBBBBBB.............................#####................#",
    "#.................................###BBBBBBBB............######.....................#",
    "#..##################################################################################",
    "#..##################################################################################",
    "#.......#...........B...........B...........#...........#...........B...........B...#",
    "#.......#...........#...........#...........#...........#...........#...........#...#",
    "#.......#...BBBBB...#...BBBBB...#...BBBBB...#...BBBBB...#...BBBBB...#...BBBBB...#...#",
    "#.......#...........#...........#...........#...........#...........#...........#...#",
    "#.......B...........#...........#...........B...........B...........#...........#...#",
    "##################################################################################..#",
    "##################################################################################..#",
    "#...................................................................................#",
    "#######...#######...#######...#######...#######...#######...#######...#######...#####",
    "#.....#BBB#.....#.B.#.....#.B.#.....BBB.#.....#.B.#.....#.B.#.....#.B.#.....#.B.#...#",
    "#.....#####.....#####.....#####.....#####.....#####.....#####.....#####.....#####...#",
    "#...................................#################################################",
    "#..##################################################################################",
    "#...#.....#.....#.....#.....#.....#.....#.....#.....#.....#.....#.....B.....B.......#",
    "#....#...#.B...B.#...#.B...#.#...#.B...B.B...B.#...#.#...#.#...#.B...#.#...#.#......#",
    "#.....#.#...#.#...#.#...#.B...#.#...#.#...#.#...#.#...#.#...#.#...#.#...#.#...#.....#",
    "#......B.....#.....B.....#.....B.....#.....#.....B.....B.....B.....#.....#.....#....#",
    "###################################################################################B#",
    "###################################################################################B#",
    "#.................................................................................#B#",
    "#.................................................................................#B#",
    "#.................................................................................#B#",
    "#.................................................................................#B#",
    "#.................................................................................#B#",
    "#.................................................................................#B#",
    "#.................................................................................#B#",
    "#.................................................................................#B#",
    "#................................................................................####",
};

// ==========================================================================================
// FUNCTION PROTOTYPES
// ==========================================================================================
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
bool CheckLineOfSight(glm::vec3 start, glm::vec3 end, const std::vector<std::string>& map);
void SpawnParticles(glm::vec3 position);
void UpdateParticles(float dt);
void RenderCrosshair(Shader& shader, unsigned int vao);

// ==========================================================================================
// MAIN FUNCTION
// ==========================================================================================
int main()
{
    // --- 1. Init GLFW & Window ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Shoot or Die", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // --- 2. Init GLAD ---
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- 3. Compile Shaders ---
    Shader ourShader("shaders/static_model.vs", "shaders/static_model.fs");
    Shader floorShader("shaders/static_model.vs", "shaders/static_model.fs");
    Shader skinningShader("shaders/skinning.vs", "shaders/skinning.fs");
    Shader particleShader("shaders/particle.vs", "shaders/particle.fs");
    Shader laserShader("shaders/laser.vs", "shaders/laser.fs");
    Shader crosshairShader("shaders/crosshair.vs", "shaders/crosshair.fs");

    stbi_set_flip_vertically_on_load(true);

    // --- 4. Load Models & Animations ---
    Model doorFrameModel("objects/kit/doorframe.obj");
    Model barrelModel("objects/Barrel/Barrels_OBJ.obj");

    // Gun Model (Collada .dae for animation support)
    Model gunModel("objects/airgun/Air_Gun-COLLADA_2.dae");

    // Hunter Animation
    Model hunterModel("objects/hunter/Ch43_nonPBR.dae");
    Animation runAnim("objects/hunter/Run Forward.dae", &hunterModel);
    Animation jumpAnim("objects/hunter/Jump.dae", &hunterModel);
    Animator animator(&runAnim);

    // Gun Animation
    Animation gunIdleAnim("objects/airgun/Air_Gun-COLLADA_2.dae", &gunModel);
    Animator gunAnimator(&gunIdleAnim);

    // Link global pointers
    globalGunAnimator = &gunAnimator;
    globalGunFireAnim = &gunIdleAnim;

    // --- 5. Load Textures ---
    unsigned int floorTexture = loadTexture("textures/brickwall.jpg");
    unsigned int wallTexture = loadTexture("textures/brickwall.jpg");
    unsigned int doorTexture = loadTexture("textures/brickwall.jpg");
    unsigned int barrelTexture = loadTexture("objects/Barrel/Barrels_MainBody_BaseColor.png");
    unsigned int gunTexture = loadTexture("objects/airgun/Air_Gun_Default_color.png.002.jpg");

    // --- 6. Setup Vertex Data (Cube, Laser, Crosshair, Particles) ---
    // (Cube Vertices for Map Building)
    float cubeVertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f, 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f, 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f, -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f, -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f, 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f, 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f, -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f, -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f, -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f, -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f, -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f, -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f, 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f, 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f, 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f, 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f, 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f, 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f, -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f, -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f, 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f, -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO); glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO); glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    // Laser Lines
    unsigned int laserVAO, laserVBO;
    glGenVertexArrays(1, &laserVAO); glGenBuffers(1, &laserVBO);
    glBindVertexArray(laserVAO); glBindBuffer(GL_ARRAY_BUFFER, laserVBO);
    float laserVertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -laserLength };
    glBufferData(GL_ARRAY_BUFFER, sizeof(laserVertices), laserVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // UI: Crosshair
    unsigned int crosshairVAO, crosshairVBO;
    float crosshairVertices[] = { -0.05f, 0.0f, 0.05f, 0.0f, 0.0f, -0.05f, 0.0f, 0.05f };
    glGenVertexArrays(1, &crosshairVAO); glGenBuffers(1, &crosshairVBO);
    glBindVertexArray(crosshairVAO); glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // VFX: Particles
    glGenVertexArrays(1, &particleVAO); glGenBuffers(1, &particleVBO);
    glBindVertexArray(particleVAO); glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    float particleQuad[] = { 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f };
    glBufferData(GL_ARRAY_BUFFER, sizeof(particleQuad), particleQuad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // --- 7. Initialize Game Entities ---
    barrelPositions.clear(); barrelVisible.clear(); totalBarrels = 0;
    for (int z = 0; z < levelLayout.size(); z++) {
        for (int x = 0; x < levelLayout[z].size(); x++) {
            if (levelLayout[z][x] == 'B') {
                barrelPositions.push_back(glm::vec3(x * TILE_SIZE, 0.0f, z * TILE_SIZE));
                barrelVisible.push_back(true);
                totalBarrels++;
            }
        }
    }

    glm::vec3 lightDirection = glm::vec3(0.5f, -0.2f, -1.0f);
    glm::vec3 ambientLight = glm::vec3(0.5f);

    // ==========================================================================================
    // GAME LOOP
    // ==========================================================================================
    while (!glfwWindowShouldClose(window))
    {
        // 1. Time Logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // 2. Input & Physics Update
        processInput(window);
        animator.UpdateAnimation(deltaTime);
        gunAnimator.UpdateAnimation(deltaTime);

        // 3. Recoil Physics (Spring Back)
        if (recoilTimer > 0.0f) {
            recoilTimer -= deltaTime * 5.0f;
            if (recoilTimer < 0.0f) recoilTimer = 0.0f;
            currentRecoilZ = glm::mix(currentRecoilZ, 0.0f, deltaTime * 10.0f);
            currentRecoilX = glm::mix(currentRecoilX, 0.0f, deltaTime * 10.0f);
        }
        else {
            currentRecoilZ = 0.0f; currentRecoilX = 0.0f;
        }

        if (isShooting) {
            laserTimer += deltaTime;
            if (laserTimer >= laserDuration) { isShooting = false; laserTimer = 0.0f; }
        }
        UpdateParticles(deltaTime);

        // 4. Game Logic (Win/Loss)
        if (!isGameOver && !isGameWon) {
            gameTime += deltaTime;

            // Hunter AI
            hunter.CurrentSpeed = hunter.BaseSpeed + (gameTime / 10.0f);
            if (hunter.CurrentSpeed > 25.0f) hunter.CurrentSpeed = 25.0f;
            hunter.HasLOS = CheckLineOfSight(hunter.Position + glm::vec3(0, 1.5, 0), camera.Position, levelLayout);

            // Hunter Pathfinding (Breadcrumbs)
            trailTimer += deltaTime;
            if (trailTimer > 0.1f) {
                if (playerTrail.empty() || glm::distance(camera.Position, playerTrail.back()) > 1.0f) {
                    playerTrail.push_back(camera.Position);
                }
                trailTimer = 0.0f;
            }

            // Hunter Movement
            glm::vec3 targetPos = camera.Position;
            if (hunter.HasLOS) {
                targetPos = camera.Position;
                hunter.CurrentSpeed += 3.0f; // Rage mode on sight
                if (playerTrail.size() > 2) playerTrail.erase(playerTrail.begin(), playerTrail.end() - 1);
            }
            else if (!playerTrail.empty()) {
                targetPos = playerTrail.front();
                if (glm::length(glm::vec2(hunter.Position.x - targetPos.x, hunter.Position.z - targetPos.z)) < 1.0f) {
                    playerTrail.erase(playerTrail.begin());
                }
            }

            // Jump Mechanic
            float distToPlayer = glm::distance(hunter.Position, glm::vec3(camera.Position.x, 0.0f, camera.Position.z));
            if (distToPlayer < 6.0f && distToPlayer > 2.0f && !hunter.IsJumping) {
                hunter.IsJumping = true;
                hunter.JumpTimer = 0.0f;
                hunter.JumpDirection = glm::normalize(camera.Position - hunter.Position);
                hunter.CurrentJumpSpeed = (distToPlayer + 1.0f) / hunter.JumpDuration;
                animator.PlayAnimation(&jumpAnim);
            }

            if (!hunter.IsJumping) {
                targetPos.y = 0.0f;
                glm::vec3 direction = glm::normalize(targetPos - hunter.Position);
                hunter.Position += direction * hunter.CurrentSpeed * deltaTime;
            }
            else {
                hunter.JumpTimer += deltaTime;
                hunter.Position += hunter.JumpDirection * hunter.CurrentJumpSpeed * deltaTime;
                float progress = hunter.JumpTimer / hunter.JumpDuration;
                hunter.Position.y = 4.0f * 1.5f * progress * (1.0f - progress); // Parabolic Arc
                if (hunter.JumpTimer >= hunter.JumpDuration) {
                    hunter.IsJumping = false; hunter.JumpTimer = 0.0f; hunter.Position.y = 0.0f;
                    animator.PlayAnimation(&runAnim);
                }
            }

            // Player Gravity
            playerVelocityY -= gravity * deltaTime;
            camera.Position.y += playerVelocityY * deltaTime;
            if (camera.Position.y < floorHeight + playerHeight) {
                camera.Position.y = floorHeight + playerHeight;
                playerVelocityY = 0.0f;
                isGrounded = true;
            }

            // Lose Condition
            if (distToPlayer < 1.0f) {
                playerHealth -= 100.0f;
                isGameOver = true;
                std::cout << "GAME OVER" << std::endl;
            }

            // Win Condition
            if (camera.Position.z >= 150.0f) {
                isGameWon = true;
                std::cout << "VICTORY! Time: " << gameTime << "s" << std::endl;
            }
        }

        // ======================================================================================
        // RENDER PIPELINE
        // ======================================================================================

        // 1. Clear Screen (Background Color Logic)
        if (isGameOver) glClearColor(0.5f, 0.0f, 0.0f, 1.0f); // Red (Death)
        else if (isGameWon) glClearColor(0.8f, 0.6f, 0.0f, 1.0f); // Gold (Win)
        else glClearColor(0.94f, 0.85f, 0.65f, 1.0f); // Sand Beige (Desert)

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(100.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 500.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // 2. Render Walls
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setVec3("light.direction", camera.Front);
        ourShader.setVec3("viewPos", camera.Position);
        ourShader.setVec3("light.ambient", ambientLight);
        ourShader.setVec3("light.diffuse", glm::vec3(0.8f));
        ourShader.setVec3("light.specular", glm::vec3(0.2f));

        for (int z = 0; z < levelLayout.size(); z++) {
            for (int x = 0; x < levelLayout[z].size(); x++) {
                char tile = levelLayout[z][x];
                glm::vec3 tilePos(x * TILE_SIZE, 0.0f, z * TILE_SIZE);
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, tilePos);

                if (tile == '#' || tile == '-' || tile == '|') {
                    if (tile == '|') model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
                    model = glm::translate(model, glm::vec3(0, 6.0f, 0));
                    model = glm::scale(model, glm::vec3(TILE_SIZE, 12.0f, 0.5f)); // Tall Walls
                    if (tile == '#') model = glm::scale(model, glm::vec3(1.0f, 1.0f, 8.0f));
                    ourShader.setMat4("model", model);
                    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, wallTexture);
                    glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }

        // 3. Render Floor
        floorShader.use();
        floorShader.setMat4("projection", projection);
        floorShader.setMat4("view", view);
        floorShader.setVec3("light.direction", camera.Front);
        floorShader.setVec3("viewPos", camera.Position);
        floorShader.setVec3("light.ambient", ambientLight);

        for (int z = 0; z < levelLayout.size(); z++) {
            for (int x = 0; x < levelLayout[z].size(); x++) {
                glm::vec3 tilePos(x * TILE_SIZE, 0.0f, z * TILE_SIZE);
                glm::mat4 fmodel = glm::mat4(1.0f);
                fmodel = glm::translate(fmodel, tilePos);
                fmodel = glm::translate(fmodel, glm::vec3(0, -0.1f, 0));
                fmodel = glm::scale(fmodel, glm::vec3(TILE_SIZE, 0.2f, TILE_SIZE));
                floorShader.setMat4("model", fmodel);
                glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, floorTexture);
                glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // 4. Render Barrels
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setVec3("viewPos", camera.Position);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, barrelTexture);

        for (size_t i = 0; i < barrelPositions.size(); ++i) {
            if (barrelVisible[i]) {
                glm::mat4 bModel = glm::mat4(1.0f);
                bModel = glm::translate(bModel, barrelPositions[i]);
                bModel = glm::scale(bModel, glm::vec3(barrelModelScale));
                ourShader.setMat4("model", bModel);
                barrelModel.Draw(ourShader);
            }
        }

        // 5. Render Gun (First Person View)
        // Clear Depth buffer to ensure gun is drawn ON TOP of walls (Prevents clipping)
        glClear(GL_DEPTH_BUFFER_BIT);

        skinningShader.use();
        skinningShader.setMat4("projection", projection);
        skinningShader.setMat4("view", view);
        skinningShader.setVec3("light.direction", lightDirection);
        skinningShader.setVec3("viewPos", camera.Position);
        skinningShader.setVec3("light.ambient", ambientLight);
        skinningShader.setVec3("light.diffuse", glm::vec3(0.8f));

        auto gunTransforms = gunAnimator.GetFinalBoneMatrices();
        for (int i = 0; i < gunTransforms.size(); ++i) {
            skinningShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", gunTransforms[i]);
        }
        glm::mat4 identityMatrix = glm::mat4(1.0f);
        for (int i = gunTransforms.size(); i < 200; ++i) {
            skinningShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", identityMatrix);
        }

        glm::mat4 gunMatrix = glm::mat4(1.0f);
        gunMatrix = glm::translate(gunMatrix, camera.Position);
        gunMatrix = glm::rotate(gunMatrix, glm::radians(-camera.Yaw - 90.0f), glm::vec3(0, 1, 0));
        gunMatrix = glm::rotate(gunMatrix, glm::radians(camera.Pitch), glm::vec3(1, 0, 0));

        // -- Gun Position & Recoil Logic --
        // Move Gun: Right (0.2), Down (-0.4), Forward (-Z) + RecoilZ
        gunMatrix = glm::translate(gunMatrix, glm::vec3(0.2f, -0.4f, 0.0f + currentRecoilZ));
        // Apply Muzzle Climb (Recoil X)
        gunMatrix = glm::rotate(gunMatrix, glm::radians(currentRecoilX * 10.0f), glm::vec3(1, 0, 0));
        // Scale Gun
        gunMatrix = glm::scale(gunMatrix, glm::vec3(3.0f));
        // Orientation Fix (Face Left)
        gunMatrix = glm::rotate(gunMatrix, glm::radians(270.0f), glm::vec3(0, 1, 0));

        skinningShader.setMat4("model", gunMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gunTexture);
        gunModel.Draw(skinningShader);

        glEnable(GL_DEPTH_TEST); // Re-enable depth for the rest of the scene

        // 6. Render Hunter
        if (!isGameOver) {
            auto transforms = animator.GetFinalBoneMatrices();
            for (int i = 0; i < transforms.size(); ++i) skinningShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
            for (int i = transforms.size(); i < 200; ++i) skinningShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", identityMatrix);

            glm::mat4 hModel = glm::mat4(1.0f);
            hModel = glm::translate(hModel, hunter.Position);
            glm::vec3 faceDir;
            if (hunter.IsJumping) faceDir = hunter.JumpDirection; else faceDir = glm::normalize(camera.Position - hunter.Position);

            if (glm::length(faceDir) > 0.01f) {
                float angle = atan2(faceDir.x, faceDir.z);
                hModel = glm::rotate(hModel, angle, glm::vec3(0, 1, 0));
            }
            hModel = glm::scale(hModel, glm::vec3(2.5f));
            skinningShader.setMat4("model", hModel);
            hunterModel.Draw(skinningShader);
        }

        // 7. Render Laser
        if (isShooting) {
            laserShader.use();
            laserShader.setMat4("projection", projection);
            laserShader.setMat4("view", view);
            glm::mat4 lModel = glm::mat4(1.0f);
            glm::vec3 laserStart = camera.Position + (camera.Front * 0.5f) + (camera.Right * 0.2f) + (camera.Up * -0.2f);
            lModel = glm::translate(lModel, laserStart);
            glm::vec3 direction = glm::normalize(camera.Front);
            glm::quat rot = glm::rotation(glm::vec3(0, 0, -1), direction);
            lModel = lModel * glm::toMat4(rot);
            laserShader.setMat4("model", lModel);
            glBindVertexArray(laserVAO); glDrawArrays(GL_LINES, 0, 2);
        }

        // 8. Render Particles
        glEnable(GL_BLEND);
        particleShader.use();
        particleShader.setMat4("projection", projection);
        particleShader.setMat4("view", view);
        glBindVertexArray(particleVAO);
        for (const auto& p : particles) {
            if (p.Life > 0.0f) {
                glm::mat4 pModel = glm::mat4(1.0f);
                pModel = glm::translate(pModel, p.Position);
                pModel[0][0] = view[0][0]; pModel[0][1] = view[1][0]; pModel[0][2] = view[2][0];
                pModel[1][0] = view[0][1]; pModel[1][1] = view[1][1]; pModel[1][2] = view[2][1];
                pModel[2][0] = view[0][2]; pModel[2][1] = view[1][2]; pModel[2][2] = view[2][2];
                pModel = glm::scale(pModel, glm::vec3(0.2f));
                particleShader.setMat4("model", pModel);
                particleShader.setVec4("color", p.Color);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        // 9. Render Crosshair
        RenderCrosshair(crosshairShader, crosshairVAO);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// ==========================================================================================
// HELPER FUNCTIONS IMPLEMENTATION
// ==========================================================================================

void RenderCrosshair(Shader& shader, unsigned int vao) {
    glDisable(GL_DEPTH_TEST);
    shader.use();
    glBindVertexArray(vao);
    glm::mat4 uiModel = glm::mat4(1.0f);
    shader.setMat4("model", uiModel);
    shader.setVec4("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    glDrawArrays(GL_LINES, 0, 4);
    glEnable(GL_DEPTH_TEST);
}

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

void SpawnParticles(glm::vec3 position) {
    for (unsigned int i = 0; i < nr_new_particles; ++i) {
        Particle p;
        p.Position = position + glm::ballRand(0.5f);
        p.Color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
        p.Life = 1.0f;
        p.Velocity = glm::ballRand(2.0f); p.Velocity.y = std::abs(p.Velocity.y) + 1.0f;
        particles.push_back(p);
    }
}

void UpdateParticles(float dt) {
    for (auto& p : particles) {
        p.Life -= dt;
        if (p.Life > 0.0f) { p.Position += p.Velocity * dt; p.Color.a -= dt * 2.0f; }
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.Life <= 0.0f; }), particles.end());
}

// --- INPUT & LOGIC HANDLER ---
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // --- GAME OVER / WIN STATE ---
    if (isGameOver || isGameWon) {

        if (isGameOver) {
            glfwSetWindowTitle(window, "YOU DIED! Press 'R' to Restart");
        }
        else {
            // --- DISPLAY WIN TIME ---
            std::stringstream ss;
            ss << "VICTORY! Time: " << std::fixed << std::setprecision(2) << gameTime << "s | Press 'R'";
            glfwSetWindowTitle(window, ss.str().c_str());
        }

        // --- RESTART LOGIC ---
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            // Reset Camera (Start of Map)
            camera.Position = glm::vec3(10.0f, 2.0f, 2.0f);

            // Reset Hunter
            hunter.Position = glm::vec3(4.0f, 0.0f, 2.0f);
            hunter.IsJumping = false;
            hunter.CurrentSpeed = hunter.BaseSpeed;

            // Reset Stats
            gameTime = 0.0f;
            playerHealth = 1.0f;
            playerTrail.clear();

            // Reset Map Objects
            barrelVisible.assign(barrelVisible.size(), true);
            destroyedBarrels = 0;

            // Reset Weapon
            isShooting = false;
            recoilTimer = 0.0f;
            currentRecoilZ = 0.0f;
            currentRecoilX = 0.0f;

            // Reset State Flags
            isGameOver = false;
            isGameWon = false;

            // Reset Title Bar
            glfwSetWindowTitle(window, "Shoot or Die: AIRGUN RECOIL");
            std::cout << "Game Restarted!" << std::endl;
        }
        return; // Halt movement input if dead/won
    }

    // --- PLAYER MOVEMENT ---
    glm::vec3 front = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
    glm::vec3 right = glm::normalize(glm::vec3(camera.Right.x, 0.0f, camera.Right.z));
    float velocity = 20.0f * deltaTime; // Standard Speed

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        velocity *= 1.5f; // Sprint Speed
    }

    glm::vec3 nextPos = camera.Position;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) nextPos += front * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) nextPos -= front * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) nextPos -= right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) nextPos += right * velocity;

    // 1. Wall Collision
    int gridX = (int)((nextPos.x + TILE_SIZE / 2) / TILE_SIZE);
    int gridZ = (int)((nextPos.z + TILE_SIZE / 2) / TILE_SIZE);
    if (gridZ >= 0 && gridZ < levelLayout.size() && gridX >= 0 && gridX < levelLayout[0].size()) {
        char tile = levelLayout[gridZ][gridX];
        if (tile == '#' || tile == '-' || tile == '|') nextPos = camera.Position;
    }
    else { nextPos = camera.Position; }

    // 2. Barrel Collision (Obstacle)
    for (size_t i = 0; i < barrelPositions.size(); ++i) {
        if (barrelVisible[i]) {
            float dist = glm::distance(glm::vec2(nextPos.x, nextPos.z), glm::vec2(barrelPositions[i].x, barrelPositions[i].z));
            if (dist < 1.0f) {
                nextPos = camera.Position; // Block movement
                break;
            }
        }
    }

    camera.Position.x = nextPos.x;
    camera.Position.z = nextPos.z;

    // Jump Logic
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded) {
        playerVelocityY = 15.0f;
        isGrounded = false;
    }
}

// --- STANDARD CALLBACKS ---
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (isGameOver || isGameWon) return;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (!isShooting) {
            isShooting = true;
            laserTimer = 0.0f;

            // Trigger Recoil Animation
            recoilTimer = 0.2f;
            currentRecoilZ = 0.3f;
            currentRecoilX = 4.0f;

            // Play Gun Animation
            if (globalGunAnimator && globalGunFireAnim) {
                globalGunAnimator->PlayAnimation(globalGunFireAnim);
            }

            // Raycasting for Shooting
            glm::vec3 rayOrigin = camera.Position;
            glm::vec3 rayDir = glm::normalize(camera.Front);
            for (size_t i = 0; i < barrelPositions.size(); ++i) {
                if (barrelVisible[i]) {
                    glm::vec3 barrelPos = barrelPositions[i];
                    // Simple Hit Detection
                    float t = glm::dot(glm::vec2(barrelPos.x, barrelPos.z) - glm::vec2(rayOrigin.x, rayOrigin.z), glm::normalize(glm::vec2(rayDir.x, rayDir.z)));
                    if (t > 0.0f) {
                        glm::vec2 closest = glm::vec2(rayOrigin.x, rayOrigin.z) + (glm::normalize(glm::vec2(rayDir.x, rayDir.z)) * t);
                        if (glm::distance(closest, glm::vec2(barrelPos.x, barrelPos.z)) < barrelRadius) {
                            // Check Height
                            float t3D = glm::dot(barrelPos - rayOrigin, rayDir);
                            glm::vec3 hitPoint = rayOrigin + (rayDir * t3D);
                            if (hitPoint.y >= 0.0f && hitPoint.y <= 3.0f) {
                                barrelVisible[i] = false;
                                destroyedBarrels++;
                                SpawnParticles(barrelPos + glm::vec3(0, 1.0f, 0));
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (isGameOver || isGameWon) return;
    float xpos = static_cast<float>(xposIn); float ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX; float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) { camera.ProcessMouseScroll(static_cast<float>(yoffset)); }

unsigned int loadTexture(char const* path) {
    unsigned int textureID; glGenTextures(1, &textureID);
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = (nrComponents == 1) ? GL_RED : (nrComponents == 3 ? GL_RGB : GL_RGBA);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else { std::cout << "Texture failed to load at path: " << path << std::endl; stbi_image_free(data); }
    return textureID;
}
