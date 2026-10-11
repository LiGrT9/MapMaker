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
#include <GridRenderer.h>
#include <GizmoRenderer.h>
#include <TransformGizmo.h>

#include <fstream>
#include <filesystem>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <cmath>
#include <limits>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
#include <algorithm>
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void updateOrbitCamera();
void gui1();

unsigned int loadTexture(const std::string& path);
std::string resolveProjectPath(const std::filesystem::path& projectRoot, const std::string& relativePath);
std::string chooseFile(const char* title);
bool copyProjectAsset(const std::string& sourcePath, const std::string& folder,
                      std::string& projectPath, std::string& fullPath);
bool addObjectFromFiles();
bool changeObjectTexture(Object& object);
bool addObjectFromZip(const std::string& archivePath);

GridRenderer gridRenderer;
GizmoRenderer gizmoRenderer;
TransformState gizmoState;
MoveGizmo moveGizmo(gizmoState);
RotateGizmo rotateGizmo(gizmoState);
bool rotateTool = false;
int selectedObjectIndex = -1;
bool leftMouseWasDown = false;
bool mouseOverObject = false;
bool mouseOverGizmo = false;
bool cameraDragAllowed = false;
enum class MouseInteraction { None, SelectingObject, MovingObject, RotatingObject, OrbitingCamera };
MouseInteraction mouseInteraction = MouseInteraction::None;
double cursorX = 0.0;
double cursorY = 0.0;
double orbitLastX = 0.0;
double orbitLastY = 0.0;
constexpr float cameraRenderDistance = 100.0f;

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
std::string activeSceneConfigPath = "scene.cfg";

Quaternion objectQuaternion(const glm::vec3& rotation)
{
    const float cx = std::cos(glm::radians(rotation.x) * 0.5f);
    const float sx = std::sin(glm::radians(rotation.x) * 0.5f);
    const float cy = std::cos(glm::radians(rotation.y) * 0.5f);
    const float sy = std::sin(glm::radians(rotation.y) * 0.5f);
    const float cz = std::cos(glm::radians(rotation.z) * 0.5f);
    const float sz = std::sin(glm::radians(rotation.z) * 0.5f);
    return Quaternion(cx * cy * cz + sx * sy * sz,
                      sx * cy * cz - cx * sy * sz,
                      cx * sy * cz + sx * cy * sz,
                      cx * cy * sz - sx * sy * cz).Normalized();
}

float objectPickRadius(const Object& object)
{
    float radius = 0.25f;
    for (size_t index = 0; index + 2 < object.vertices.size(); index += 5) {
        const Vector3 vertex{object.vertices[index], object.vertices[index + 1], object.vertices[index + 2]};
        radius = std::max(radius, Length(vertex));
    }
    return radius * std::max(object.scale.x, std::max(object.scale.y, object.scale.z));
}

int pickObject(const Ray& ray, float maxDistance)
{
    int result = -1;
    float nearest = std::numeric_limits<float>::max();
    for (size_t index = 0; index < cubes.size(); ++index) {
        const Object& object = cubes[index];
        const float distance = GizmoPicking::PickSphere(
            ray, {object.position.x, object.position.y, object.position.z}, objectPickRadius(object));
        if (distance >= 0.0f && distance <= maxDistance && distance < nearest) {
            nearest = distance;
            result = static_cast<int>(index);
        }
    }
    return result;
}

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

        file << "# object <name> <x> <y> <z> <object_folder> "
            "<rotation_x> <rotation_y> <rotation_z> "
            "<scale_x> <scale_y> <scale_z>\n";

    file << std::fixed << std::setprecision(6);
    for (const Object& object : cubes) {
        file << "object " << (object.name.empty() ? "unnamed" : object.name) << ' '
             << object.position.x << ' ' << object.position.y << ' ' << object.position.z << ' '
             << object.meshPath << ' '
             << object.rotation.x << ' ' << object.rotation.y << ' ' << object.rotation.z << ' '
             << object.scale.x << ' ' << object.scale.y << ' ' << object.scale.z << '\n';
    }

    std::cout << "Scene saved to " << path << std::endl;
    return true;
}

