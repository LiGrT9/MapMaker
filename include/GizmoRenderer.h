#pragma once

#include "GizmoPicking.h"
#include <vector>

class GizmoRenderer
{
public:
    void Initialize();
    void Shutdown();
    void Begin(const GizmoCamera& camera, const Vector3& origin, float size,
               const Quaternion& orientation = Quaternion{});
    void DrawMoveAxis(GizmoAxis axis, bool highlighted);
    void DrawRotateRing(GizmoAxis axis, bool highlighted);
    void End();
private:
    unsigned int program_ = 0;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    int viewProjectionLocation_ = -1;
    int colorLocation_ = -1;
    Matrix4 viewProjection_{};
    Vector3 origin_{};
    Quaternion orientation_{};
    float size_ = 1.0f;
    void Draw(const std::vector<Vector3>& vertices, const Vector3& color, float width, bool loop);
};
