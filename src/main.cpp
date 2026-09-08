#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui-master/imgui.h>
#include <imgui-master/backends/imgui_impl_glfw.h>
#include <imgui-master/backends/imgui_impl_opengl3.h>

#include <shader.h>
#include <objects.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void updateOrbitCamera();
void gui1();

unsigned int loadTexture(const std::string& path);

// for github alow commit

// настройки
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

float cameraGlobalSpeed = 4.0f; // speed of camera movement (without frame time)
float yaw   = -90.0f; // начальное направление по оси Y
float pitch = 0.0f;
float lastX = 400.0f; // половина ширины окна
float lastY = 300.0f; // половина высоты окна
bool firstMouse = true;
bool movement1 = false;
bool movement2 = false;
glm::vec3 cameraCenter = glm::vec3(0.0f);
float orbitDistance = 3.0f;
bool movement2KeyPressedLastFrame = false;

float deltaTime = 0.0f; // время между текущим и предыдущим кадром
float lastFrame = 0.0f; // время предыдущего кадра

float my_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
bool my_tool_active = true; 

std::vector<Object> cubes; // вектор обьектов
std::unordered_map<std::string, unsigned int> textureCache;

unsigned int loadTexture(const std::string& path)
{
    const auto cachedTexture = textureCache.find(path);
    if (cachedTexture != textureCache.end())
        return cachedTexture->second;

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cout << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    GLenum format = channels == 4 ? GL_RGBA : channels == 3 ? GL_RGB : GL_RED;
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    textureCache.emplace(path, texture);
    return texture;
}

bool saveSceneConfig(const std::string& path)
{
    for (const Object& object : cubes) {
        if (object.meshPath.empty()) {
            std::cout << "Cannot save scene: object '" << object.name
                      << "' has no OBJ path." << std::endl;
            return false;
        }
    }

    std::ofstream file(path);
    if (!file) {
        std::cout << "Cannot write scene config: " << path << std::endl;
        return false;
    }

        file << "# object <name> <x> <y> <z> <obj_path> "
            "<rotation_x> <rotation_y> <rotation_z> "
            "<scale_x> <scale_y> <scale_z> <texture_path>\n";

    file << std::fixed << std::setprecision(6);
    for (const Object& object : cubes) {
        file << "object " << (object.name.empty() ? "unnamed" : object.name) << ' '
             << object.position.x << ' ' << object.position.y << ' ' << object.position.z << ' '
             << object.meshPath << ' '
             << object.rotation.x << ' ' << object.rotation.y << ' ' << object.rotation.z << ' '
             << object.scale.x << ' ' << object.scale.y << ' ' << object.scale.z << ' '
             << object.texturePath << '\n';
    }

    std::cout << "Scene saved to " << path << std::endl;
    return true;
}

bool loadSceneConfig(const std::string& path)
{
    cubes.clear();
    std::ifstream file(path);
    if (!file) {
        std::cout << "Scene config not found: " << path << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        std::istringstream stream(line);
        std::string type;
        stream >> type;

        if (type.empty() || type[0] == '#')
            continue;
        if (type != "object") {
            std::cout << "Unknown scene command at line " << lineNumber << ": " << type << std::endl;
            continue;
        }

        std::string name;
        std::string meshPath;
        glm::vec3 position;
        glm::vec3 rotation(0.0f);
        glm::vec3 scale(1.0f);
        std::string texturePath = "include/resources/container.jpg";

        if (!(stream >> name >> position.x >> position.y >> position.z >> meshPath)) {
            std::cout << "Invalid object at scene line " << lineNumber << std::endl;
            return false;
        }

        if (!(stream >> rotation.x >> rotation.y >> rotation.z)) {
            stream.clear();
        }
        if (!(stream >> scale.x >> scale.y >> scale.z)) {
            stream.clear();
        }
        if (stream >> texturePath) {
            if (texturePath == "1")
                texturePath = "include/resources/awesomeface.png";
            else if (texturePath == "0")
                texturePath = "include/resources/container.jpg";
        }

        std::vector<float> vertices;
        std::string error;
        if (!loadObjMesh(meshPath, vertices, error)) {
            std::cout << "Failed to load object '" << name << "': " << error << std::endl;
            cubes.clear();
            return false;
        }

        unsigned int texture = loadTexture(texturePath);
        if (texture == 0) {
            cubes.clear();
            return false;
        }

        cubes.emplace_back(vertices, texture, position, rotation, scale,
                           name, meshPath, texturePath);
    }

    return !cubes.empty();
}