bool isTextureFile(const std::filesystem::path& path)
{
    const std::string extension = path.extension().string();
    return extension == ".png" || extension == ".PNG" ||
           extension == ".jpg" || extension == ".JPG" ||
           extension == ".jpeg" || extension == ".JPEG" ||
           extension == ".bmp" || extension == ".BMP" ||
           extension == ".tga" || extension == ".TGA";
}

    bool loadObjectFolder(const std::filesystem::path& folderPath,
                          const std::filesystem::path& projectRoot,
                          std::vector<float>& vertices,
                          std::vector<std::string>& texturePaths,
                          std::vector<unsigned int>& textureIDs)
    {
        std::string error;
        if (!std::filesystem::is_directory(folderPath))
            return false;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(folderPath)) {
            if (!entry.is_regular_file())
                continue;

            const std::filesystem::path resourcePath = entry.path();
            const std::string extension = resourcePath.extension().string();
            if (extension == ".obj" || extension == ".OBJ") {
                std::vector<float> objectVertices;
                if (!loadObjMesh(resourcePath.string(), objectVertices, error)) {
                    std::cout << "Failed to load OBJ: " << error << std::endl;
                    return false;
                }
                vertices.insert(vertices.end(), objectVertices.begin(), objectVertices.end());
            } else if (isTextureFile(resourcePath)) {
                const unsigned int texture = loadTexture(resourcePath.string());
                if (texture == 0)
                    return false;
                texturePaths.push_back(std::filesystem::relative(resourcePath, projectRoot).generic_string());
                textureIDs.push_back(texture);
            }
        }
        return !vertices.empty() && !textureIDs.empty();
    }

bool loadSceneConfig(const std::string& path)
{
    cubes.clear();
    std::ifstream file(path);
    if (!file) {
        std::cout << "Scene config not found: " << path << std::endl;
        return false;
    }

    const std::filesystem::path projectRoot = std::filesystem::path(path).parent_path();
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
        std::string objectFolder;
        glm::vec3 position;
        glm::vec3 rotation(0.0f);
        glm::vec3 scale(1.0f);

        if (!(stream >> name >> position.x >> position.y >> position.z >> objectFolder)) {
            std::cout << "Invalid object at scene line " << lineNumber << std::endl;
            return false;
        }

        if (!(stream >> rotation.x >> rotation.y >> rotation.z)) {
            stream.clear();
        }
        if (!(stream >> scale.x >> scale.y >> scale.z)) {
            stream.clear();
        }
        const std::filesystem::path folderPath = resolveProjectPath(projectRoot, objectFolder);
        std::vector<float> vertices;
        std::vector<std::string> texturePaths;
        std::vector<unsigned int> textureIDs;
        std::string error;
        if (!std::filesystem::is_directory(folderPath)) {
            std::cout << "Object folder not found: " << folderPath << std::endl;
            cubes.clear();
            return false;
        }

        if (!loadObjectFolder(folderPath, projectRoot, vertices, texturePaths, textureIDs)) {
            std::cout << "Object folder must contain OBJ and texture files: " << folderPath << std::endl;
            cubes.clear();
            return false;
        }

        cubes.emplace_back(vertices, textureIDs.front(), position, rotation, scale,
                           name, objectFolder, texturePaths.front(), texturePaths, textureIDs);
    }

    return !cubes.empty();
}

std::string resolveProjectPath(const std::filesystem::path& projectRoot,
                               const std::string& relativePath)
{
    const std::filesystem::path path(relativePath);
    if (path.is_absolute())
        return path.string();

    const std::filesystem::path besideConfig = (projectRoot / path).lexically_normal();
    if (std::filesystem::exists(besideConfig))
        return besideConfig.string();

    const std::filesystem::path fromWorkingDirectory =
        (std::filesystem::current_path() / path).lexically_normal();
    if (std::filesystem::exists(fromWorkingDirectory))
        return fromWorkingDirectory.string();

    return besideConfig.string();
}

