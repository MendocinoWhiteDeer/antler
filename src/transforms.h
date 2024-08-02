#pragma once
#include <math.h>

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

static inline AtlrVec2 atlrVec2Add(const AtlrVec2* v1, const AtlrVec2* v2) { return (AtlrVec2){{v1->x + v2->x, v1->y + v2->y}}; }
static inline AtlrVec3 atlrVec3Add(const AtlrVec3* v1, const AtlrVec3* v2) { return (AtlrVec3){{v1->x + v2->x, v1->y + v2->y, v1->z + v2->z}}; }
static inline AtlrVec4 atlrVec4Add(const AtlrVec4* v1, const AtlrVec4* v2) { return (AtlrVec4){{v1->x + v2->x, v1->y + v2->y, v1->z + v2->z, v1->w + v2->w}}; }

static inline AtlrVec2 atlrVec2Sub(const AtlrVec2* v1, const AtlrVec2* v2) { return (AtlrVec2){{v1->x - v2->x, v1->y - v2->y}}; }
static inline AtlrVec3 atlrVec3Sub(const AtlrVec3* v1, const AtlrVec3* v2) { return (AtlrVec3){{v1->x - v2->x, v1->y - v2->y, v1->z - v2->z}}; }
static inline AtlrVec4 atlrVec4Sub(const AtlrVec4* v1, const AtlrVec4* v2) { return (AtlrVec4){{v1->x - v2->x, v1->y - v2->y, v1->z - v2->z, v1->w - v2->w}}; }

static inline AtlrVec2 atlrVec2Mul(const AtlrVec2* v1, const AtlrVec2* v2) { return (AtlrVec2){{v1->x * v2->x, v1->y * v2->y}}; }
static inline AtlrVec3 atlrVec3Mul(const AtlrVec3* v1, const AtlrVec3* v2) { return (AtlrVec3){{v1->x * v2->x, v1->y * v2->y, v1->z * v2->z}}; }
static inline AtlrVec4 atlrVec4Mul(const AtlrVec4* v1, const AtlrVec4* v2) { return (AtlrVec4){{v1->x * v2->x, v1->y * v2->y, v1->z * v2->z, v1->w * v2->w}}; }

static inline AtlrVec2 atlrVec2Div(const AtlrVec2* v1, const AtlrVec2* v2) { return (AtlrVec2){{v1->x / v2->x, v1->y / v2->y}}; }
static inline AtlrVec3 atlrVec3Div(const AtlrVec3* v1, const AtlrVec3* v2) { return (AtlrVec3){{v1->x / v2->x, v1->y / v2->y, v1->z / v2->z}}; }
static inline AtlrVec4 atlrVec4Div(const AtlrVec4* v1, const AtlrVec4* v2) { return (AtlrVec4){{v1->x / v2->x, v1->y / v2->y, v1->z / v2->z, v1->w / v2->w}}; }

static inline AtlrVec2 atlrVec2Scale(const AtlrVec2* restrict v, const float s) { return (AtlrVec2){{v->x * s, v->y * s}}; }
static inline AtlrVec3 atlrVec3Scale(const AtlrVec3* restrict v, const float s) { return (AtlrVec3){{v->x * s, v->y * s, v->z * s}}; }
static inline AtlrVec4 atlrVec4Scale(const AtlrVec4* restrict v, const float s) { return (AtlrVec4){{v->x * s, v->y * s, v->z * s, v->w * s}}; }

static inline float atlrVec2Dot(const AtlrVec2* v1, const AtlrVec2* v2) { return v1->x * v2->x + v1->y * v2->y; }
static inline float atlrVec3Dot(const AtlrVec3* v1, const AtlrVec3* v2) { return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z; }
static inline float atlrVec4Dot(const AtlrVec4* v1, const AtlrVec4* v2) { return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z + v1->w * v2->w; }

static inline AtlrVec3 atlrVec3Cross(const AtlrVec3* v1, const AtlrVec3* v2) { return (AtlrVec3){{v1->y * v2->z - v1->z * v2->y, v1->z * v2->x - v1->x * v2->z, v1->x * v2->y - v1->y * v2->x}}; }

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
  else return atlrVec2Scale(v, sqrtf(dot));
}
static inline AtlrVec3 atlrVec3Normalize(const AtlrVec3* restrict v)
{
  const float dot = atlrVec3Dot(v, v);
  if (dot == 0.0f) return (AtlrVec3){{0.0f, 0.0f, 0.0f}};
  else return atlrVec3Scale(v, sqrtf(dot));
}
static inline AtlrVec4 atlrVec4Normalize(const AtlrVec4* restrict v)
{
  const float dot = atlrVec4Dot(v, v);
  if (dot == 0.0f) return (AtlrVec4){{0.0f, 0.0f, 0.0f, 0.0f}};
  else return atlrVec4Scale(v, sqrtf(dot));
}

typedef union _AtlrMat2
{
  struct { float xx, yx, xy, yy; };
  float arr[4];
  struct { AtlrVec2 xCol, yCol; };
  AtlrVec2 cols[2];

} AtlrMat2;

typedef union _AtlrMat3
{
  struct { float xx, yx, zx, xy, yy, zy, xz, yz, zz; };
  float arr[9];
  struct { AtlrVec3 xCol, yCol, zCol; };
  AtlrVec3 cols[3];

} AtlrMat3;

typedef union _AtlrMat4
{
  struct { float xx, yx, zx, wx, xy, yy, zy, wy, xz, yz, zz, wz, xw, yw, zw, ww; };
  float arr[16];
  struct { AtlrVec4 xCol, yCol, zCol, wCol; };
  AtlrVec4 cols[4];

} AtlrMat4;

static inline AtlrMat2 atlrMat2Transpose(const AtlrMat2* restrict m) { return (AtlrMat2){{m->xx, m->xy, m->yx, m->yy}}; }
static inline AtlrMat3 atlrMat3Transpose(const AtlrMat3* restrict m) { return (AtlrMat3){{m->xx, m->xy, m->xz, m->yx, m->yy, m->yz, m->zx, m->zy, m->zz}}; }
static inline AtlrMat4 atlrMat4Transpose(const AtlrMat4* restrict m) { return (AtlrMat4){{m->xx, m->xy, m->xz, m->xw, m->yx, m->yy, m->yz, m->yw, m->zx, m->zy, m->zz, m->zw, m->wx, m->wy, m->wz, m->ww}}; }

static inline AtlrMat2 atlrMat2Scale(const AtlrMat2* restrict m, const float s){
  return (AtlrMat2){.xCol = atlrVec2Scale(&m->xCol, s), .yCol = atlrVec2Scale(&m->yCol, s)}; }
static inline AtlrMat3 atlrMat3Scale(const AtlrMat3* restrict m, const float s) {
  return (AtlrMat3){.xCol = atlrVec3Scale(&m->xCol, s), .yCol = atlrVec3Scale(&m->yCol, s), .zCol = atlrVec3Scale(&m->zCol, s)}; }
static inline AtlrMat4 atlrMat4Scale(const AtlrMat4* restrict m, const float s) {
  return (AtlrMat4){.xCol = atlrVec4Scale(&m->xCol, s), .yCol = atlrVec4Scale(&m->yCol, s), .zCol = atlrVec4Scale(&m->zCol, s), .wCol = atlrVec4Scale(&m->wCol, s)}; }
