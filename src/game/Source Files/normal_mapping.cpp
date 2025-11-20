#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// Particle structure for the explosion effect
struct Particle {
    glm::vec3 Position, Velocity;
    glm::vec4 Color;
    float Life;

    Particle() : Position(0.0f), Velocity(0.0f), Color(1.0f), Life(0.0f) {}
};

// Settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(-0.25f, 3.0f, -5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Physics
float playerVelocityY = 0.0f;
float gravity = 20.0f;
bool isGrounded = false;
float floorHeight = 3.0f;

// <<< CHANGE & EXPLANATION >>>
// This is the current "invisible wall" system. It's a simple boundary box.
// To fix walking through walls, you would need to replace this system
// with one that checks against the actual geometry of your map model,
// for example, by creating a vector of bounding boxes for each wall.
float mapLimitX = 100.0f;
float mapLimitZ = 100.0f;

// Laser
bool isShooting = false;
float laserLength = 100.0f;
float laserTimer = 0.0f;
float laserDuration = 0.1f;

// Barrels
std::vector<glm::vec3> barrelPositions;
std::vector<bool> barrelVisible;
// <<< CHANGE & EXPLANATION >>>
// Made the barrel model 2x bigger (0.02 -> 0.04)
const float barrelModelScale = 0.04f;
// Made the collision radius 2x bigger to match the new visual size.
// This invisible circle is used for both player collision and for hit detection when shooting.
const float barrelRadius = 0.8f;

// Particle container and functions
std::vector<Particle> particles;
unsigned int nr_new_particles = 100;
void SpawnParticles(glm::vec3 position);
unsigned int particleVAO;