std::string chooseFile(const char* title)
{
    std::string command = "zenity --file-selection --title='" + std::string(title) + "' 2>/dev/null";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        command = "kdialog --getopenfilename 2>/dev/null";
        pipe = popen(command.c_str(), "r");
    }
    if (!pipe)
        return {};

    char buffer[512];
    std::string path;
    if (fgets(buffer, sizeof(buffer), pipe))
        path = buffer;
    pclose(pipe);

    while (!path.empty() && (path.back() == '\n' || path.back() == '\r'))
        path.pop_back();
    return path;
}

bool copyProjectAsset(const std::string& sourcePath, const std::string& folder,
                      std::string& projectPath, std::string& fullPath)
{
    const std::filesystem::path projectRoot =
        std::filesystem::absolute(activeSceneConfigPath).parent_path();
    const std::filesystem::path source = std::filesystem::absolute(sourcePath);
    const std::filesystem::path destinationFolder = projectRoot / folder;

    std::error_code error;
    std::filesystem::create_directories(destinationFolder, error);
    if (error)
        return false;

    std::string safeName = source.stem().string();
    for (char& character : safeName) {
        if (!std::isalnum(static_cast<unsigned char>(character)) &&
            character != '_' && character != '-')
            character = '_';
    }
    if (safeName.empty())
        safeName = "asset";

    std::filesystem::path destination = destinationFolder / (safeName + source.extension().string());
    for (int suffix = 1; std::filesystem::exists(destination); ++suffix)
        destination = destinationFolder / (safeName + "_" +
                                           std::to_string(suffix) + source.extension().string());

    std::filesystem::copy_file(source, destination,
                               std::filesystem::copy_options::none, error);
    if (error)
        return false;

    projectPath = std::filesystem::relative(destination, projectRoot, error).generic_string();
    if (error)
        return false;
    fullPath = destination.string();
    return true;
}

bool addObjectFromFiles()
{
    const std::string sourceObj = chooseFile("Select OBJ object");
    if (sourceObj.empty())
        return false;

    if (std::filesystem::path(sourceObj).extension() == ".zip" ||
        std::filesystem::path(sourceObj).extension() == ".ZIP")
        return addObjectFromZip(sourceObj);

    const std::string sourceTexture = chooseFile("Select object texture");
    if (sourceTexture.empty())
        return false;

    const std::filesystem::path sourceObject(sourceObj);
    std::string safeName = sourceObject.stem().string();
    const std::filesystem::path projectRoot =
        std::filesystem::absolute(activeSceneConfigPath).parent_path();
    const std::string objectFolder = "objects/" + safeName;
    const std::filesystem::path objectFolderPath = projectRoot / objectFolder;
    std::error_code copyError;
    std::filesystem::create_directories(objectFolderPath, copyError);
    if (copyError)
        return false;

    std::string copiedMeshPath;
    std::string meshFullPath;
    if (!copyProjectAsset(sourceObj, objectFolder, copiedMeshPath, meshFullPath))
        return false;

    std::string texturePath;
    std::string textureFullPath;
    if (!copyProjectAsset(sourceTexture, "objects/" + safeName, texturePath, textureFullPath))
        return false;

    std::vector<float> vertices;
    std::string error;
    std::vector<std::string> texturePaths;
    std::vector<unsigned int> textureIDs;
    if (!loadObjectFolder(objectFolderPath,
                          projectRoot,
                          vertices, texturePaths, textureIDs)) {
        std::cout << "Failed to load selected object folder." << std::endl;
        return false;
    }

    cubes.emplace_back(vertices, textureIDs.front(), glm::vec3(0.0f), glm::vec3(0.0f),
                       glm::vec3(1.0f), safeName, objectFolder,
                       texturePaths.front(), texturePaths, textureIDs);
    return true;
}

bool changeObjectTexture(Object& object)
{
    const std::string sourceTexture = chooseFile("Select object texture");
    if (sourceTexture.empty())
        return false;

    std::string texturePath;
    std::string textureFullPath;
    if (!copyProjectAsset(sourceTexture, object.meshPath, texturePath, textureFullPath))
        return false;

    unsigned int texture = loadTexture(textureFullPath);
    if (texture == 0)
        return false;

    object.textureID = texture;
    object.texturePath = texturePath;
    object.texturePaths.push_back(texturePath);
    object.textureIDs.push_back(texture);
    return true;
}

