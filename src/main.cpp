#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <iostream>
#include <stb_image.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(char const * path);

unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool blinn = false;
bool blinnKeyPressed = false;
bool freeCamKeyPressed = false;

glm::vec3 lightColor = glm::vec3(120.0f,80.0f,45.0f);

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct SpotLight {
    float constant;
    float linear;
    float quadratic;
};



struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;

    //house
    glm::vec3 housePosition = glm::vec3(0.0f, 0.0f, -15.0f);
    float houseRotation = 0.0f;
    float houseScale = 1.6f;

    //trees
    glm::vec3 treePosition = glm::vec3(15.0f, 0.0f, 9.0f);
    glm::vec3 tree2Position = glm::vec3(-13.0f, 0.0f, 9.0f);
    float treeScale = 6.7f;

    //campfire
    glm::vec3 campfirePosition = glm::vec3(2.0f, 0.0f, 12.0f);
    float campfireScale = 1.0f;

    PointLight pointLight;
    DirLight dirLight;
    SpotLight spotLight;
    ProgramState()
            : camera(glm::vec3(2.5f, 3.0f, 30.0f)) {}
    void SaveToFile(std::string filename);
    void LoadFromFile(std::string filename);
};
void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        /*<< camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'*/
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}
void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           /*>> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z*/
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}
ProgramState *programState;
void DrawImGui(ProgramState *programState);
int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Computer Graphics Project", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);
    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    // configure global opengl state
    // -----------------------------

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // build and compile shaders
    // -------------------------

    Shader modelShader("resources/shaders/model_light.vs", "resources/shaders/model_light.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader cometShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");

    // load models
    // -----------

    Model treeModel("resources/objects/tree/tree.obj");
    treeModel.SetShaderTextureNamePrefix("material.");

    Model houseModel("resources/objects/house/house.obj");
    houseModel.SetShaderTextureNamePrefix("material.");

    Model campfireModel("resources/objects/campfire/campfire.obj");
    campfireModel.SetShaderTextureNamePrefix("material.");


    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(15.0f, 20.0, 25.0);
    pointLight.ambient = glm::vec3(lightColor.x*0.1, lightColor.y*0.1, lightColor.z*0.1);
    pointLight.diffuse = lightColor;
    pointLight.specular = lightColor;
    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    DirLight& dirLight = programState->dirLight;
    dirLight.direction = glm::vec3(6.0f, 8.0f, -53.0f);
    dirLight.ambient =   glm::vec3(0.05f, 0.05f, 0.20f);
    dirLight.diffuse =   glm::vec3( 0.4f, 0.4f, 0.6f);
    dirLight.specular =  glm::vec3(0.5f, 0.5f, 0.7f);

    SpotLight& spotLight = programState->spotLight;
    spotLight.constant = 1.0f;
    spotLight.linear = 0.12f;
    spotLight.quadratic = 0.04f;


    float terrainVertices[] = {
            25.0f,  0.0f,  25.0f, 0.0f, 1.0f, 0.0f, 20.0f, 0.0f,
            -25.0f,  0.0f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 20.0f,
            -25.0f,  0.0f,  25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            25.0f,  0.0f,  25.0f, 0.0f, 1.0f, 0.0f, 20.0f, 0.0f,
            25.0f,  0.0f, -25.0f, 0.0f, 1.0f, 0.0f, 20.0f, 20.0f,
            -25.0f,  0.0f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 20.0f
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

    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f, 0.0f,
            0.0f, -0.5f, 0.0f, 1.0f,
            1.0f, -0.5f, 0.0f, 1.0f,

            0.0f,  0.5f,  0.0f, 0.0f,
            1.0f, -0.5f,  0.0f, 1.0f,
            1.0f,  0.5f,  0.0f, 0.0f
    };


    // terrain
    unsigned int terrainVAO, terrainVBO;
    glGenVertexArrays(1, &terrainVAO);
    glGenBuffers(1, &terrainVBO);
    glBindVertexArray(terrainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(terrainVertices), &terrainVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6* sizeof(float)));
    glEnableVertexAttribArray(2);

    // skybox
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // transparent
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    //load textures
    //------------
    unsigned int terrainTexture = loadTexture(FileSystem::getPath("/resources/textures/terrain.jpeg").c_str());
    unsigned int cometTexture = loadTexture(FileSystem::getPath("resources/textures/comet.png").c_str());


    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    vector<std::string> faces
            {
                    FileSystem::getPath("/resources/textures/skybox/right.png"),
                    FileSystem::getPath("/resources/textures/skybox/left.png"),
                    FileSystem::getPath("/resources/textures/skybox/top.png"),
                    FileSystem::getPath("/resources/textures/skybox/bottom.png"),
                    FileSystem::getPath("/resources/textures/skybox/front.png"),
                    FileSystem::getPath("/resources/textures/skybox/back.png")
            };
    unsigned int cubemapTexture = loadCubemap(faces);

    vector<glm::vec3> comets
            {
                    glm::vec3(0.0f, 1.0f, -7.0f),
                    glm::vec3(2.0f, 1.5f, -4.0f),
                    glm::vec3(1.0f, 1.0f, -6.0f)
            };

    cometShader.use();
    cometShader.setInt("texture1", 0);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // input
        // -----
        processInput(window);
        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms

        modelShader.use();

        //directional light
        modelShader.setVec3("dirLight.direction", dirLight.direction);
        modelShader.setVec3("dirLight.ambient", dirLight.ambient);
        modelShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        modelShader.setVec3("dirLight.specular", dirLight.specular);

        //point light
        modelShader.setVec3("pointLight.position", pointLight.position);
        modelShader.setVec3("pointLight.ambient", pointLight.ambient);
        modelShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        modelShader.setVec3("pointLight.specular", pointLight.specular);
        modelShader.setFloat("pointLight.constant", pointLight.constant);
        modelShader.setFloat("pointLight.linear", pointLight.linear);
        modelShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        //spotlight (turn on if u want flashlight)
        modelShader.setVec3("spotLight.position", programState->camera.Position);
        modelShader.setVec3("spotLight.direction", programState->camera.Front);
        modelShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        modelShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        modelShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        modelShader.setFloat("spotLight.constant", spotLight.constant);
        modelShader.setFloat("spotLight.linear", spotLight.linear);
        modelShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        modelShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        modelShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
        modelShader.setVec3("viewPos", programState->camera.Position);
        modelShader.setFloat("material.shininess", 32.0f);
        modelShader.setInt("blinn", blinn);

        // view/projection transformations
        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);


        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);

        // tree
        model = glm::translate(model,
                               programState->treePosition);
        model = glm::scale(model,
                           glm::vec3(programState->treeScale));
        modelShader.setMat4("model", model);
        treeModel.Draw(modelShader);

        // tree
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               programState->tree2Position);
        model = glm::scale(model,
                           glm::vec3(programState->treeScale));
        modelShader.setMat4("model", model);
        treeModel.Draw(modelShader);

        glDisable(GL_CULL_FACE);

        // campfire
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               programState->campfirePosition);
        model = glm::scale(model,
                           glm::vec3(programState->campfireScale));
        modelShader.setMat4("model", model);
        campfireModel.Draw(modelShader);

        // house
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               programState->housePosition);
        model = glm::rotate(model,
                            glm::radians(programState->houseRotation), glm::vec3(0, 1, 0));
        model = glm::scale(model,
                           glm::vec3(programState->houseScale));
        modelShader.setMat4("model", model);
        houseModel.Draw(modelShader);

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(terrainVAO);
        glBindTexture(GL_TEXTURE_2D, terrainTexture);
        model = glm::mat4(1.0f);
        modelShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        cometShader.use();
        cometShader.setMat4("projection", projection);
        cometShader.setMat4("view", view);
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, cometTexture);
        for (unsigned int i = 0; i < comets.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, comets[i]);
            model = glm::scale(model, glm::vec3(20.5f));
            cometShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // draw skybox as last
        glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));// remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        //skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        if (programState->ImGuiEnabled)
            DrawImGui(programState);
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);
    glDeleteVertexArrays(1, &transparentVAO);
    glDeleteBuffers(1, &transparentVBO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);

    glfwTerminate();
    return 0;
}
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !blinnKeyPressed)
    {
        blinn = !blinn;
        blinnKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        blinnKeyPressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS && !freeCamKeyPressed)
    {
        programState->camera.freeCam = !programState->camera.freeCam;
        freeCamKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_RELEASE)
    {
        freeCamKeyPressed = false;
    }
}
// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;
    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}
// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}
void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("DirLight:");
        ImGui::DragFloat3("dirLight.direction", (float*)&programState->dirLight.direction);
        ImGui::DragFloat3("dirLight.ambient",  (float*)  &programState->dirLight.ambient);
        ImGui::DragFloat3("dirLight.diffuse", (float*) &programState->dirLight.diffuse);
        ImGui::DragFloat3("dirLight.specular",(float*) &programState->dirLight.specular);
        ImGui::Text("SpotLight:");
        ImGui::DragFloat("spotLight.constant", &programState->spotLight.constant, 0.05, 0.0, 3.0);
        ImGui::DragFloat("spotLight.linear", &programState->spotLight.linear, 0.05, 0.0, 3.0);
        ImGui::DragFloat("spotLight.quadratic", &programState->spotLight.quadratic, 0.05, 0.0, 3.0);
        ImGui::Text("PointLight:");
        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 30.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 30.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 3.0);
        ImGui::Text("Farmhouse: ");
        ImGui::DragFloat3("Farmhouse position", (float*)&programState->housePosition);
        ImGui::DragFloat("Farmhouse rotation", &programState->houseRotation, 1.0, 0, 360);
        ImGui::DragFloat("Farmhouse scale", &programState->houseScale, 0.2, 0.2, 500.0);
        ImGui::End();
    }
    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
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