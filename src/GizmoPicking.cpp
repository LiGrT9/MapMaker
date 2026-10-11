#include "GizmoPicking.h"
#include <limits>

Ray GizmoPicking::MakeRay(const GizmoCamera& camera, Vector2 mouse)
{
    const float x=2.0f*mouse.x/camera.viewport.x-1.0f, y=1.0f-2.0f*mouse.y/camera.viewport.y;
    const Matrix4 inverse=(camera.projection*camera.view).Inverse();
    const Vector3 farPoint=inverse.TransformPoint({x,y,1});
    return {camera.position,Normalize(farPoint-camera.position)};
}
static float SegmentDistanceSquared(const Ray& ray,const Vector3& a,const Vector3& b)
{const Vector3 u=ray.direction,v=b-a,w=ray.origin-a;const float aa=Dot(u,u),bb=Dot(v,v),ab=Dot(u,v);float s=0,t=0,d=aa*bb-ab*ab;if(d>1e-6f){s=Clamp((ab*Dot(v,w)-bb*Dot(u,w))/d,0,1);t=Clamp((aa*Dot(v,w)-ab*Dot(u,w))/d,0,1);}const Vector3 delta=ray.origin+u*s-(a+v*t);return LengthSquared(delta);}
GizmoAxis GizmoPicking::PickAxis(const Ray& ray,const Vector3& origin,float size,bool rings,const Quaternion& orientation)
{const Vector3 localAxes[3]={{1,0,0},{0,1,0},{0,0,1}};GizmoAxis best=GizmoAxis::None;float distance=std::numeric_limits<float>::max();for(int i=0;i<3;++i){const Vector3 axis=Normalize(orientation.Rotate(localAxes[i]));if(rings){const float denom=Dot(ray.direction,axis);if(std::fabs(denom)<1e-5f)continue;const Vector3 hit=ray.origin+ray.direction*(Dot(origin-ray.origin,axis)/denom);const float radial=Length(hit-origin);const float error=std::fabs(radial-size);if(error<0.12f*size&&error<distance){distance=error;best=static_cast<GizmoAxis>(i+1);}}else{const float error=SegmentDistanceSquared(ray,origin+axis*size*0.12f,origin+axis*size);if(error<0.06f*0.06f*size*size&&error<distance){distance=error;best=static_cast<GizmoAxis>(i+1);}}}return best;}
float GizmoPicking::PickSphere(const Ray& ray,const Vector3& center,float radius)
{const Vector3 offset=ray.origin-center;const float b=Dot(offset,ray.direction);const float c=Dot(offset,offset)-radius*radius;const float discriminant=b*b-c;if(discriminant<0.0f)return -1.0f;const float distance=-b-std::sqrt(discriminant);return distance>=0.0f?distance:0.0f;}
float GizmoPicking::AxisParameterOnPlane(const Ray& ray,const Vector3& origin,const Vector3& axis,const Vector3& planeNormal,float fallback)
{const Vector3 normal=Normalize(planeNormal);const float denom=Dot(ray.direction,normal);if(std::fabs(denom)<1e-5f)return fallback;const Vector3 point=ray.origin+ray.direction*(Dot(origin-ray.origin,normal)/denom);return Dot(point-origin,Normalize(axis));}
float GizmoPicking::RingAngle(const Ray& ray,const Vector3& origin,const Vector3& axis,const Vector3& reference)
{const Vector3 n=Normalize(axis);const float denom=Dot(ray.direction,n);if(std::fabs(denom)<1e-5f)return 0;const Vector3 hit=ray.origin+ray.direction*(Dot(origin-ray.origin,n)/denom);const Vector3 radial=Normalize(hit-origin);const Vector3 a=Normalize(reference-n*Dot(reference,n));const Vector3 b=Normalize(Cross(n,a));return std::atan2(Dot(radial,b),Dot(radial,a));}