int main()
{
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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // изначально мышь не захвачена
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);


    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shader program
    Shader ourShader("include/shader.vs", "include/shader.fs");


    //std::cout << "1" << std::endl;
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init("#version 330");

    //std::cout << "2" << std::endl;



    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    // world space positions of our cubes
    glm::vec3 cubePositions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f),
        glm::vec3( 2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3( 2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3( 1.3f, -2.0f, -2.5f),
        glm::vec3( 1.5f,  2.0f, -2.5f),
        glm::vec3( 1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);



    // Load default textures used by the shader's second sampler and fallback scene.
    stbi_set_flip_vertically_on_load(true);
    unsigned int texture1 = loadTexture("include/resources/container.jpg");
    unsigned int texture2 = loadTexture("include/resources/awesomeface.png");
    if (texture1 == 0 || texture2 == 0) {
        glfwTerminate();
        return -1;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    ourShader.use();
    ourShader.setInt("texture1", 0);
    ourShader.setInt("texture2", 1);


    
    if (!loadSceneConfig("scene.cfg")) {
        std::cout << "Using built-in cube scene." << std::endl;
        for (int i = 0; i < 10; ++i) {
            cubes.emplace_back(
                std::vector<float>(vertices, vertices + sizeof(vertices) / sizeof(float)),
                (i % 2 == 0) ? texture1 : texture2,
                cubePositions[i],
                glm::vec3(0.0f),
                glm::vec3(1.0f),
                "Cube" + std::to_string(i),
                "",
                "include/resources/container.jpg"
            );
        }
    }

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        //std::cout << "костыль 1" << std::endl; //without this, it doesn't work (imgui adress boundary error)
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        
        gui1();

        
        // render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);

        // activate shader
        ourShader.use();

        // create transformations
        glm::mat4 view          = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        glm::mat4 projection    = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        // pass transformation matrices to the shader
        ourShader.setMat4("projection", projection); // note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
        ourShader.setMat4("view", view);


        int uni_Loc = glGetUniformLocation(ourShader.ID, "screenRatio");
        glUniform1f(uni_Loc, float(SCR_WIDTH) / float(SCR_HEIGHT)); 

        // render boxes
        glBindVertexArray(VAO);
        for (const auto& cube : cubes)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cube.textureID);
            ourShader.setMat4("model", cube.getModelMatrix());
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(cube.vertices.size() / 5));
        }
        //std::cout << "костыль 2" << std::endl;
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    static bool altPressedLastFrame = false;
    bool movement2KeyPressed = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;

    if (movement2KeyPressed && !movement2KeyPressedLastFrame) {
        movement2 = !movement2;
        movement1 = false;
        firstMouse = true;
        updateOrbitCamera();
    }
    movement2KeyPressedLastFrame = movement2KeyPressed;

    // Переключение захвата мыши по Alt (левый Alt)
    bool altPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
    if (altPressed && !altPressedLastFrame) {
        movement1 = !movement1;
        if (movement1 ) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true; // чтобы не было рывка при захвате
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    altPressedLastFrame = altPressed;

    // выход из приложения
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    // движение камеры

    if (movement1) {
        float cameraSpeed = cameraGlobalSpeed * deltaTime;

        glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
        glm::vec3 right = glm::normalize(glm::cross(horizontalFront, cameraUp));

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += cameraSpeed * horizontalFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= cameraSpeed * horizontalFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= right * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += right * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            cameraPos += cameraUp * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            cameraPos -= cameraUp * cameraSpeed;
        
    }
    static bool saveKeyWasPressed = false;
    bool saveKeyPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
                       && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    if (saveKeyPressed && !saveKeyWasPressed)
        saveSceneConfig("scene.cfg");
    saveKeyWasPressed = saveKeyPressed;

}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!movement1) {
        bool leftMousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool rightMousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        if (!leftMousePressed && !rightMousePressed) {
            firstMouse = true;
            return;
        }

        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
            return;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // перевернуто, потому что y-координаты идут сверху вниз
        lastX = xpos;
        lastY = ypos;

        if (leftMousePressed) {
            yaw += xoffset * 0.1f;
            pitch += yoffset * 0.1f;
            pitch = glm::clamp(pitch, -89.0f, 89.0f);
            updateOrbitCamera();
        } else if (rightMousePressed) {
            glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
            glm::vec3 up = glm::normalize(glm::cross(right, cameraFront));
            float panSpeed = orbitDistance * 0.0025f;
            glm::vec3 panOffset = (-right * xoffset - up * yoffset) * panSpeed;
            cameraCenter += panOffset;
            cameraPos += panOffset;
            cameraFront = glm::normalize(cameraCenter - cameraPos);
        }
    } else if (movement1) {
        static float sensitivity = 0.1f;
        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;

        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw   += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
}

