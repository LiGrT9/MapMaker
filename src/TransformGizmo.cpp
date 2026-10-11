#include "TransformGizmo.h"
#include "GizmoRenderer.h"
#include <algorithm>
#include <initializer_list>

Vector3 TransformGizmo::Axis(GizmoAxis axis) const
{
    Vector3 result = axis == GizmoAxis::X ? Vector3{1,0,0} : axis == GizmoAxis::Y ? Vector3{0,1,0} : Vector3{0,0,1};
    return space_ == GizmoSpace::Local ? state_.rotation.Rotate(result) : result;
}

void MoveGizmo::Update(const GizmoCamera& camera, Vector2 mouse, bool leftButton)
{
    camera_=camera; mouse_=mouse; const Ray ray=GizmoPicking::MakeRay(camera,mouse); const float size=std::max(0.5f,Length(camera.position-state_.position)*0.18f);
    if(!dragging_) hovered_=GizmoPicking::PickAxis(ray,state_.position,size,false,state_.rotation);
    if(leftButton&&!dragging_&&hovered_!=GizmoAxis::None)BeginDrag(ray);
    if (leftButton && dragging_) Drag(ray);
    if (!leftButton && dragging_) EndDrag();
}
void MoveGizmo::BeginDrag(const Ray& ray)
{ selected_=hovered_; dragging_=selected_!=GizmoAxis::None; if(!dragging_)return; dragOrigin_=state_.position;dragAxis_=Normalize(Axis(selected_)); const Vector3 view=Normalize(camera_.position-state_.position);dragPlaneNormal_=Normalize(Cross(dragAxis_,Cross(view,dragAxis_)));if(LengthSquared(dragPlaneNormal_)<1e-6f)dragPlaneNormal_=Normalize(Cross(dragAxis_,Vector3{0,1,0}));dragStartParameter_=GizmoPicking::AxisParameterOnPlane(ray,dragOrigin_,dragAxis_,dragPlaneNormal_,0); }
void MoveGizmo::Drag(const Ray& ray){const float parameter=GizmoPicking::AxisParameterOnPlane(ray,dragOrigin_,dragAxis_,dragPlaneNormal_,dragStartParameter_);state_.position=dragOrigin_+dragAxis_*(parameter-dragStartParameter_);}
void MoveGizmo::EndDrag(){dragging_=false;selected_=GizmoAxis::None;}
void MoveGizmo::Render(GizmoRenderer& renderer,const GizmoCamera& camera) const{renderer.Begin(camera,state_.position,std::max(0.5f,Length(camera.position-state_.position)*0.18f),state_.rotation);for(GizmoAxis axis:{GizmoAxis::X,GizmoAxis::Y,GizmoAxis::Z})renderer.DrawMoveAxis(axis,axis==hovered_||axis==selected_);renderer.End();}

void RotateGizmo::Update(const GizmoCamera& camera, Vector2 mouse, bool leftButton)
{camera_=camera;mouse_=mouse;const Ray ray=GizmoPicking::MakeRay(camera,mouse);const float size=std::max(0.6f,Length(camera.position-state_.position)*0.22f);if(!dragging_)hovered_=GizmoPicking::PickAxis(ray,state_.position,size,true,state_.rotation);if(leftButton&&!dragging_&&hovered_!=GizmoAxis::None)BeginDrag(ray);if(leftButton&&dragging_)Drag(ray);if(!leftButton&&dragging_)EndDrag();}
void RotateGizmo::BeginDrag(const Ray& ray){selected_=hovered_;dragging_=selected_!=GizmoAxis::None;if(!dragging_)return;dragAxis_=Normalize(Axis(selected_));dragReference_=std::fabs(Dot(dragAxis_,Vector3{0,1,0}))<0.9f?Vector3{0,1,0}:Vector3{1,0,0};dragStartAngle_=GizmoPicking::RingAngle(ray,state_.position,dragAxis_,dragReference_);dragStartRotation_=state_.rotation;}
void RotateGizmo::Drag(const Ray& ray){const float angle=GizmoPicking::RingAngle(ray,state_.position,dragAxis_,dragReference_)-dragStartAngle_;state_.rotation=Quaternion::FromAxisAngle(dragAxis_,angle)*dragStartRotation_;}
void RotateGizmo::EndDrag(){dragging_=false;selected_=GizmoAxis::None;}
void RotateGizmo::Render(GizmoRenderer& renderer,const GizmoCamera& camera) const{renderer.Begin(camera,state_.position,std::max(0.6f,Length(camera.position-state_.position)*0.22f),state_.rotation);for(GizmoAxis axis:{GizmoAxis::X,GizmoAxis::Y,GizmoAxis::Z})renderer.DrawRotateRing(axis,axis==hovered_||axis==selected_);renderer.End();}
