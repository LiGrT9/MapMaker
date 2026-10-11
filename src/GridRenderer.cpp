#include "GridRenderer.h"

#include <glad/glad.h>

#include <cmath>
#include <iostream>
#include <utility>

namespace
{
const char* gridVertexShader = R"GLSL(
#version 330 core
out vec2 screenPosition;

void main()
{
    // One oversized triangle covers the complete viewport without a VBO.
    const vec2 positions[3] = vec2[3](
        vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    screenPosition = positions[gl_VertexID];
    gl_Position = vec4(screenPosition, 0.0, 1.0);
}
)GLSL";

const char* gridFragmentShader = R"GLSL(
#version 330 core
in vec2 screenPosition;
out vec4 fragmentColor;

uniform mat4 inverseViewProjection;
uniform vec3 cameraPosition;

float gridLine(vec2 coordinate, float spacing, float width)
{
    // fwidth keeps a line about one pixel wide at any camera distance.
    vec2 cell = abs(fract(coordinate / spacing - 0.5) - 0.5);
    vec2 derivative = max(fwidth(coordinate / spacing), vec2(0.0001));
    vec2 line = 1.0 - smoothstep(vec2(0.0), derivative * width, cell);
    return max(line.x, line.y);
}

void main()
{
    // Unproject two points on this pixel's ray, then intersect it with Y=0.
    vec4 nearPoint = inverseViewProjection * vec4(screenPosition, -1.0, 1.0);
    vec4 farPoint = inverseViewProjection * vec4(screenPosition, 1.0, 1.0);
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;
    vec3 rayOrigin = cameraPosition;
    vec3 rayDirection = normalize(farPoint.xyz - nearPoint.xyz);
    if (abs(rayDirection.y) < 0.00001)
        discard;

    float distanceToPlane = -rayOrigin.y / rayDirection.y;
    if (distanceToPlane < 0.0)
        discard;

    vec3 worldPosition = rayOrigin + rayDirection * distanceToPlane;
    vec2 gridCoordinate = worldPosition.xz;
    float minor = gridLine(gridCoordinate, 1.0, 1.0);
    float major = gridLine(gridCoordinate, 10.0, 1.35);
    float axisX = 1.0 - smoothstep(0.0, max(fwidth(worldPosition.z) * 1.5, 0.001), abs(worldPosition.z));
    float axisZ = 1.0 - smoothstep(0.0, max(fwidth(worldPosition.x) * 1.5, 0.001), abs(worldPosition.x));

    vec3 color = vec3(0.17, 0.19, 0.22) * minor;
    color = mix(color, vec3(0.31, 0.33, 0.37), major);
    color = mix(color, vec3(0.82, 0.12, 0.10), axisX);
    color = mix(color, vec3(0.10, 0.30, 0.86), axisZ);

    // Do not fade by world distance: the ray-plane intersection itself provides
    // the infinite extent, while fwidth keeps distant cells readable.
    float alpha = max(max(minor * 0.48, major * 0.72), max(axisX, axisZ) * 0.95);
    if (alpha < 0.01)
        discard;
    fragmentColor = vec4(color, alpha);
}
)GLSL";

unsigned int compileShader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Grid shader compilation failed: " << log << std::endl;
    }
    return shader;
}
}

void GridRenderer::Initialize()
{
    const unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, gridVertexShader);
    const unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, gridFragmentShader);
    program_ = glCreateProgram();
    glAttachShader(program_, vertexShader);
    glAttachShader(program_, fragmentShader);
    glLinkProgram(program_);
    int success = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024] = {};
        glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        std::cerr << "Grid shader linking failed: " << log << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glGenVertexArrays(1, &vertexArray_);
    inverseViewProjectionLocation_ = glGetUniformLocation(program_, "inverseViewProjection");
    cameraPositionLocation_ = glGetUniformLocation(program_, "cameraPosition");
}

void GridRenderer::Render(const Camera& camera)
{
    if (program_ == 0 || vertexArray_ == 0)
        return;

    // Invert projection*view with Gauss-Jordan. The grid module remains independent of GLM.
    float augmented[4][8] = {};
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            float value = 0.0f;
            for (int index = 0; index < 4; ++index)
                value += camera.projection.value[index * 4 + row] * camera.view.value[column * 4 + index];
            augmented[row][column] = value;
        }
        augmented[row][row + 4] = 1.0f;
    }
    for (int column = 0; column < 4; ++column) {
        int pivot = column;
        for (int row = column + 1; row < 4; ++row)
            if (std::fabs(augmented[row][column]) > std::fabs(augmented[pivot][column])) pivot = row;
        for (int value = 0; value < 8; ++value) std::swap(augmented[column][value], augmented[pivot][value]);
        const float divisor = augmented[column][column];
        if (std::fabs(divisor) < 0.000001f)
            return;
        for (int value = 0; value < 8; ++value) augmented[column][value] /= divisor;
        for (int row = 0; row < 4; ++row) {
            if (row == column) continue;
            const float factor = augmented[row][column];
            for (int value = 0; value < 8; ++value) augmented[row][value] -= factor * augmented[column][value];
        }
    }
    float inverse[16] = {};
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 4; ++column)
            inverse[column * 4 + row] = augmented[row][column + 4];

    glUseProgram(program_);
    glUniformMatrix4fv(inverseViewProjectionLocation_, 1, GL_FALSE, inverse);
    glUniform3f(cameraPositionLocation_, camera.position.x, camera.position.y, camera.position.z);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(vertexArray_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void GridRenderer::Shutdown()
{
    if (vertexArray_ != 0) glDeleteVertexArrays(1, &vertexArray_);
    if (program_ != 0) glDeleteProgram(program_);
    vertexArray_ = 0;
    program_ = 0;
}