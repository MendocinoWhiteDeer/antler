#pragma once
#include <math.h>


// In the following operations, functions that take in two vector/matrix pointers as input CAN NOT point to the same address

static inline float atlrLerp(const float f1, const float f2, const float L) { return f1 + L * (f2 - f1); }

typedef union _AtlrVec2
{
  struct {float x, y; };
  float arr[2];
  
} AtlrVec2;

typedef union _AtlrVec3
{
  struct {float x, y, z; };
  float arr[3];
  
} AtlrVec3;

typedef union _AtlrVec4
{
  struct {float x, y, z, w; };
  float arr[4];
  
} AtlrVec4;

typedef union _AtlrMat2
{
  struct { float xx, yx, xy, yy; };
  float arr[2][2];
  struct { AtlrVec2 xCol, yCol; };
  AtlrVec2 cols[2];

} AtlrMat2;

typedef union _AtlrMat3
{
  struct { float xx, yx, zx, xy, yy, zy, xz, yz, zz; };
  float arr[3][3];
  struct { AtlrVec3 xCol, yCol, zCol; };
  AtlrVec3 cols[3];

} AtlrMat3;

typedef union _AtlrMat4
{
  struct { float xx, yx, zx, wx, xy, yy, zy, wy, xz, yz, zz, wz, xw, yw, zw, ww; };
  float arr[4][4];
  struct { AtlrVec4 xCol, yCol, zCol, wCol; };
  AtlrVec4 cols[4];

} AtlrMat4;

// Rigid body transformation with scaling. The mapping order is scale -> rotate -> translate.
// The rotation is represented by a unit quaternion.
typedef struct _AtlrNodeTransform
{
  AtlrVec3 scale;
  AtlrVec4 rotate;
  AtlrVec3 translate;
  
} AtlrNodeTransform;

static inline AtlrVec2 atlrVec2Add(const AtlrVec2* restrict v1, const AtlrVec2* restrict v2)   { return (AtlrVec2){{v1->x + v2->x, v1->y + v2->y}}; }
static inline AtlrVec3 atlrVec3Add(const AtlrVec3* restrict v1, const AtlrVec3* restrict v2)   { return (AtlrVec3){{v1->x + v2->x, v1->y + v2->y, v1->z + v2->z}}; }
static inline AtlrVec4 atlrVec4Add(const AtlrVec4* restrict v1, const AtlrVec4* restrict v2)   { return (AtlrVec4){{v1->x + v2->x, v1->y + v2->y, v1->z + v2->z, v1->w + v2->w}}; }
static inline AtlrVec2 atlrVec2Sub(const AtlrVec2* restrict v1, const AtlrVec2* restrict v2)   { return (AtlrVec2){{v1->x - v2->x, v1->y - v2->y}}; }
static inline AtlrVec3 atlrVec3Sub(const AtlrVec3* restrict v1, const AtlrVec3* restrict v2)   { return (AtlrVec3){{v1->x - v2->x, v1->y - v2->y, v1->z - v2->z}}; }
static inline AtlrVec4 atlrVec4Sub(const AtlrVec4* restrict v1, const AtlrVec4* restrict v2)   { return (AtlrVec4){{v1->x - v2->x, v1->y - v2->y, v1->z - v2->z, v1->w - v2->w}}; }
static inline AtlrVec2 atlrVec2Mul(const AtlrVec2* restrict v1, const AtlrVec2* restrict v2)   { return (AtlrVec2){{v1->x * v2->x, v1->y * v2->y}}; }
static inline AtlrVec3 atlrVec3Mul(const AtlrVec3* restrict v1, const AtlrVec3* restrict v2)   { return (AtlrVec3){{v1->x * v2->x, v1->y * v2->y, v1->z * v2->z}}; }
static inline AtlrVec4 atlrVec4Mul(const AtlrVec4* restrict v1, const AtlrVec4* restrict v2)   { return (AtlrVec4){{v1->x * v2->x, v1->y * v2->y, v1->z * v2->z, v1->w * v2->w}}; }
static inline AtlrVec2 atlrVec2Div(const AtlrVec2* restrict v1, const AtlrVec2* restrict v2)   { return (AtlrVec2){{v1->x / v2->x, v1->y / v2->y}}; }
static inline AtlrVec3 atlrVec3Div(const AtlrVec3* restrict v1, const AtlrVec3* restrict v2)   { return (AtlrVec3){{v1->x / v2->x, v1->y / v2->y, v1->z / v2->z}}; }
static inline AtlrVec4 atlrVec4Div(const AtlrVec4* restrict v1, const AtlrVec4* restrict v2)   { return (AtlrVec4){{v1->x / v2->x, v1->y / v2->y, v1->z / v2->z, v1->w / v2->w}}; }
static inline AtlrVec2 atlrVec2Scale(const AtlrVec2* restrict v, const float s)                { return (AtlrVec2){{v->x * s, v->y * s}}; }
static inline AtlrVec3 atlrVec3Scale(const AtlrVec3* restrict v, const float s)                { return (AtlrVec3){{v->x * s, v->y * s, v->z * s}}; }
static inline AtlrVec4 atlrVec4Scale(const AtlrVec4* restrict v, const float s)                { return (AtlrVec4){{v->x * s, v->y * s, v->z * s, v->w * s}}; }
static inline float atlrVec2Dot(const AtlrVec2* restrict v1, const AtlrVec2* restrict v2)      { return v1->x * v2->x + v1->y * v2->y; }
static inline float atlrVec3Dot(const AtlrVec3* restrict v1, const AtlrVec3* restrict v2)      { return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z; }
static inline float atlrVec4Dot(const AtlrVec4* restrict v1, const AtlrVec4* restrict v2)      { return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z + v1->w * v2->w; }
static inline AtlrVec3 atlrVec3Cross(const AtlrVec3* restrict v1, const AtlrVec3* restrict v2) { return (AtlrVec3){{v1->y * v2->z - v1->z * v2->y, v1->z * v2->x - v1->x * v2->z, v1->x * v2->y - v1->y * v2->x}}; }