void scroll_callback(GLFWwindow*, double, double yoffset)
{
    if (!movement1 && !ImGui::GetIO().WantCaptureMouse) {
        orbitDistance = glm::clamp(orbitDistance - static_cast<float>(yoffset) * 0.5f, 0.5f, 100.0f);
        updateOrbitCamera();
    }
}

void updateOrbitCamera()
{
    glm::vec3 offset;
    offset.x = orbitDistance * cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    offset.y = orbitDistance * sin(glm::radians(pitch));
    offset.z = orbitDistance * sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraPos = cameraCenter + offset;
    cameraFront = glm::normalize(cameraCenter - cameraPos);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}

void gui1(){
    ImGui::NewFrame();

    ImGui::Begin("Tool", &my_tool_active, ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::Text("Color:");
    ImGui::ColorEdit4("", my_color);


    // --- Папка с объектами ---
    if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (size_t i = 0; i < cubes.size(); ++i) {
            std::string label = "Object " + std::to_string(i);
            if (ImGui::TreeNode(label.c_str())) {
                Object& obj = cubes[i];
                ImGui::Text("ID: %zu", i);
                ImGui::Text("Name: %s", obj.name.c_str());
                if (!obj.meshPath.empty())
                    ImGui::Text("Mesh: %s", obj.meshPath.c_str());
                ImGui::Text("Texture: %s", obj.texturePath.c_str());

                // Position
                ImGui::Text("Position");
                ImGui::SameLine();
                for (int j = 0; j < 3; ++j) {
                    ImGui::PushID(j);
                    ImGui::SetNextItemWidth(60);
                    // Показываем только значение, редактируем колесиком мыши
                    ImGui::InputFloat("", &obj.position[j], 0.0f, 0.0f, "%.2f");
                    if (ImGui::IsItemHovered() && !ImGui::IsItemActive()) {
                        float wheel = ImGui::GetIO().MouseWheel;
                        if (wheel != 0.0f) {
                            obj.position[j] += 0.05f * wheel /1.5f;
                        }
                    }
                    ImGui::SameLine();
                    ImGui::PopID();
                }
                ImGui::NewLine();

                // Rotation
                ImGui::Text("Rotation");
                ImGui::SameLine();
                for (int j = 0; j < 3; ++j) {
                    ImGui::PushID(100 + j);
                    ImGui::SetNextItemWidth(60);
                    ImGui::InputFloat("", &obj.rotation[j], 0.0f, 0.0f, "%.2f");
                    if (ImGui::IsItemHovered() && !ImGui::IsItemActive()) {
                        float wheel = ImGui::GetIO().MouseWheel;
                        if (wheel != 0.0f) {
                            obj.rotation[j] += 1.0f * wheel / 1.5f;
                        }
                    }
                    ImGui::SameLine();
                    ImGui::PopID();
                }
                ImGui::NewLine();

                // Scale
                ImGui::Text("Scale");
                ImGui::SameLine();
                for (int j = 0; j < 3; ++j) {
                    ImGui::PushID(200 + j);
                    ImGui::SetNextItemWidth(60);
                    ImGui::InputFloat("", &obj.scale[j], 0.0f, 0.0f, "%.2f");
                    if (ImGui::IsItemHovered() && !ImGui::IsItemActive()) {
                        float wheel = ImGui::GetIO().MouseWheel;
                        if (wheel != 0.0f) {
                            obj.scale[j] += 0.05f * wheel /1.5f;
                        }
                    }
                    ImGui::SameLine();
                    ImGui::PopID();
                }
                ImGui::NewLine();

                ImGui::Text("Texture ID: %u", obj.textureID);
                ImGui::TreePop();
            }
        }
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1,1,0,1), "Important Stuff");

    ImGui::End();
}