#pragma once
#include "GizmoMath.h"

enum class GizmoAxis { None, X, Y, Z };
struct Ray { Vector3 origin; Vector3 direction; };
struct GizmoCamera { Vector3 position; Matrix4 view; Matrix4 projection; Vector2 viewport; };

class GizmoPicking
{
public:
    static Ray MakeRay(const GizmoCamera& camera, Vector2 mouse);
    static GizmoAxis PickAxis(const Ray& ray, const Vector3& origin, float size, bool rings,
                              const Quaternion& orientation = Quaternion{});
    static float PickSphere(const Ray& ray, const Vector3& center, float radius);
    static float AxisParameterOnPlane(const Ray& ray, const Vector3& origin, const Vector3& axis, const Vector3& planeNormal, float fallback);
    static float RingAngle(const Ray& ray, const Vector3& origin, const Vector3& axis, const Vector3& reference);
};