static inline AtlrVec2 atlrVec2Lerp(const AtlrVec2* restrict v1, const AtlrVec2* restrict v2, const float L) {
  return (AtlrVec2){{atlrLerp(v1->x, v2->x, L), atlrLerp(v1->y, v2->y, L)}}; }
static inline AtlrVec3 atlrVec3Lerp(const AtlrVec3* restrict v1, const AtlrVec3* restrict v2, const float L) {
  return (AtlrVec3){{atlrLerp(v1->x, v2->x, L), atlrLerp(v1->y, v2->y, L), atlrLerp(v1->z, v2->z, L)}}; }
static inline AtlrVec4 atlrVec4Lerp(const AtlrVec4* restrict v1, const AtlrVec4* restrict v2, const float L) {
  return (AtlrVec4){{atlrLerp(v1->x, v2->x, L), atlrLerp(v1->y, v2->y, L), atlrLerp(v1->z, v2->z, L), atlrLerp(v1->w, v2->w, L)}}; }

static inline AtlrVec2 atlrVec2Normalize(const AtlrVec2* restrict v)
{
  float dot = atlrVec2Dot(v, v);
  if (dot == 0.0f) return (AtlrVec2){{0.0f, 0.0f}};
  else return atlrVec2Scale(v, 1.0f / sqrtf(dot));
}
static inline AtlrVec3 atlrVec3Normalize(const AtlrVec3* restrict v)
{
  const float dot = atlrVec3Dot(v, v);
  if (dot == 0.0f) return (AtlrVec3){{0.0f, 0.0f, 0.0f}};
  else return atlrVec3Scale(v, 1.0f / sqrtf(dot));
}
static inline AtlrVec4 atlrVec4Normalize(const AtlrVec4* restrict v)
{
  const float dot = atlrVec4Dot(v, v);
  if (dot == 0.0f) return (AtlrVec4){{0.0f, 0.0f, 0.0f, 0.0f}};
  else return atlrVec4Scale(v, 1.0f / sqrtf(dot));
}

static inline AtlrVec4 atlrQuatMul(const AtlrVec4* quat1, const AtlrVec4* quat2)
{
  return (AtlrVec4)
  {{
      quat1->w * quat2->x + quat1->x * quat2->w + quat1->y * quat2->z - quat1->z * quat2->y,
      quat1->w * quat2->y + quat1->y * quat2->w + quat1->z * quat2->x - quat1->x * quat2->z,
      quat1->w * quat2->z + quat1->z * quat2->w + quat1->x * quat2->y - quat1->y * quat2->x,
      quat1->w * quat2->w - quat1->x * quat2->x - quat2->y * quat2->y - quat1->z * quat2->z
  }};
}

static inline AtlrVec4 atlrQuatConjugate(const AtlrVec4* restrict quat) { return (AtlrVec4){{ -quat->x, -quat->y, -quat->z, quat->w }}; }

// Convert an axis-angle pair (angle in degrees) into a unit quaternion.
AtlrVec4 atlrUnitQuatFromAxisAngle(const AtlrVec3* restrict axis, const float angle);