std::string shellQuote(const std::string& value)
{
    std::string quoted = "'";
    for (char character : value) {
        if (character == '\'')
            quoted += "'\\''";
        else
            quoted += character;
    }
    return quoted + "'";
}

bool addObjectFromZip(const std::string& archivePath)
{
    const std::filesystem::path archive = std::filesystem::absolute(archivePath);
    const std::filesystem::path projectRoot =
        std::filesystem::absolute(activeSceneConfigPath).parent_path();
    std::string safeName = archive.stem().string();
    for (char& character : safeName) {
        if (!std::isalnum(static_cast<unsigned char>(character)) &&
            character != '_' && character != '-')
            character = '_';
    }
    if (safeName.empty())
        safeName = "archive_object";

    const std::filesystem::path objectFolder = projectRoot / "objects" / safeName;
    const std::filesystem::path temporaryDirectory = objectFolder;

    std::error_code error;
    std::filesystem::create_directories(objectFolder, error);
    if (error)
        return false;

    const std::string unzipCommand = "unzip -q " + shellQuote(archive.string()) +
                                     " -d " + shellQuote(temporaryDirectory.string());
    if (std::system(unzipCommand.c_str()) != 0) {
        std::filesystem::remove_all(temporaryDirectory, error);
        std::cout << "Failed to extract ZIP archive: " << archive << std::endl;
        return false;
    }

    std::vector<float> vertices;
    std::vector<std::string> texturePaths;
    std::vector<unsigned int> textureIDs;
    if (!loadObjectFolder(objectFolder, projectRoot, vertices, texturePaths, textureIDs)) {
        std::cout << "ZIP must contain OBJ and texture files." << std::endl;
        std::filesystem::remove_all(objectFolder, error);
        return false;
    }

    const std::string objectFolderPath =
        std::filesystem::relative(objectFolder, projectRoot, error).generic_string();
    if (error) {
        std::filesystem::remove_all(objectFolder, error);
        return false;
    }

    cubes.emplace_back(vertices, textureIDs.front(), glm::vec3(0.0f), glm::vec3(0.0f),
                       glm::vec3(1.0f), safeName, objectFolderPath,
                       texturePaths.front(), texturePaths, textureIDs);
    std::filesystem::remove(archive, error);
    return true;
}


