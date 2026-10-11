#pragma once

#include "GizmoMath.h"

struct Camera
{
    Vector3 position;
    Matrix4 view;
    Matrix4 projection;
    Vector2 viewport;
};

class GridRenderer
{
public:
    void Initialize();
    void Render(const Camera& camera);
    void Shutdown();

private:
    unsigned int program_ = 0;
    unsigned int vertexArray_ = 0;
    int inverseViewProjectionLocation_ = -1;
    int cameraPositionLocation_ = -1;
    int viewportLocation_ = -1;
};