// Slerp between two unit quaternions and return a third unit quaternion.
AtlrVec4 atlrUnitQuatSlerp(const AtlrVec4* restrict quat1, const AtlrVec4* restrict quat2, const float L);

static inline AtlrMat2 atlrMat2Transpose(const AtlrMat2* restrict m) { return (AtlrMat2){{m->xx, m->xy, m->yx, m->yy}}; }
static inline AtlrMat3 atlrMat3Transpose(const AtlrMat3* restrict m) { return (AtlrMat3){{m->xx, m->xy, m->xz, m->yx, m->yy, m->yz, m->zx, m->zy, m->zz}}; }
static inline AtlrMat4 atlrMat4Transpose(const AtlrMat4* restrict m) { return (AtlrMat4){{m->xx, m->xy, m->xz, m->xw, m->yx, m->yy, m->yz, m->yw, m->zx, m->zy, m->zz, m->zw, m->wx, m->wy, m->wz, m->ww}}; }

static inline AtlrMat2 atlrMat2Scale(const AtlrMat2* restrict m, const float s) {
  return (AtlrMat2){.xCol = atlrVec2Scale(&m->xCol, s), .yCol = atlrVec2Scale(&m->yCol, s)}; }
static inline AtlrMat3 atlrMat3Scale(const AtlrMat3* restrict m, const float s) {
  return (AtlrMat3){.xCol = atlrVec3Scale(&m->xCol, s), .yCol = atlrVec3Scale(&m->yCol, s), .zCol = atlrVec3Scale(&m->zCol, s)}; }
static inline AtlrMat4 atlrMat4Scale(const AtlrMat4* restrict m, const float s) {
  return (AtlrMat4){.xCol = atlrVec4Scale(&m->xCol, s), .yCol = atlrVec4Scale(&m->yCol, s), .zCol = atlrVec4Scale(&m->zCol, s), .wCol = atlrVec4Scale(&m->wCol, s)}; }

// matrix multiplication
AtlrMat2 atlrMat2MulMat2(const AtlrMat2* restrict mat1, const AtlrMat2* restrict mat2);
AtlrMat3 atlrMat3MulMat3(const AtlrMat3* restrict mat1, const AtlrMat3* restrict mat2);
AtlrMat4 atlrMat4MulMat4(const AtlrMat4* restrict mat1, const AtlrMat4* restrict mat2);

// matrix and vector multiplication
AtlrVec2 atlrMat2MulVec2(const AtlrMat2* restrict mat, const AtlrVec2* restrict v);
AtlrVec3 atlrMat3MulVec3(const AtlrMat3* restrict mat, const AtlrVec3* restrict v);
AtlrVec4 atlrMat4MulVec4(const AtlrMat4* restrict mat, const AtlrVec4* restrict v);

// matrix and diagonal matrix multiplication
AtlrMat2 atlrMat2MulDiag2(const AtlrMat2* restrict mat, const AtlrVec2* restrict diagMat);
AtlrMat3 atlrMat3MulDiag3(const AtlrMat3* restrict mat, const AtlrVec3* restrict diagMat);
AtlrMat4 atlrMat4MulDiag4(const AtlrMat4* restrict mat, const AtlrVec4* restrict diagMat);

// Convert unit quaternion into a 3 dimensional rotation matrix.
AtlrMat3 atlrRotFromUnitQuat(const AtlrVec4* quat);

AtlrMat4 atlrLookAt(const AtlrVec3* restrict eyePos, const AtlrVec3* restrict targetPos, const AtlrVec3* restrict worldUpDir);

// The frustrum xNear: [-cot, cot], yNear: [-cot / ratio, cot / ratio], z: [-near, -far] where cot = cotangent(fov * pi / 180) and fov is the field of view angle in degrees
// is mapped into clip space x: [-1, 1], y: [1, -1], z: [1, 0]
// This follows the reverse-z convention where higher depths move you to the near plane.
// A y-flip takes counterclockwise to clockwise ordering (and vice versa).
AtlrMat4 atlrPerspectiveProjection(const float fov, const float ratio, const float near, const float far);

AtlrNodeTransform atlrNodeTransformMul(const AtlrNodeTransform* restrict node1, const AtlrNodeTransform* restrict node2);

AtlrNodeTransform atlrNodeTransformInterpolate(const AtlrNodeTransform* restrict node1, const AtlrNodeTransform* restrict node2, const float L);

// Get the node transform in homogeneous coordinates.
AtlrMat4 atlrMat4FromNodeTransform(const AtlrNodeTransform* restrict node);

// Matrix for vertex normals. Mat3 padded with zeros.
AtlrMat4 atlrMat4NormalFromNodeTransform(const AtlrNodeTransform* restrict node);
