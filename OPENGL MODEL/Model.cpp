#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Model.h"


#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

static constexpr int numCubeLight{ 4 };

// for model loading 
Model* currentModel = nullptr;
std::string modelPath = "resources/models/Sponza-master/sponza.obj";
float modelScale = 0.01f;

void modelLoading();

// screen color
glm::vec4 screenColor(0.0f, 0.0f, 0.0f, 1.0f);

// data for ImGui window
struct DirLight
{
    glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 specular = glm::vec3(0.5f, 0.5f, 0.5f);
};

DirLight dirLightData{};

struct PointLight
{
    glm::vec3 ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    glm::vec3 diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);

    // with distance = 50
    float constant{ 1.0f };
    float linear{ 0.09f };
    float quadratic{ 0.032f };
};

PointLight pointLightData[numCubeLight];


struct SpotLight
{
    glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 specular = glm::vec3(0.5f, 0.5f, 0.5f);

    float constant{ 1.0f };
    float linear{ 0.09f };
    float quadratic{ 0.032f };
    float cutOff{ 12.5f };
    float outerCutOff{ 17.5f };
};


SpotLight spotLightData{};


void directionalLightChange();
void pointLightChange();
void spotLightChange();


// Attenuation presets based on distance
struct AttenuationPreset
{
    const char* name{};     // display name (e.g. distance 50)
    float linear{};
    float quadratic{};
};

// common attenuation value for distances
static AttenuationPreset attenuationPreset[]{
    {"7 units", 0.7f, 1.8f},
    {"13 units", 0.35f, 0.44f},
    {"20 units", 0.22f, 0.2f},
    {"32 units", 0.14f, 0.07f},
    {"50 units", 0.09f, 0.032f},
    {"65 units", 0.07f, 0.017f},
    {"100 units", 0.045f, 0.0075f},
    // add more if you want to
};

static const int numPreset = IM_ARRAYSIZE(attenuationPreset);

// track which preset is being selected with each light
static int selectedPresetIndex[4] = { 4,4,4,4 };    // default to 50 units
static int selectSpotlightIndex = 4;

// check blinn phong model
bool blinn = false;
bool blinnKeyPress = false;

float SCR_WIDTH{ 1920.0f };
float SCR_HEIGHT{ 1080.0f };

bool showImGuiWindow{ true };

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));


// speed: use deltaTime (calculate the time between the last frame and current frame)
float deltaTime{};
float lastFrame{};

// mouse movement
// last mouse position
float lastX{ SCR_WIDTH / 2.0f };
float lastY{ SCR_HEIGHT / 2.0f };
bool firstMouse{ true };

// yaw and pitch value
float yaw{ -90.0f };
float pitch{ 0.0f };
// xpos is current x position
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

    ImGuiIO& io = ImGui::GetIO();

    // if in imgui window, stop all mouse movement
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
        return;

    if (io.WantCaptureMouse)
        return;

    // check if is this the 1st mouse movement
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // calculate the offset position
    float xoffset = xpos - lastX;
    float yoffset = -(ypos - lastY);   // inverted since in GLFW Y-coordinate go from top->bottom

    // update last position to current position
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    float velocity = 2.5 * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // move camera with WASD
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // for ImGui window
    static bool tabWasPressed{ false };

    // open imgui window when press key of choice
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS)
    {
        if (!tabWasPressed)
        {
            showImGuiWindow = !showImGuiWindow;
            glfwSetInputMode(window, GLFW_CURSOR, showImGuiWindow ?
                GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
            if (!showImGuiWindow)
                firstMouse = true;

        }
        tabWasPressed = true;

    }

    else
    {
        tabWasPressed = false;
    }

    // switch between blinn phong and phong lighting model
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !blinnKeyPress)
    {
        blinn = !blinn;
        blinnKeyPress = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        blinnKeyPress = false;
    }
}

// load cube map
unsigned int loadCubeMap(const std::vector<std::string>& faces);

glm::vec3 pointLightPositions[] = {
glm::vec3(0.7f,  0.2f,  2.0f),
glm::vec3(2.3f, -3.3f, -4.0f),
glm::vec3(-4.0f,  2.0f, -12.0f),
glm::vec3(0.0f,  0.0f, -3.0f)
};



