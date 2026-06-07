#include "objects.h"
#include <glm/gtc/matrix_transform.hpp>

Object::Object(const std::vector<float>& verts, unsigned int texID,
           const glm::vec3& pos,
           const glm::vec3& rot,
           const glm::vec3& scl)
    : vertices(verts), textureID(texID), position(pos), rotation(rot), scale(scl) {}

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