int main()
{
    // --- Initialize GLFW and GLAD ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Shoot or Die", NULL, NULL);
    if (window == NULL) { std::cout << "Failed to create GLFW window" << std::endl; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cout << "Failed to initialize GLAD" << std::endl; return -1; }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- Build Shaders and Load Models ---
    Shader ourShader("4.normal_mapping.vs", "4.normal_mapping.fs");
    Shader barrelShader("4.normal_mapping.vs", "4.normal_mapping.fs");
    Shader laserShader("laser.vs", "laser.fs");
    Shader crosshairShader("crosshair.vs", "crosshair.fs");
    Shader particleShader("particle.vs", "particle.fs");

    Model mapModel(FileSystem::getPath("resources/objects/map/Untitled1.obj"));
    Model barrelModel(FileSystem::getPath("resources/objects/Barrel/Barrels_OBJ.obj"));

    // --- Barrel Positions ---
    barrelPositions.push_back(glm::vec3(5.0f, 0.0f, 5.0f));
    barrelPositions.push_back(glm::vec3(-5.0f, 0.0f, 8.0f));
    barrelPositions.push_back(glm::vec3(15.0f, 0.0f, -12.0f));
    barrelVisible.assign(barrelPositions.size(), true);

    // --- Setup VAOs for laser, crosshair, and particles ---
    unsigned int laserVAO, laserVBO;
    glGenVertexArrays(1, &laserVAO); glGenBuffers(1, &laserVBO);
    glBindVertexArray(laserVAO); glBindBuffer(GL_ARRAY_BUFFER, laserVBO);
    float laserVertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -laserLength };
    glBufferData(GL_ARRAY_BUFFER, sizeof(laserVertices), laserVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int crosshairVAO, crosshairVBO;
    float crosshairVertices[] = { -0.05f, 0.0f, 0.05f, 0.0f, 0.0f, -0.05f, 0.0f, 0.05f };
    glGenVertexArrays(1, &crosshairVAO); glGenBuffers(1, &crosshairVBO);
    glBindVertexArray(crosshairVAO); glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &particleVAO);
    glBindVertexArray(particleVAO);

    glm::vec3 lightDirection = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 ambientLight = glm::vec3(0.9f);

    // --- Render Loop ---
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        playerVelocityY -= gravity * deltaTime;
        camera.Position.y += playerVelocityY * deltaTime;
        if (camera.Position.y < floorHeight) {
            camera.Position.y = floorHeight;
            playerVelocityY = 0.0f;
            isGrounded = true;
        }

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 500.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model;

        // --- Draw Map ---
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setVec3("light.direction", lightDirection);
        ourShader.setVec3("viewPos", camera.Position);
        ourShader.setVec3("light.ambient", ambientLight);
        ourShader.setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
        ourShader.setVec3("light.specular", 0.8f, 0.8f, 0.8f);
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(10.0f));
        ourShader.setMat4("model", model);
        mapModel.Draw(ourShader);

        // --- Draw Barrels ---
        barrelShader.use();
        barrelShader.setMat4("projection", projection);
        barrelShader.setMat4("view", view);
        barrelShader.setVec3("light.direction", lightDirection);
        barrelShader.setVec3("viewPos", camera.Position);
        barrelShader.setVec3("light.ambient", ambientLight);
        barrelShader.setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
        barrelShader.setVec3("light.specular", 0.8f, 0.8f, 0.8f);
        for (size_t i = 0; i < barrelPositions.size(); ++i) {
            if (barrelVisible[i]) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, barrelPositions[i]);
                model = glm::scale(model, glm::vec3(barrelModelScale));
                barrelShader.setMat4("model", model);
                barrelModel.Draw(barrelShader);
            }
        }

        // --- Update and Draw Particles ---
        for (auto& p : particles) { p.Life -= deltaTime; if (p.Life > 0.0f) { p.Position += p.Velocity * deltaTime; p.Velocity.y -= 5.0f * deltaTime; p.Color.a = p.Life * 2.0f; } }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.Life <= 0.0f; }), particles.end());
        if (!particles.empty()) { glDepthMask(GL_FALSE); particleShader.use(); particleShader.setMat4("projection", projection); particleShader.setMat4("view", view); glBindVertexArray(particleVAO); for (const auto& particle : particles) { particleShader.setVec3("offset", particle.Position); particleShader.setVec4("color", particle.Color); glDrawArrays(GL_POINTS, 0, 1); } glBindVertexArray(0); glDepthMask(GL_TRUE); }


        // <<< LASER FIX IS HERE >>>
        // ====================================================================================
        // --- Draw Laser Visual Effect ---
        if (isShooting) {
            laserTimer += deltaTime;
            if (laserTimer >= laserDuration) {
                isShooting = false;
            }
            else {
                laserShader.use();
                laserShader.setMat4("projection", projection);
                laserShader.setMat4("view", view);

                model = glm::mat4(1.0f);

                // This is the corrected position calculation.
                // It starts the laser in front of the camera and slightly below center.
                glm::vec3 laserOrigin = camera.Position + (camera.Front * 0.5f) - (camera.Up * 0.2f);
                model = glm::translate(model, laserOrigin);

                // This rotation aligns the laser line (which points down the Z-axis)
                // with the camera's forward direction.
                model = model * glm::mat4_cast(glm::rotation(glm::vec3(0.0f, 0.0f, -1.0f), camera.Front));

                laserShader.setMat4("model", model);

                glLineWidth(5.0f);
                glBindVertexArray(laserVAO);
                glDrawArrays(GL_LINES, 0, 2);
                glBindVertexArray(0);
                glLineWidth(1.0f);
            }
        }
        // ====================================================================================


        // --- Draw Crosshair ---
        glDisable(GL_DEPTH_TEST);
        crosshairShader.use();
        glBindVertexArray(crosshairVAO);
        glDrawArrays(GL_LINES, 0, 4);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    glm::vec3 front = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
    glm::vec3 right = glm::normalize(glm::vec3(camera.Right.x, 0.0f, camera.Right.z));
    float velocity = 50.0f * deltaTime;
    glm::vec3 nextPos = camera.Position;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) nextPos += front * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) nextPos -= front * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) nextPos -= right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) nextPos += right * velocity;


    // ===================================================================
    // MAP COLLISION IS HANDLED HERE (CURRENTLY THE "INVISIBLE WALL")
    // This code just checks if you are past the mapLimitX/Z values.
    // To fix walking through walls, you would need to replace this block
    // with code that checks the player's `nextPos` against a list of
    // bounding boxes for your actual walls.
    // ===================================================================
    if (nextPos.x > mapLimitX) nextPos.x = camera.Position.x;
    if (nextPos.x < -mapLimitX) nextPos.x = camera.Position.x;
    if (nextPos.z > mapLimitZ) nextPos.z = camera.Position.z;
    if (nextPos.z < -mapLimitZ) nextPos.z = camera.Position.z;

    // ===================================================================
    // BARREL COLLISION (PLAYER WALKING INTO IT)
    // This code checks the distance between the player and the center of each barrel.
    // If the distance is less than the barrel's radius plus the player's radius (0.5f),
    // it stops the player from moving. It uses the `barrelRadius` variable.
    // ===================================================================
    for (size_t i = 0; i < barrelPositions.size(); ++i) {
        if (barrelVisible[i]) {
            float dist = glm::distance(glm::vec2(nextPos.x, nextPos.z), glm::vec2(barrelPositions[i].x, barrelPositions[i].z));
            if (dist < barrelRadius + 5.0f) { // Using the updated barrelRadius
                nextPos = camera.Position;
                break;
            }
        }
    }

    camera.Position.x = nextPos.x;
    camera.Position.z = nextPos.z;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded)
    {
        playerVelocityY = 8.0f;
        isGrounded = false;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !isShooting)
    {
        isShooting = true;
        laserTimer = 0.0f;

        glm::vec3 rayOrigin = camera.Position;
        glm::vec3 rayDir = camera.Front;
        for (size_t i = 0; i < barrelPositions.size(); ++i) {
            if (barrelVisible[i]) {
                glm::vec3 oc = rayOrigin - barrelPositions[i];
                float a = glm::dot(rayDir, rayDir);
                float b = 2.0f * glm::dot(oc, rayDir);
                // BARREL COLLISION (SHOOTING IT)
                // This is the math for ray-sphere intersection.
                // It uses the `barrelRadius` variable to determine the barrel's hitbox size.
                float c = glm::dot(oc, oc) - (barrelRadius + 5.0f) * (barrelRadius + 5.0f); // Using the updated barrelRadius
                float discriminant = b * b - 4 * a * c;
                if (discriminant > 0) {
                    barrelVisible[i] = false;
                    SpawnParticles(barrelPositions[i]);
                    break;
                }
            }
        }
    }
}

void SpawnParticles(glm::vec3 position)
{
    for (unsigned int i = 0; i < nr_new_particles; ++i) {
        Particle p;
        p.Position = position + glm::vec3(0.0f, barrelRadius, 0.0f);
        p.Velocity = glm::ballRand(5.0f);
        p.Color = glm::vec4(1.0f, glm::linearRand(0.2f, 0.6f), 0.0f, 1.0f);
        p.Life = glm::linearRand(0.5f, 1.5f);
        particles.push_back(p);
    }
}

// Other callbacks remain the same
void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) { camera.ProcessMouseScroll(static_cast<float>(yoffset)); }