int main()
{

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // first create a window
    GLFWwindow* window{ glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lighting", NULL, NULL) };


    if (window == nullptr)
    {
        std::cout << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    // initialize GLAD to use modern OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    // set this before cursor callback
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");



    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    //glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);



    // tell stbi_image to flip the loaded texture on the y axis
    //stbi_set_flip_vertically_on_load(true);

    // Enable Depth test (Z-buffer) to correctly render cube
    glEnable(GL_DEPTH_TEST);

    // Initialize our shader
    Shader shader("resources/shader/model.vs", "resources/shader/model.fs");

    Shader lightCubeShader{ "resources/shader/lightVertex.vs",  "resources/shader/lightFragment.fs" };

    Shader cubeMapShader{ "resources/shader/cubemap.vs", "resources/shader/cubemap.fs" };

    // load models
    currentModel = new Model(modelPath);

    // cube vertices data
  // this time with Normal vector as the 2nd attribue
  // Normal vector is a vector that is perpendicular to the vertex's surface
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    // flip to load skybox correctly
    stbi_set_flip_vertically_on_load(false);
    // skybox faces
    std::vector<std::string> faces{
        "resources/textures/skybox/right.jpg",
        "resources/textures/skybox/left.jpg",
        "resources/textures/skybox/top.jpg",
        "resources/textures/skybox/bottom.jpg",
        "resources/textures/skybox/back.jpg",
        "resources/textures/skybox/front.jpg",
    };

    unsigned int skyboxTexture{ loadCubeMap(faces) };

    // flip back to load model correctly
    stbi_set_flip_vertically_on_load(true);
    // cube map box
    unsigned int skyboxVAO, skyboxVBO;
    glGenBuffers(1, &skyboxVBO);
    glGenVertexArrays(1, &skyboxVAO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int lightVAO{}, VBO{};
    glGenVertexArrays(1, &lightVAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    cubeMapShader.use();
    cubeMapShader.setInt("skybox", 0);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);



    // render loop
    while (!glfwWindowShouldClose(window))
    {

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        processInput(window);

        // clear buffer color and set the windows color
        glClearColor(screenColor.r, screenColor.g, screenColor.b, screenColor.a);
        // clear color buffer and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;


        // Now activate our shader programs
        shader.use();


        // transformation
        glm::mat4 model{ glm::mat4(1.0f) };         // set to identity matrix
        glm::mat4 view{ glm::mat4(1.0f) };          // set to identity matrix
        glm::mat4 projection{ glm::mat4(1.0f) };    // set to identity matrix


        view = camera.GetViewMatrix();
        projection = glm::perspective((45.0f), SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        shader.setFloat("material.shininess", 32.0f); // Typical range: 8.0 to 256.0


        shader.setVec3("viewPos", camera.Position);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setBool("blinn", blinn);
        // for directional lights
        shader.setVec3("dirLight.direction", dirLightData.direction);
        shader.setVec3("dirLight.ambient", dirLightData.ambient);
        shader.setVec3("dirLight.diffuse", dirLightData.diffuse);
        shader.setVec3("dirLight.specular", dirLightData.specular);

        // for point lights
        for (int i{ 0 }; i < 4; ++i)
        {
            shader.setVec3("pointLights[" + std::to_string(i) + "].position",
                pointLightPositions[i]);
            shader.setVec3("pointLights[" + std::to_string(i) + "].ambient",
                pointLightData[i].ambient);
            shader.setVec3("pointLights[" + std::to_string(i) + "].diffuse",
                pointLightData[i].diffuse);
            shader.setVec3("pointLights[" + std::to_string(i) + "].specular",
                pointLightData[i].specular);
            shader.setFloat("pointLights[" + std::to_string(i) + "].constant",
                1.0f);
            shader.setFloat("pointLights[" + std::to_string(i) + "].linear",
                pointLightData[i].linear);
            shader.setFloat("pointLights[" + std::to_string(i) + "].quadratic",
                pointLightData[i].quadratic);

        }

        // for spotlight - flashlight
        shader.setVec3("spotLight.position", camera.Position);
        shader.setVec3("spotLight.direction", camera.Front);
        shader.setFloat("spotLight.cutOff", cos(glm::radians(spotLightData.cutOff)));
        shader.setFloat("spotLight.outerCutOff", cos(glm::radians(spotLightData.outerCutOff)));

        shader.setVec3("spotLight.ambient", spotLightData.ambient);
        shader.setVec3("spotLight.diffuse", spotLightData.diffuse);
        shader.setVec3("spotLight.specular", spotLightData.specular);

        shader.setFloat("spotLight.constant", 1.0f);
        shader.setFloat("spotLight.linear", spotLightData.linear);
        shader.setFloat("spotLight.quadratic", spotLightData.quadratic);



        // render the loaded model
        //model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // center of the screen
        if (currentModel)
        {
            model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(modelScale));
            shader.setMat4("model", model);
            currentModel->Draw(shader);
        }

        lightCubeShader.use();
        lightCubeShader.setMat4("view", view);
        lightCubeShader.setMat4("projection", projection);

        glBindVertexArray(lightVAO);

        for (int i{ 0 }; i < numCubeLight; ++i)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pointLightPositions[i]);
            model = glm::scale(model, glm::vec3(0.5f));
            lightCubeShader.setMat4("model", model);

            // Optional: Set light color (if your fragment shader supports it)
            // lightCubeShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
            lightCubeShader.setVec3("lightColor", pointLightData[i].diffuse);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // draw the skybox last to save performance
        //
        glDepthFunc(GL_LEQUAL);
        cubeMapShader.use();
        // remove translation
        glm::mat4 skyboxView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        cubeMapShader.setMat4("view", skyboxView);
        cubeMapShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);


        // call every time resizing a window
        //glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        // toggle imgui window
        if (showImGuiWindow)
        {
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            // ImGui window creation
            ImGui::Begin("My name is window, ImGui window");
            // Text that appears in the window
            ImGui::Text("Hello");
            ImGui::ColorEdit3("Screen Color", glm::value_ptr(screenColor));

            modelLoading();
            directionalLightChange();
            pointLightChange();
            spotLightChange();


            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // double buffer
        glfwSwapBuffers(window);
        // record any events (keyboard, mouse input, ...) and execute the corresponding callback functions
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();


    glfwTerminate();
    return 0;
}


void modelLoading()
{

    ImGui::InputText("Model path", modelPath.data(), modelPath.capacity());

    // scale adjust
    ImGui::Text("Uniform Scale");
    ImGui::SliderFloat("##scaleSlider", &modelScale, 0.01f, 1.0f, "%.3f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);  // Make input box narrower
    ImGui::InputFloat("Manual", &modelScale, 0.01f, 0.01f, "%.4f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Precise scale value");

    ImGui::SameLine();
    if (ImGui::Button("Reset Scale")) {
        modelScale = 0.01f;
    }



    if (ImGui::Button("Load Model"))
    {
        // delete old model if exist
        delete currentModel;
        currentModel = nullptr;

        // else try to load new model
        try
        {
            currentModel = new Model(modelPath);
        }
        catch (...)
        {
            ImGui::OpenPopup("Load Error");
        }

        if (ImGui::BeginPopupModal("Load Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to load model: %s", modelPath.c_str());
            if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
}


void directionalLightChange()
{
    if (ImGui::TreeNode("Directional light"))
    {
        if (ImGui::TreeNode("position"))
        {
            ImGui::SliderFloat3("directional light position",
                glm::value_ptr(dirLightData.direction), -1.0f, 1.0f);
            ImGui::TreePop();

        }

        if (ImGui::TreeNode("lighting"))
        {
            // for directional lighting
            ImGui::ColorEdit3("Directional ambient",
                glm::value_ptr(dirLightData.ambient));

            ImGui::ColorEdit3("Directional diffuse",
                glm::value_ptr(dirLightData.diffuse));

            ImGui::ColorEdit3("Directional specular",
                glm::value_ptr(dirLightData.specular));

            ImGui::TreePop();

        }
        ImGui::TreePop();

    }


}

void pointLightChange()
{
    if (ImGui::TreeNode("Point Lighting"))
    {
        if (ImGui::TreeNode("position"))
        {
            ImGui::SliderFloat3("light 1 position", glm::value_ptr(pointLightPositions[0]),
                -10.0f, 10.0f);
            ImGui::SliderFloat3("light 2 position", glm::value_ptr(pointLightPositions[1]),
                -10.0f, 10.0f);
            ImGui::SliderFloat3("light 3 position", glm::value_ptr(pointLightPositions[2]),
                -10.0f, 10.0f);
            ImGui::SliderFloat3("light 4 position", glm::value_ptr(pointLightPositions[3]),
                -10.0f, 10.0f);
            // display the list 
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("lighting"))
        {
            if (ImGui::TreeNode("Point light 1"))
            {
                ImGui::ColorEdit3("Ambient", glm::value_ptr(pointLightData[0].ambient));
                ImGui::ColorEdit3("Diffuse", glm::value_ptr(pointLightData[0].diffuse));
                ImGui::ColorEdit3("Specular", glm::value_ptr(pointLightData[0].specular));

                // display box
                std::string comboLabel = "Distance##combo" + std::to_string(0);
                if (ImGui::Combo(comboLabel.c_str(), &selectedPresetIndex[0],
                    [](void* data, int index, const char** out_text) {
                        AttenuationPreset* preset = (AttenuationPreset*)data;
                        *out_text = preset[index].name;
                        return true;
                    }, attenuationPreset, numPreset))

                    pointLightData[0].linear = attenuationPreset[selectedPresetIndex[0]].linear;
                pointLightData[0].quadratic = attenuationPreset[selectedPresetIndex[0]].quadratic;

                ImGui::Text("Linear: %.3f", pointLightData[0].linear);
                ImGui::Text("Quadratic: %.3f", pointLightData[0].quadratic);


                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Point light 2"))
            {
                ImGui::ColorEdit3("Ambient", glm::value_ptr(pointLightData[1].ambient));
                ImGui::ColorEdit3("Diffuse", glm::value_ptr(pointLightData[1].diffuse));
                ImGui::ColorEdit3("Specular", glm::value_ptr(pointLightData[1].specular));

                // display box
                std::string comboLabel = "Distance##combo" + std::to_string(1);
                if (ImGui::Combo(comboLabel.c_str(), &selectedPresetIndex[1],
                    [](void* data, int index, const char** out_text) {
                        AttenuationPreset* preset = (AttenuationPreset*)data;
                        *out_text = preset[index].name;
                        return true;
                    }, attenuationPreset, numPreset))

                    pointLightData[1].linear = attenuationPreset[selectedPresetIndex[1]].linear;
                pointLightData[1].quadratic = attenuationPreset[selectedPresetIndex[1]].quadratic;

                ImGui::Text("Linear: %.3f", pointLightData[1].linear);
                ImGui::Text("Quadratic: %.3f", pointLightData[1].quadratic);


                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Point light 3"))
            {
                ImGui::ColorEdit3("Ambient", glm::value_ptr(pointLightData[2].ambient));
                ImGui::ColorEdit3("Diffuse", glm::value_ptr(pointLightData[2].diffuse));
                ImGui::ColorEdit3("Specular", glm::value_ptr(pointLightData[2].specular));


                // display box
                std::string comboLabel = "Distance##combo" + std::to_string(2);
                if (ImGui::Combo(comboLabel.c_str(), &selectedPresetIndex[2],
                    [](void* data, int index, const char** out_text) {
                        AttenuationPreset* preset = (AttenuationPreset*)data;
                        *out_text = preset[index].name;
                        return true;
                    }, attenuationPreset, numPreset))

                    pointLightData[2].linear = attenuationPreset[selectedPresetIndex[2]].linear;
                pointLightData[2].quadratic = attenuationPreset[selectedPresetIndex[2]].quadratic;

                ImGui::Text("Linear: %.3f", pointLightData[2].linear);
                ImGui::Text("Quadratic: %.3f", pointLightData[2].quadratic);


                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Point light 4"))
            {
                ImGui::ColorEdit3("Ambient", glm::value_ptr(pointLightData[3].ambient));
                ImGui::ColorEdit3("Diffuse", glm::value_ptr(pointLightData[3].diffuse));
                ImGui::ColorEdit3("Specular", glm::value_ptr(pointLightData[3].specular));

                // display box
                std::string comboLabel = "Distance##combo" + std::to_string(3);
                if (ImGui::Combo(comboLabel.c_str(), &selectedPresetIndex[3],
                    [](void* data, int index, const char** out_text) {
                        AttenuationPreset* preset = (AttenuationPreset*)data;
                        *out_text = preset[index].name;
                        return true;
                    }, attenuationPreset, numPreset))

                    pointLightData[3].linear = attenuationPreset[selectedPresetIndex[3]].linear;
                pointLightData[3].quadratic = attenuationPreset[selectedPresetIndex[3]].quadratic;

                ImGui::Text("Linear: %.3f", pointLightData[3].linear);
                ImGui::Text("Quadratic: %.3f", pointLightData[3].quadratic);

                ImGui::TreePop();
            }

            ImGui::TreePop();
        }

        ImGui::TreePop();

    }

}

void spotLightChange()
{
    if (ImGui::TreeNode("Spotlight Lighting"))
    {
        ImGui::ColorEdit3("Ambient", glm::value_ptr(spotLightData.ambient));
        ImGui::ColorEdit3("Diffuse", glm::value_ptr(spotLightData.diffuse));
        ImGui::ColorEdit3("Specular", glm::value_ptr(spotLightData.specular));

        ImGui::SliderFloat("Cutoff angle (deg)", &spotLightData.cutOff, 0.0f, 90.0f);
        ImGui::SliderFloat("Outer cutoff angle (deg)", &spotLightData.outerCutOff, 0.0f, 90.0f);

        // display box
        std::string comboLabel = "Distance##combo" + std::to_string(0);
        if (ImGui::Combo(comboLabel.c_str(), &selectSpotlightIndex,
            [](void* data, int index, const char** out_text) {
                AttenuationPreset* preset = (AttenuationPreset*)data;
                *out_text = preset[index].name;
                return true;
            }, attenuationPreset, numPreset))

            spotLightData.linear = attenuationPreset[selectSpotlightIndex].linear;
        spotLightData.quadratic = attenuationPreset[selectSpotlightIndex].quadratic;

        ImGui::Text("Linear: %.3f", spotLightData.linear);
        ImGui::Text("Quadratic: %.3f", spotLightData.quadratic);


        ImGui::TreePop();
    }
}


// load cubemap texture
unsigned int loadCubeMap(const std::vector<std::string>& faces)
{
    unsigned int textureID{};
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i{ 0 }; i < faces.size(); ++i)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0,
                GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Texture failed to load at path: " << faces[i] << '\n';
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}