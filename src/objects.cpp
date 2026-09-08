#include "objects.h"
#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <sstream>

namespace {
int resolveObjIndex(int index, int count)
{
    return index > 0 ? index - 1 : count + index;
}
}

bool loadObjMesh(const std::string& path, std::vector<float>& vertices, std::string& error)
{
    std::ifstream file(path);
    if (!file) {
        error = "cannot open " + path;
        return false;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    vertices.clear();

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        std::istringstream stream(line);
        std::string type;
        stream >> type;

        if (type.empty() || type[0] == '#')
            continue;

        if (type == "v") {
            glm::vec3 position;
            if (!(stream >> position.x >> position.y >> position.z)) {
                error = path + ": invalid vertex at line " + std::to_string(lineNumber);
                return false;
            }
            positions.push_back(position);
        } else if (type == "vt") {
            glm::vec2 texCoord;
            if (!(stream >> texCoord.x >> texCoord.y)) {
                error = path + ": invalid texture coordinate at line " + std::to_string(lineNumber);
                return false;
            }
            texCoords.push_back(texCoord);
        } else if (type == "f") {
            std::vector<std::string> face;
            std::string vertex;
            while (stream >> vertex)
                face.push_back(vertex);

            if (face.size() < 3) {
                error = path + ": face has fewer than 3 vertices at line " + std::to_string(lineNumber);
                return false;
            }

            for (size_t i = 1; i + 1 < face.size(); ++i) {
                const std::string triangle[] = {face[0], face[i], face[i + 1]};
                for (const std::string& descriptor : triangle) {
                    std::istringstream descriptorStream(descriptor);
                    std::string positionIndexText;
                    std::string texCoordIndexText;
                    std::getline(descriptorStream, positionIndexText, '/');
                    std::getline(descriptorStream, texCoordIndexText, '/');

                    if (positionIndexText.empty()) {
                        error = path + ": invalid face at line " + std::to_string(lineNumber);
                        return false;
                    }

                    int positionIndex = 0;
                    try {
                        positionIndex = std::stoi(positionIndexText);
                    } catch (...) {
                        error = path + ": invalid face vertex at line " + std::to_string(lineNumber);
                        return false;
                    }

                    positionIndex = resolveObjIndex(positionIndex, static_cast<int>(positions.size()));
                    if (positionIndex < 0 || positionIndex >= static_cast<int>(positions.size())) {
                        error = path + ": face references missing vertex at line " + std::to_string(lineNumber);
                        return false;
                    }

                    glm::vec2 texCoord(0.0f);
                    if (!texCoordIndexText.empty()) {
                        int texCoordIndex = 0;
                        try {
                            texCoordIndex = std::stoi(texCoordIndexText);
                        } catch (...) {
                            error = path + ": invalid face texture coordinate at line " + std::to_string(lineNumber);
                            return false;
                        }
                        texCoordIndex = resolveObjIndex(texCoordIndex, static_cast<int>(texCoords.size()));
                        if (texCoordIndex < 0 || texCoordIndex >= static_cast<int>(texCoords.size())) {
                            error = path + ": face references missing texture coordinate at line " + std::to_string(lineNumber);
                            return false;
                        }
                        texCoord = texCoords[texCoordIndex];
                    }

                    const glm::vec3& position = positions[positionIndex];
                    vertices.insert(vertices.end(), {position.x, position.y, position.z, texCoord.x, texCoord.y});
                }
            }
        }
    }

    if (vertices.empty()) {
        error = path + ": no faces found";
        return false;
    }
    return true;
}

Object::Object(const std::vector<float>& verts, unsigned int texID,
           const glm::vec3& pos,
           const glm::vec3& rot,
           const glm::vec3& scl,
           const std::string& objectName,
                     const std::string& sourcePath,
                     const std::string& sourceTexturePath)
        : name(objectName), meshPath(sourcePath), texturePath(sourceTexturePath),
            vertices(verts), textureID(texID),
      position(pos), rotation(rot), scale(scl) {}

void Object::setPosition(const glm::vec3& pos) {
    position = pos;
}

void Object::setRotation(const glm::vec3& rot) {
    rotation = rot;
}

void Object::setScale(const glm::vec3& scl) {
    scale = scl;
}

glm::mat4 Object::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1,0,0));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0,1,0));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0,0,1));
    model = glm::scale(model, scale);
    return model;
}

void Object::moveX(float delta) {
    position.x += delta;
}

void Object::rotateY(float angle) {
    rotation.y += angle;
}

void Object::scaleAll(float factor) {
    scale *= factor;
}