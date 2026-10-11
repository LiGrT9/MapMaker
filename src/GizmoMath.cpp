#include "GizmoMath.h"
#include <algorithm>

Matrix4 Matrix4::Identity() { Matrix4 m; m.value[0]=m.value[5]=m.value[10]=m.value[15]=1.0f; return m; }
Matrix4 Matrix4::Translation(const Vector3& v) { Matrix4 m=Identity(); m.value[12]=v.x; m.value[13]=v.y; m.value[14]=v.z; return m; }
Matrix4 Matrix4::Scale(const Vector3& v) { Matrix4 m=Identity(); m.value[0]=v.x; m.value[5]=v.y; m.value[10]=v.z; return m; }
Matrix4 Matrix4::Rotation(const Quaternion& q)
{
    Matrix4 m=Identity(); const float xx=q.x*q.x, yy=q.y*q.y, zz=q.z*q.z;
    const float xy=q.x*q.y, xz=q.x*q.z, yz=q.y*q.z, wx=q.w*q.x, wy=q.w*q.y, wz=q.w*q.z;
    m.value[0]=1-2*(yy+zz); m.value[1]=2*(xy+wz); m.value[2]=2*(xz-wy);
    m.value[4]=2*(xy-wz); m.value[5]=1-2*(xx+zz); m.value[6]=2*(yz+wx);
    m.value[8]=2*(xz+wy); m.value[9]=2*(yz-wx); m.value[10]=1-2*(xx+yy); return m;
}
Matrix4 Matrix4::operator*(const Matrix4& rhs) const
{ Matrix4 result; for(int c=0;c<4;++c) for(int r=0;r<4;++r) for(int k=0;k<4;++k) result.value[c*4+r]+=value[k*4+r]*rhs.value[c*4+k]; return result; }
Vector3 Matrix4::TransformPoint(const Vector3& p) const
{ const float w=value[3]*p.x+value[7]*p.y+value[11]*p.z+value[15]; const float inv=std::fabs(w)>1e-6f?1.0f/w:1.0f; return {(value[0]*p.x+value[4]*p.y+value[8]*p.z+value[12])*inv,(value[1]*p.x+value[5]*p.y+value[9]*p.z+value[13])*inv,(value[2]*p.x+value[6]*p.y+value[10]*p.z+value[14])*inv}; }
Vector3 Matrix4::TransformVector(const Vector3& v) const { return {value[0]*v.x+value[4]*v.y+value[8]*v.z,value[1]*v.x+value[5]*v.y+value[9]*v.z,value[2]*v.x+value[6]*v.y+value[10]*v.z}; }
Matrix4 Matrix4::Inverse() const
{
    Matrix4 out; float aug[4][8]{}; for(int r=0;r<4;++r){for(int c=0;c<4;++c)aug[r][c]=value[c*4+r];aug[r][r+4]=1;}
    for(int c=0;c<4;++c){int pivot=c;for(int r=c+1;r<4;++r)if(std::fabs(aug[r][c])>std::fabs(aug[pivot][c]))pivot=r;if(std::fabs(aug[pivot][c])<1e-8f)return Identity();for(int j=0;j<8;++j)std::swap(aug[c][j],aug[pivot][j]);float d=aug[c][c];for(int j=0;j<8;++j)aug[c][j]/=d;for(int r=0;r<4;++r)if(r!=c){float f=aug[r][c];for(int j=0;j<8;++j)aug[r][j]-=f*aug[c][j];}}
    for(int r=0;r<4;++r) for(int c=0;c<4;++c) out.value[c*4+r]=aug[r][c+4];
    return out;
}
Quaternion Quaternion::FromAxisAngle(const Vector3& axis,float radians){const Vector3 n=Normalize(axis);const float s=std::sin(radians*0.5f);return {std::cos(radians*0.5f),n.x*s,n.y*s,n.z*s};}
Quaternion Quaternion::operator*(const Quaternion& q) const{return {w*q.w-x*q.x-y*q.y-z*q.z,w*q.x+x*q.w+y*q.z-z*q.y,w*q.y-x*q.z+y*q.w+z*q.x,w*q.z+x*q.y-y*q.x+z*q.w};}
Quaternion Quaternion::Normalized() const{const float n=std::sqrt(w*w+x*x+y*y+z*z);return n>1e-6f?Quaternion(w/n,x/n,y/n,z/n):Quaternion{};}
Vector3 Quaternion::Rotate(const Vector3& v) const{const Quaternion p(0,v.x,v.y,v.z), qn(w,-x,-y,-z), r=(*this)*p*qn;return {r.x,r.y,r.z};}
