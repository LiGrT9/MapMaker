#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

bool loadObjMesh(const std::string& path, std::vector<float>& vertices, std::string& error);

class Object {
public:
    std::string name;
    std::string meshPath;
    std::string texturePath;
    std::vector<float> vertices;
    unsigned int textureID;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    Object(const std::vector<float>& verts, unsigned int texID,
           const glm::vec3& pos = glm::vec3(0.0f),
           const glm::vec3& rot = glm::vec3(0.0f),
            const glm::vec3& scl = glm::vec3(1.0f),
            const std::string& objectName = "unnamed",
               const std::string& sourcePath = "",
               const std::string& sourceTexturePath = "");

    void setPosition(const glm::vec3& pos);
    void setRotation(const glm::vec3& rot);
    void setScale(const glm::vec3& scl);

    glm::mat4 getModelMatrix() const;

    void moveX(float delta);
    void rotateY(float angle);
    void scaleAll(float factor);
};