int main(int argc, char** argv)
{
    const std::string sceneConfigPath = argc > 1 ? argv[1] : "scene.cfg";
    const std::filesystem::path projectRoot = std::filesystem::path(sceneConfigPath).parent_path();
    activeSceneConfigPath = sceneConfigPath;
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
    gridRenderer.Initialize();
    gizmoRenderer.Initialize();

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



    // Set up the GPU buffer used by meshes loaded from the scene config.
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);



    stbi_set_flip_vertically_on_load(true);
    ourShader.use();
    for (int index = 0; index < 8; ++index)
        ourShader.setInt("textures[" + std::to_string(index) + "]", index);
    if (!loadSceneConfig(sceneConfigPath))
        return -1;
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
        // activate shader
        

        // create transformations
        glm::mat4 view          = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        glm::mat4 projection    = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        Camera gridCamera{
            {cameraPos.x, cameraPos.y, cameraPos.z}, {}, {},
            {static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT)}};
        std::copy(&view[0][0], &view[0][0] + 16, gridCamera.view.value);
        std::copy(&projection[0][0], &projection[0][0] + 16, gridCamera.projection.value);
        gridRenderer.Render(gridCamera);

        GizmoCamera gizmoCamera{{cameraPos.x, cameraPos.y, cameraPos.z}, {}, {},
                                {static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT)}};
        std::copy(&view[0][0], &view[0][0] + 16, gizmoCamera.view.value);
        std::copy(&projection[0][0], &projection[0][0] + 16, gizmoCamera.projection.value);
        const Vector2 mouse{static_cast<float>(cursorX), static_cast<float>(cursorY)};
        const Ray mouseRay = GizmoPicking::MakeRay(gizmoCamera, mouse);
        const bool leftMouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool controlHeld = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        const bool imguiCapturesMouse = ImGui::GetIO().WantCaptureMouse;

        const int hoveredObjectIndex = pickObject(mouseRay, cameraRenderDistance);
        mouseOverObject = hoveredObjectIndex >= 0;
        mouseOverGizmo = false;
        if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(cubes.size())) {
            const Object& selected = cubes[static_cast<size_t>(selectedObjectIndex)];
            const Vector3 position{selected.position.x, selected.position.y, selected.position.z};
            const Quaternion rotation = objectQuaternion(selected.rotation);
            const float gizmoSize = std::max(0.5f, Length(gizmoCamera.position - position) * 0.18f);
            const GizmoAxis moveAxis = GizmoPicking::PickAxis(mouseRay, position, gizmoSize, false, rotation);
            const GizmoAxis rotateAxis = GizmoPicking::PickAxis(mouseRay, position, gizmoSize * 1.22f, true, rotation);
            mouseOverGizmo = moveAxis != GizmoAxis::None || rotateAxis != GizmoAxis::None;

            if (leftMouseDown && !leftMouseWasDown && !imguiCapturesMouse) {
                if (controlHeld) {
                    mouseInteraction = MouseInteraction::OrbitingCamera;
                    orbitLastX = cursorX;
                    orbitLastY = cursorY;
                } else if (rotateAxis != GizmoAxis::None) {
                    rotateTool = true;
                    mouseInteraction = MouseInteraction::RotatingObject;
                } else if (moveAxis != GizmoAxis::None) {
                    rotateTool = false;
                    mouseInteraction = MouseInteraction::MovingObject;
                } else if (hoveredObjectIndex >= 0) {
                    selectedObjectIndex = hoveredObjectIndex;
                    mouseInteraction = MouseInteraction::SelectingObject;
                } else {
                    mouseInteraction = MouseInteraction::OrbitingCamera;
                    orbitLastX = cursorX;
                    orbitLastY = cursorY;
                }
            }
        } else if (leftMouseDown && !leftMouseWasDown && !imguiCapturesMouse) {
            mouseInteraction = controlHeld ? MouseInteraction::OrbitingCamera
                                            : hoveredObjectIndex >= 0
                                                ? MouseInteraction::SelectingObject
                                                : MouseInteraction::OrbitingCamera;
            if (mouseInteraction == MouseInteraction::OrbitingCamera) {
                orbitLastX = cursorX;
                orbitLastY = cursorY;
            } else {
                selectedObjectIndex = hoveredObjectIndex;
            }
        }

        if (!leftMouseDown && leftMouseWasDown) {
            if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(cubes.size())) {
                TransformGizmo& activeGizmo = rotateTool ? static_cast<TransformGizmo&>(rotateGizmo)
                                                          : static_cast<TransformGizmo&>(moveGizmo);
                if (mouseInteraction == MouseInteraction::MovingObject ||
                    mouseInteraction == MouseInteraction::RotatingObject)
                    activeGizmo.Update(gizmoCamera, mouse, false);
            }
            mouseInteraction = MouseInteraction::None;
        }

        if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(cubes.size())) {
            Object& gizmoObject = cubes[static_cast<size_t>(selectedObjectIndex)];
            const bool gizmoDragging = moveGizmo.IsDragging() || rotateGizmo.IsDragging();
            if (!gizmoDragging) {
                gizmoState.position = {gizmoObject.position.x, gizmoObject.position.y, gizmoObject.position.z};
                gizmoState.rotation = objectQuaternion(gizmoObject.rotation);
            }
            TransformGizmo& activeGizmo = rotateTool ? static_cast<TransformGizmo&>(rotateGizmo)
                                                      : static_cast<TransformGizmo&>(moveGizmo);
            const bool gizmoDrag = leftMouseDown &&
                                   (mouseInteraction == MouseInteraction::MovingObject ||
                                    mouseInteraction == MouseInteraction::RotatingObject);
            activeGizmo.Update(gizmoCamera, mouse, gizmoDrag);
            gizmoObject.position = glm::vec3(gizmoState.position.x, gizmoState.position.y, gizmoState.position.z);
            if (rotateTool) {
                const Quaternion& q = gizmoState.rotation;
                const float pitch = std::asin(Clamp(2.0f * (q.w * q.y - q.z * q.x), -1.0f, 1.0f));
                gizmoObject.rotation = glm::vec3(
                    glm::degrees(std::atan2(2.0f * (q.w * q.x + q.y * q.z), 1.0f - 2.0f * (q.x*q.x + q.y*q.y))),
                    glm::degrees(pitch),
                    glm::degrees(std::atan2(2.0f * (q.w * q.z + q.x * q.y), 1.0f - 2.0f * (q.y*q.y + q.z*q.z))));
            }
        }
        leftMouseWasDown = leftMouseDown;
        
        ourShader.use();
        // pass transformation matrices to the shader
        ourShader.setMat4("projection", projection); // note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
        ourShader.setMat4("view", view);


        int uni_Loc = glGetUniformLocation(ourShader.ID, "screenRatio");
        glUniform1f(uni_Loc, float(SCR_WIDTH) / float(SCR_HEIGHT)); 

        // render boxes
        glBindVertexArray(VAO);
        for (const auto& cube : cubes)
        {
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER,
                         cube.vertices.size() * sizeof(float),
                         cube.vertices.data(),
                         GL_DYNAMIC_DRAW);
            const int textureCount = std::min(static_cast<int>(cube.textureIDs.size()), 8);
            for (int index = 0; index < textureCount; ++index) {
                glActiveTexture(GL_TEXTURE0 + index);
                glBindTexture(GL_TEXTURE_2D, cube.textureIDs[index]);
            }
            ourShader.setInt("textureCount", textureCount);
            ourShader.setMat4("model", cube.getModelMatrix());
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(cube.vertices.size() / 5));
        }
        if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(cubes.size())) {
            TransformGizmo& activeGizmo = rotateTool ? static_cast<TransformGizmo&>(rotateGizmo)
                                                      : static_cast<TransformGizmo&>(moveGizmo);
            activeGizmo.Render(gizmoRenderer, { {cameraPos.x, cameraPos.y, cameraPos.z},
                                                gridCamera.view, gridCamera.projection,
                                                {static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT)} });
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
    gridRenderer.Shutdown();
    gizmoRenderer.Shutdown();

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
        saveSceneConfig(activeSceneConfigPath);
    saveKeyWasPressed = saveKeyPressed;

}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    cursorX = xpos;
    cursorY = ypos;

    if (!movement1) {
        const bool leftMousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool rightMousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        if (!leftMousePressed && !rightMousePressed) {
            firstMouse = true;
            return;
        }

        if (leftMousePressed && mouseInteraction == MouseInteraction::OrbitingCamera &&
            !ImGui::GetIO().WantCaptureMouse) {
            const float xoffset = static_cast<float>(xpos - orbitLastX);
            const float yoffset = static_cast<float>(orbitLastY - ypos);
            orbitLastX = xpos;
            orbitLastY = ypos;
            yaw += xoffset * 0.1f;
            pitch += yoffset * 0.1f;
            pitch = glm::clamp(pitch, -89.0f, 89.0f);
            updateOrbitCamera();
        } else if (rightMousePressed && cameraDragAllowed && !ImGui::GetIO().WantCaptureMouse) {
            if (firstMouse) {
                lastX = xpos;
                lastY = ypos;
                firstMouse = false;
                return;
            }
            const float xoffset = xpos - lastX;
            const float yoffset = lastY - ypos;
            lastX = xpos;
            lastY = ypos;
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

    ImGui::Text("Select an object, then drag an axis or rotation ring");
    if (ImGui::Button("Add Object"))
        addObjectFromFiles();


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
                ImGui::Text("Object folder: %s", obj.meshPath.c_str());
                ImGui::Text("Textures: %zu", obj.texturePaths.size());
                ImGui::PushID("texture");
                if (ImGui::Button("Choose Texture"))
                    changeObjectTexture(obj);
                ImGui::PopID();

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