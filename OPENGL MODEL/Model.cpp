#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <random>

#include "stb_image.h"
#include "Shader.h"
#include "Camera.h"
#include "Model.h"


#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

static constexpr int numCubeLight{ 500 };

// Settings
float SCR_WIDTH{ 1920.0f };
float SCR_HEIGHT{ 1080.0f };

// Benchmarking variables
float shadowPassTime = 0.0f;
float geometryPassTime = 0.0f;
float lightingPassTime = 0.0f;

// For deferred debug quad
const unsigned int debugWidth = SCR_WIDTH / 4.0;
const unsigned int debugHeight = SCR_HEIGHT / 4.0;

// toggles
bool useDeferred = true;
int activeLightCount = 32; // Default starting lights (adjustable)

// for linear interpolation
float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

// for model loading 
std::unique_ptr<Model> currentModel = nullptr;
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
void renderQuad();


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

//glm::vec3 pointLightPositions[] = {
//glm::vec3(0.7f,  0.2f,  2.0f),
//glm::vec3(2.3f, -3.3f, -4.0f),
//glm::vec3(-4.0f,  2.0f, -12.0f),
//glm::vec3(0.0f,  0.0f, -3.0f)
//};
//
std::vector<glm::vec3> pointLightPositions;


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
    Shader forwardShader("resources/shader/model.vs", "resources/shader/model.fs");

    // For light sources
    Shader lightCubeShader{ "resources/shader/lightVertex.vs", "resources/shader/lightFragment.fs" };
    Shader cubeMapShader{ "resources/shader/cubemap.vs", "resources/shader/cubemap.fs" };

    // For Shadow mapping
    Shader simpleDepthShader("resources/shader/shadowDepth.vs", "resources/shader/shadowDepth.fs");
    Shader pointDepthShader("resources/shader/shadows/pointDepth.vs", 
        "resources/shader/shadows/pointDepth.fs", "resources/shader/shadows/geometryDepth.gs");
    
    // For deferred shading
    Shader shaderGeometryPass("resources/shader/deferred/geoDeferred.vs", "resources/shader/deferred/geoDeferred.fs");;
    Shader shaderLightingPass("resources/shader/deferred/lightingDeferred.vs", "resources/shader/deferred/lightingDeferred.fs");
    Shader debugDeferred("resources/shader/deferred/debug.vs", "resources/shader/deferred/debug.fs");
    
    // For SSAO
    Shader ssaoShader("resources/shader/ssao/ssao.vs", "resources/shader/ssao/ssao.fs");
    Shader ssaoBlur("resources/shader/ssao/blur.vs", "resources/shader/ssao/blur.fs");


    // load models
    currentModel = std::make_unique<Model>(modelPath);

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

    // framebuffer for depth map
    unsigned int depthMapFBO{};
    glGenFramebuffers(1, &depthMapFBO);

    // depth map resolution
    unsigned int SHADOW_WIDTH = 1024;
    unsigned int SHADOW_HEIGHT = 1024;

    unsigned int depthMap{};
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT,
        0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    std::vector<float> borderColor{ 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor.data());

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    // tell OpenGL we dont need to render any color data
    glDrawBuffer(GL_NONE);
    glDrawBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    
    // framebuffer point shadows
    unsigned int pointDepthFBO{};
    glGenFramebuffers(1, &pointDepthFBO);

    unsigned int depthCubeMap{};
    glGenTextures(1, &depthCubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
    for (unsigned int i{ 0 }; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
            SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

   // render all 6 faces in 1 pass using geometry shader (dont have to loop 6 times)
    glBindFramebuffer(GL_FRAMEBUFFER, pointDepthFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0);
    // tell OpenGL we dont need to render any color data
    glDrawBuffer(GL_NONE);
    glDrawBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // Setup G-buffer
    unsigned int gBuffer{};
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gAlbedoSpec;

    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA,
        GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    // normal buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA,
        GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // albedo and specular buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA,
        GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

    // tell opengl which color attachment we will use
    std::vector<unsigned int> attachments{ GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1 , GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments.data());

    // create and attach depth buffer
    unsigned int rboDepth{};
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // check if framebuffer is completed
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Framebuffer is not completed" << '\n';
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // SSAO configuration
    // Distribute 64 random sample values inside the hemisphere
    std::uniform_real_distribution<float> randomFloat{ 0.0, 1.0 };
    std::default_random_engine generator{};
    std::vector<glm::vec3> ssaoKernel{};
    constexpr int kernelSize {64};

    for (unsigned int i{ 0 }; i < kernelSize; ++i)
    {
        glm::vec3 sample{
            randomFloat(generator) * 2.0 - 1.0,     // vary in range [-1,1]
            randomFloat(generator) * 2.0 - 1.0,
            randomFloat(generator)          // vary in range [0,1]
        };
        
        sample = glm::normalize(sample);
        sample *= randomFloat(generator);

        // find a point between 2 values (linear interpolation) to distribute
        // more kernel samples around the center
        float scale = (float)i / 64.0f;
        scale = lerp(0.1f, 0.1f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    // Use noise to increase sample count (without sacrificing performance)
    std::vector<glm::vec3> ssaoNoise{ };
    constexpr int noiseSize{ 16 };
    for (unsigned int i{ 0 }; i < noiseSize; ++i)
    {
        glm::vec3 noise{
            randomFloat(generator) * 2.0 - 1.0,     // vary in range [-1,1]
            randomFloat(generator) * 2.0 - 1.0,
            0.0f   // we want to rotate around z axis
        };
        ssaoNoise.push_back(noise);
    }

    // Store noise in a 4x4 texture
    unsigned int noiseTexture{};
    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, ssaoNoise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // SSAO buffer
    unsigned int ssaoFBO{};
    glGenFramebuffers(1, &ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

    // ambient occlusion
    unsigned int ssaoColorBuffer{};
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        ssaoColorBuffer, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // Ambiant occlusion blur
    unsigned int ssaoBlurFBO{};
    unsigned int ssaoBlurColorBuffer{};
    glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glGenTextures(1, &ssaoBlurColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        ssaoBlurColorBuffer,0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    cubeMapShader.use();
    cubeMapShader.setInt("skybox", 0);

    shaderLightingPass.use();
    shaderLightingPass.setInt("gPosition", 0);
    shaderLightingPass.setInt("gNormal", 1);
    shaderLightingPass.setInt("gAlbedoSpec", 2);
    shaderLightingPass.setInt("ssao", 3);
    // Use high slots (10 & 11) to avoid conflict with the Mesh class materials (0, 1, 2...)
    shaderLightingPass.setInt("shadowMapDir", 10);
    shaderLightingPass.setInt("shadowMapPoint", 11);

    // ssao
    ssaoShader.use();
    ssaoShader.setInt("gPosition", 0);
    ssaoShader.setInt("gNormal", 1);
    ssaoShader.setInt("texNoise", 2);
    ssaoShader.setVec2("noiseScale", SCR_WIDTH / 4.0, SCR_HEIGHT / 4.0);

    ssaoBlur.use();
    ssaoBlur.setInt("ssaoInput", 0);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    // lighting setting
    std::vector<glm::vec3> lightColors;
    srand(13);
    for (unsigned int i = 0; i < numCubeLight; i++)
    {
        // calculate slightly random offsets
        float xPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
        float yPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 4.0);
        float zPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
        pointLightPositions.push_back(glm::vec3(xPos, yPos, zPos));
        // also calculate random color
        float rColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
        float gColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
        float bColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
        lightColors.push_back(glm::vec3(rColor, gColor, bColor));
    }

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

        // Shadow mapping
        // PASS 1: SHADOW PASS
        // ---------------------------------------------------
        // Start shadow pass timer
        float timerStart = glfwGetTime();

        // Directional shadow
        simpleDepthShader.use();
        glm::mat4 model = glm::mat4(1.0f);

        // Use lightPos as the first argument
        float lightDistance = 20.0f; // Adjust based on your scene size
        glm::vec3 dirLightPos = -dirLightData.direction * lightDistance;
        glm::mat4 lightView = glm::lookAt(dirLightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::vec3 lightPos = pointLightPositions[0]; // Use first point light


        float near_plane = 1.0f;
        float far_plane = 25.0f;
        glm::mat4 lightProjection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, near_plane, far_plane);
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        glCullFace(GL_FRONT);
        simpleDepthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
    
        // render the loaded model
        if (currentModel)
        {
            model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(modelScale));
            simpleDepthShader.setMat4("model", model);
            currentModel->Draw(simpleDepthShader);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glCullFace(GL_BACK);

        // Point shadow cube map

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, pointDepthFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        // For point light shadow
        float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;


        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near_plane, far_plane);

        // transformation matrix for point light shadow
        std::vector<glm::mat4> shadowTransform{};
        shadowTransform.reserve(6);

        shadowTransform.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0),
            glm::vec3(0.0, -1.0, 0.0)));
        shadowTransform.push_back(shadowProj *
            glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransform.push_back(shadowProj *
            glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        shadowTransform.push_back(shadowProj *
            glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
        shadowTransform.push_back(shadowProj *
            glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransform.push_back(shadowProj *
            glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));


        pointDepthShader.use();
        model = glm::mat4(1.0);
        model = glm::scale(model, glm::vec3(modelScale));
        pointDepthShader.setMat4("model", model);
        pointDepthShader.setVec3("lightPos", lightPos);        
        pointDepthShader.setFloat("far_plane", far_plane);     

        for (unsigned int i{ 0 }; i < 6; ++i)
        {
            pointDepthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]",
                shadowTransform[i]);
        }

        currentModel->Draw(pointDepthShader);  
        glBindFramebuffer(GL_FRAMEBUFFER, 0);  
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        shadowPassTime = (glfwGetTime() - timerStart) * 1000.0f; // Convert to ms

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);

        if (useDeferred)
        {
            // PASS 2: DEFERRED GEOMETRY PASS
            // ---------------------------------------------------
            glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Start geometry pass timer
            timerStart = glfwGetTime();

            shaderGeometryPass.use();
            glm::mat4 view = camera.GetViewMatrix();
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);

            shaderGeometryPass.setMat4("view", view);
            shaderGeometryPass.setMat4("projection", projection);

            if (currentModel)
            {
                model = glm::mat4(1.0f);
                model = glm::scale(model, glm::vec3(modelScale));
                shaderGeometryPass.setMat4("model", model);
                currentModel->Draw(shaderGeometryPass);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            geometryPassTime = (glfwGetTime() - timerStart) * 1000.0f;

            // SSAO GENERATION PASS
            ssaoShader.use();
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
            glClear(GL_COLOR_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, noiseTexture);
            // send kernel + rotation
            for (unsigned int i{ 0 }; i < kernelSize; ++i)
            {
                ssaoShader.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
            }
            ssaoShader.setMat4("projection", projection);
            ssaoShader.setMat4("view", view);
            renderQuad();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // SSAO BLUR PASS
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
            glClear(GL_COLOR_BUFFER_BIT);

            ssaoBlur.use();
            glActiveTexture(GL_TEXTURE0);
            // read from ssao
            glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
            renderQuad();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);


            // PASS 3: DEFERRED LIGHTING PASS
            // ---------------------------------------------------
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            timerStart = glfwGetTime();

            shaderLightingPass.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, ssaoBlurColorBuffer);

            // Bind shadow maps
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, depthMap);
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);

            shaderLightingPass.setVec3("viewPos", camera.Position);
            shaderLightingPass.setBool("blinn", blinn); // Blinn toggle
            shaderLightingPass.setFloat("material.shininess", 32.0f);

            // Send Active Light Count to Shader 
            shaderLightingPass.setInt("nr_lights", activeLightCount);

            shaderLightingPass.setMat4("lightSpaceMatrix", lightSpaceMatrix);
            shaderLightingPass.setVec3("dirLight.direction", dirLightData.direction);
            shaderLightingPass.setVec3("dirLight.ambient", dirLightData.ambient);
            shaderLightingPass.setVec3("dirLight.diffuse", dirLightData.diffuse);
            shaderLightingPass.setVec3("dirLight.specular", dirLightData.specular);

            for (int i{ 0 }; i < activeLightCount; ++i)
            {
                std::string number = std::to_string(i);
                shaderLightingPass.setVec3("pointLights[" + number + "].position", pointLightPositions[i]);
                shaderLightingPass.setVec3("pointLights[" + number + "].ambient", pointLightData[i].ambient);
                shaderLightingPass.setVec3("pointLights[" + number + "].diffuse", pointLightData[i].diffuse);
                shaderLightingPass.setVec3("pointLights[" + number + "].specular", pointLightData[i].specular);
                shaderLightingPass.setFloat("pointLights[" + number + "].constant", 1.0f);
                shaderLightingPass.setFloat("pointLights[" + number + "].linear", pointLightData[i].linear);
                shaderLightingPass.setFloat("pointLights[" + number + "].quadratic", pointLightData[i].quadratic);
            }

            shaderLightingPass.setFloat("far_plane", far_plane);
            shaderLightingPass.setVec3("shadowCasterPos", pointLightPositions[0]);

            // Spot Light
            shaderLightingPass.setVec3("spotLight.position", camera.Position);
            shaderLightingPass.setVec3("spotLight.direction", camera.Front);
            shaderLightingPass.setFloat("spotLight.cutOff", cos(glm::radians(spotLightData.cutOff)));
            shaderLightingPass.setFloat("spotLight.outerCutOff", cos(glm::radians(spotLightData.outerCutOff)));
            shaderLightingPass.setVec3("spotLight.ambient", spotLightData.ambient);
            shaderLightingPass.setVec3("spotLight.diffuse", spotLightData.diffuse);
            shaderLightingPass.setVec3("spotLight.specular", spotLightData.specular);
            shaderLightingPass.setFloat("spotLight.constant", 1.0f);
            shaderLightingPass.setFloat("spotLight.linear", spotLightData.linear);
            shaderLightingPass.setFloat("spotLight.quadratic", spotLightData.quadratic);

            renderQuad();

            lightingPassTime = (glfwGetTime() - timerStart) * 1000.0f;

            // Blit (copy) depth buffer for forward rendering (Skybox/Light Cubes)
            glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        else
        {
            // ALTERNATIVE: FORWARD RENDERING PASS
            // ---------------------------------------------------
            // Reset timers since we arent using deferred steps
            geometryPassTime = 0.0f;
            lightingPassTime = 0.0f;

            timerStart = glfwGetTime();

            // 1. Render directly to default buffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            forwardShader.use();

            // Standard Matrices
            glm::mat4 view = camera.GetViewMatrix();
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
            forwardShader.setMat4("projection", projection);
            forwardShader.setMat4("view", view);
            forwardShader.setVec3("viewPos", camera.Position);

            forwardShader.setBool("blinn", blinn); // Essential for lighting model match
            forwardShader.setFloat("material.shininess", 32.0f); // Essential for specular

            // Lighting Uniforms (Must match the deferred setup exactly)
            forwardShader.setInt("nr_lights", activeLightCount);

            // Shadows
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, depthMap);
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
            forwardShader.setInt("shadowMapDir", 10);
            forwardShader.setInt("shadowMapPoint", 11);
            forwardShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
            forwardShader.setFloat("far_plane", far_plane);
            forwardShader.setVec3("shadowCasterPos", pointLightPositions[0]);

            // Directional Light
            forwardShader.setVec3("dirLight.direction", dirLightData.direction);
            forwardShader.setVec3("dirLight.ambient", dirLightData.ambient);
            forwardShader.setVec3("dirLight.diffuse", dirLightData.diffuse);
            forwardShader.setVec3("dirLight.specular", dirLightData.specular);

            // Point Lights Loop
            for (int i{ 0 }; i < activeLightCount; ++i)
            {
                std::string number = std::to_string(i);
                forwardShader.setVec3("pointLights[" + number + "].position", pointLightPositions[i]);
                forwardShader.setVec3("pointLights[" + number + "].ambient", pointLightData[i].ambient);
                forwardShader.setVec3("pointLights[" + number + "].diffuse", pointLightData[i].diffuse);
                forwardShader.setVec3("pointLights[" + number + "].specular", pointLightData[i].specular);
                forwardShader.setFloat("pointLights[" + number + "].constant", 1.0f);
                forwardShader.setFloat("pointLights[" + number + "].linear", pointLightData[i].linear);
                forwardShader.setFloat("pointLights[" + number + "].quadratic", pointLightData[i].quadratic);
            }

            // Spot Light
            forwardShader.setVec3("spotLight.position", camera.Position);
            forwardShader.setVec3("spotLight.direction", camera.Front);
            forwardShader.setFloat("spotLight.cutOff", cos(glm::radians(spotLightData.cutOff)));
            forwardShader.setFloat("spotLight.outerCutOff", cos(glm::radians(spotLightData.outerCutOff)));
            forwardShader.setVec3("spotLight.ambient", spotLightData.ambient);
            forwardShader.setVec3("spotLight.diffuse", spotLightData.diffuse);
            forwardShader.setVec3("spotLight.specular", spotLightData.specular);
            forwardShader.setFloat("spotLight.constant", 1.0f);
            forwardShader.setFloat("spotLight.linear", spotLightData.linear);
            forwardShader.setFloat("spotLight.quadratic", spotLightData.quadratic);

            // Draw Model
            if (currentModel)
            {
                model = glm::mat4(1.0f);
                model = glm::scale(model, glm::vec3(modelScale));
                forwardShader.setMat4("model", model);
                currentModel->Draw(forwardShader);
            }

            // Measure Forward Time as "Lighting Pass" time for simplicity in report
            lightingPassTime = (glfwGetTime() - timerStart) * 1000.0f;

        }



        // PASS 4: Forward pass 
        lightCubeShader.use();
        lightCubeShader.setMat4("view", view);
        lightCubeShader.setMat4("projection", projection);

        glBindVertexArray(lightVAO);

        for (int i{ 0 }; i < activeLightCount; ++i)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pointLightPositions[i]);
            model = glm::scale(model, glm::vec3(0.5f));
            lightCubeShader.setMat4("model", model);

            lightCubeShader.setVec3("lightColor", pointLightData[i].diffuse);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }


        // draw the skybox last to save performance
        glDepthFunc(GL_LEQUAL);
        cubeMapShader.use();
        // remove translation
        glm::mat4 skyboxView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        cubeMapShader.setMat4("view", skyboxView);
        cubeMapShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);

        // debug quad
        // 1. Save original viewport
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        // 2. setup debug rendering
        debugDeferred.use();
        // render on top of everything
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);

        // Debug quad (Top-Bottom: Position, Normal, Albedo, Specular)
        // Position
        glViewport(SCR_WIDTH - debugWidth, SCR_HEIGHT - debugHeight, debugWidth, debugHeight);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        debugDeferred.setInt("mode", 0);    // raw RGB value
        renderQuad();

        // Normal
        glViewport(SCR_WIDTH - debugWidth, SCR_HEIGHT - (debugHeight * 2), debugWidth, debugHeight);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        debugDeferred.setInt("mode", 2);    
        renderQuad();

        // Albedo
        glViewport(SCR_WIDTH - debugWidth, SCR_HEIGHT - (debugHeight * 3), debugWidth, debugHeight);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
        debugDeferred.setInt("mode", 0);    // raw RGB value
        renderQuad();

        glViewport(SCR_WIDTH - debugWidth, SCR_HEIGHT - (debugHeight * 4), debugWidth, debugHeight);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
        debugDeferred.setInt("mode", 1);    
        renderQuad();

        // 3. Restore original state
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glEnable(GL_DEPTH_TEST);

        // call every time resizing a window
        //glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        // toggle imgui window
        if (showImGuiWindow)
        {
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            ImGui::Begin("Performance Statistics");

            // --- Performance Data ---
            ImGui::Text("Total FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Total Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
            ImGui::Separator();

            // --- Render Settings ---
            ImGui::Text("Render Pipeline:");
            if (ImGui::Checkbox("Use Deferred Shading", &useDeferred)) {
                // Reset times when switching to avoid weird display
                shadowPassTime = 0.0f;
                geometryPassTime = 0.0f;
                lightingPassTime = 0.0f;
            }

            ImGui::Text("Light Settings:");
            // Slider to control active lights (1 to 80)
            ImGui::SliderInt("Active Lights", &activeLightCount, 1, numCubeLight);

            ImGui::Separator();
            ImGui::Text("Pass Breakdown:");
            ImGui::Text("Shadow Pass:   %.3f ms", shadowPassTime);
            if (useDeferred) {
                ImGui::Text("Geometry Pass: %.3f ms", geometryPassTime);
                ImGui::Text("Lighting Pass: %.3f ms", lightingPassTime);
            }
            else {
                ImGui::Text("Forward Pass:  %.3f ms", lightingPassTime);
            }
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
        currentModel = nullptr;

        // else try to load new model
        try
        {
            currentModel = std::make_unique<Model>(modelPath); 
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

// render quad
unsigned int quadVAO{};
unsigned int quadVBO{};

void renderQuad()
{
    if (!quadVAO)
    {
        // Positions
        glm::vec3 pos1(-1.0, 1.0, 0.0);
        glm::vec3 pos2(-1.0, -1.0, 0.0);
        glm::vec3 pos3(1.0, -1.0, 0.0);
        glm::vec3 pos4(1.0, 1.0, 0.0);

        // Texture coordinates
        glm::vec2 uv1(0.0, 1.0);
        glm::vec2 uv2(0.0, 0.0);
        glm::vec2 uv3(1.0, 0.0);
        glm::vec2 uv4(1.0, 1.0);

        // Normal
        glm::vec3 nm(0.0, 0.0, 1.0);

        glm::vec3 tangent1, tangent2;
        glm::vec3 bitangent1, bitangent2;

        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;

        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        bitangent1.x = f * (deltaUV2.x * edge1.x - deltaUV1.x * edge2.x);
        bitangent1.y = f * (deltaUV2.x * edge1.y - deltaUV1.x * edge2.y);
        bitangent1.z = f * (deltaUV2.x * edge1.z - deltaUV1.x * edge2.z);

        // 2nd triangle
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;

        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        bitangent2.x = f * (deltaUV2.x * edge1.x - deltaUV1.x * edge2.x);
        bitangent2.y = f * (deltaUV2.x * edge1.y - deltaUV1.x * edge2.y);
        bitangent2.z = f * (deltaUV2.x * edge1.z - deltaUV1.x * edge2.z);

        float quadVertices[] = {
            // positions            // normal         // texcoords  // tangent                          // bitangent
            pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
            pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
            pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

            pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
            pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
            pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));

    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}