#pragma once

#include "GizmoPicking.h"
#include <memory>

struct TransformState
{
    Vector3 position;
    Quaternion rotation;
    Vector3 scale{1.0f, 1.0f, 1.0f};
};

enum class GizmoSpace { Global, Local };

class GizmoRenderer;

class TransformGizmo
{
public:
    explicit TransformGizmo(TransformState& state) : state_(state) {}
    virtual ~TransformGizmo() = default;
    virtual void Update(const GizmoCamera& camera, Vector2 mouse, bool leftButton) = 0;
    virtual void Render(GizmoRenderer& renderer, const GizmoCamera& camera) const = 0;
    virtual void BeginDrag(const Ray& ray) = 0;
    virtual void Drag(const Ray& ray) = 0;
    virtual void EndDrag() = 0;
    void Update() {}
    void Render() const {}
    GizmoAxis HoveredAxis() const { return hovered_; }
    GizmoAxis SelectedAxis() const { return selected_; }
    void SetSpace(GizmoSpace space) { space_ = space; }
    GizmoSpace Space() const { return space_; }
    bool IsDragging() const { return dragging_; }

protected:
    Vector3 Axis(GizmoAxis axis) const;
    TransformState& state_;
    GizmoSpace space_ = GizmoSpace::Global;
    GizmoAxis hovered_ = GizmoAxis::None;
    GizmoAxis selected_ = GizmoAxis::None;
    bool dragging_ = false;
    GizmoCamera camera_{};
    Vector2 mouse_{};
};

class MoveGizmo final : public TransformGizmo
{
public:
    using TransformGizmo::TransformGizmo;
    void Update(const GizmoCamera& camera, Vector2 mouse, bool leftButton) override;
    void Render(GizmoRenderer& renderer, const GizmoCamera& camera) const override;
    void BeginDrag(const Ray& ray) override;
    void Drag(const Ray& ray) override;
    void EndDrag() override;
private:
    Vector3 dragOrigin_{};
    Vector3 dragAxis_{};
    Vector3 dragPlaneNormal_{};
    float dragStartParameter_ = 0.0f;
};

class RotateGizmo final : public TransformGizmo
{
public:
    using TransformGizmo::TransformGizmo;
    void Update(const GizmoCamera& camera, Vector2 mouse, bool leftButton) override;
    void Render(GizmoRenderer& renderer, const GizmoCamera& camera) const override;
    void BeginDrag(const Ray& ray) override;
    void Drag(const Ray& ray) override;
    void EndDrag() override;
private:
    Vector3 dragAxis_{};
    Vector3 dragReference_{};
    float dragStartAngle_ = 0.0f;
    Quaternion dragStartRotation_{};
};
