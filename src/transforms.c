/*

This file is part of antler.
Copyright (C) 2024 Taylor Wampler 

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
  
*/

#include "transforms.h"
#include "antler.h"

#define SLERP_TOLERANCE 0.99

AtlrVec4 atlrUnitQuatFromAxisAngle(const AtlrVec3* restrict axis, const float angle)
{
  const float theta = angle * M_PI / 360;
  AtlrVec3 vec = atlrVec3Normalize(axis);
  vec = atlrVec3Scale(&vec, sinf(theta));
  return (AtlrVec4){{vec.x, vec.y, vec.z, cosf(theta)}};
}

AtlrVec4 atlrUnitQuatSlerp(const AtlrVec4* restrict quat1, const AtlrVec4* restrict quat2, const float L)
{
  float cosTheta = atlrVec4Dot(quat1, quat2);
  AtlrVec4 quat = *quat2;

  // unit quaternions provide a double cover over the space of 3D rotations, pick the shortest of the two available paths
  if (cosTheta < 0)
  {
    cosTheta = -cosTheta;
    quat = (AtlrVec4){{-quat.x, -quat.y, -quat.z, -quat.w}}; 
  }

  // nlerp optimization for short paths
  if (cosTheta > SLERP_TOLERANCE)
  {
    quat = atlrVec4Lerp(quat1, &quat, L);
    return atlrVec4Normalize(&quat);
  }

  const float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
  const float theta = acosf(cosTheta);
  const float L1 = sinf(theta * (1.0f - L));
  const float L2 = sinf(theta * L);
  return (AtlrVec4)
  {{
      (L1 * quat1->x + L2 * quat.x) / sinTheta,
      (L1 * quat1->y + L2 * quat.y) / sinTheta,
      (L1 * quat1->z * L2 * quat.z) / sinTheta,
      (L1 * quat1->w * L2 * quat.w) / sinTheta
  }};
}

AtlrMat2 atlrMat2MulMat2(const AtlrMat2* restrict mat1, const AtlrMat2* restrict mat2)
{
  AtlrMat2 result;
  for (AtlrU8 j = 0; j < 2; j++)
    for (AtlrU8 i = 0; i < 2; i++)
    {
      result.arr[j][i] = 0.0f;
      for (AtlrU8 k = 0; k < 2; k++)
	result.arr[j][i] += mat1->arr[k][i] * mat2->arr[j][k];
    }  
  return result;
}

AtlrMat3 atlrMat3MulMat3(const AtlrMat3* restrict mat1, const AtlrMat3* restrict mat2)
{
  AtlrMat3 result;
  for (AtlrU8 j = 0; j < 3; j++)
    for (AtlrU8 i = 0; i < 3; i++)
    {
      result.arr[j][i] = 0.0f;
      for (AtlrU8 k = 0; k < 3; k++)
	result.arr[j][i] += mat1->arr[k][i] * mat2->arr[j][k];
    }  
  return result;
}

AtlrMat4 atlrMat4MulMat4(const AtlrMat4* restrict mat1, const AtlrMat4* restrict mat2)
{
  AtlrMat4 result;
  for (AtlrU8 j = 0; j < 4; j++)
    for (AtlrU8 i = 0; i < 4; i++)
    {
      result.arr[j][i] = 0.0f;
      for (AtlrU8 k = 0; k < 4; k++)
	result.arr[j][i] += mat1->arr[k][i] * mat2->arr[j][k];
    }  
  return result;
}

AtlrVec2 atlrMat2MulVec2(const AtlrMat2* restrict mat, const AtlrVec2* restrict v)
{
  AtlrVec2 result;
  for (AtlrU8 i = 0; i < 2; i++)
    {
      result.arr[i] = 0.0f;
      for (AtlrU8 j = 0; j < 2; j++)
	result.arr[i] += mat->arr[j][i] * v->arr[j];
    }  
  return result;
}

AtlrVec3 atlrMat3MulVec3(const AtlrMat3* restrict mat, const AtlrVec3* restrict v)
{
  AtlrVec3 result;
  for (AtlrU8 i = 0; i < 3; i++)
    {
      result.arr[i] = 0.0f;
      for (AtlrU8 j = 0; j < 3; j++)
	result.arr[i] += mat->arr[j][i] * v->arr[j];
    }  
  return result;
}

AtlrVec4 atlrMat4MulVec4(const AtlrMat4* restrict mat, const AtlrVec4* restrict v)
{
  AtlrVec4 result;
  for (AtlrU8 i = 0; i < 4; i++)
    {
      result.arr[i] = 0.0f;
      for (AtlrU8 j = 0; j < 4; j++)
	result.arr[i] += mat->arr[j][i] * v->arr[j];
    }  
  return result;
}

AtlrMat2 atlrMat2MulDiag2(const AtlrMat2* restrict mat, const AtlrVec2* restrict diagMat)
{
  return (AtlrMat2) 
  { .cols = 
    { 
      atlrVec2Scale(&mat->xCol, diagMat->x), 
      atlrVec2Scale(&mat->yCol, diagMat->y)
    } 
  };
}

AtlrMat3 atlrMat3MulDiag3(const AtlrMat3* restrict mat, const AtlrVec3* restrict diagMat)
{
  return (AtlrMat3) 
  { .cols = 
    { 
      atlrVec3Scale(&mat->xCol, diagMat->x), 
      atlrVec3Scale(&mat->yCol, diagMat->y),
      atlrVec3Scale(&mat->zCol, diagMat->z) 
    } 
  };
}

AtlrMat4 atlrMat4MulDiag4(const AtlrMat4* restrict mat, const AtlrVec4* restrict diagMat)
{
  return (AtlrMat4) 
  { .cols = 
    { 
      atlrVec4Scale(&mat->xCol, diagMat->x), 
      atlrVec4Scale(&mat->yCol, diagMat->y),
      atlrVec4Scale(&mat->zCol, diagMat->z),
      atlrVec4Scale(&mat->wCol, diagMat->w)
    } 
  };
}

AtlrMat3 atlrRotFromUnitQuat(const AtlrVec4* quat)
{
  const float xx = quat->x * quat->x;
  const float xy = quat->x * quat->y;
  const float xz = quat->x * quat->z;
  const float xw = quat->x * quat->w;
  const float yy = quat->y * quat->y;
  const float yz = quat->y * quat->z;
  const float yw = quat->y * quat->w;
  const float zz = quat->z * quat->z;
  const float zw = quat->z * quat->w;
  return (AtlrMat3)
  {{
      1 - 2 * (yy + zz), 2 * (xy + zw)    , 2 * (xz - yw)    ,
      2 * (xy - zw)    , 1 - 2 * (xx + zz), 2 * (yz + xw)    ,
      2 * (xz + yw)    , 2 * (yz - xw)    , 1 - 2 * (xx + yy)
  }};
}

AtlrMat4 atlrLookAt(const AtlrVec3* restrict eyePos, const AtlrVec3* restrict targetPos, const AtlrVec3* restrict worldUpDir)
{
  AtlrVec3 forward = atlrVec3Sub(eyePos, targetPos);
  forward = atlrVec3Normalize(&forward);
  AtlrVec3 left = atlrVec3Cross(worldUpDir, &forward);
  left = atlrVec3Normalize(&left);
  const AtlrVec3 up = atlrVec3Cross(&forward, &left);
  const AtlrVec3 dot =
  {{
    -atlrVec3Dot(&left, eyePos),
    -atlrVec3Dot(&up, eyePos),
    -atlrVec3Dot(&forward, eyePos)
  }};
  return (AtlrMat4)
  {{
      left.x, up.x , forward.x, 0.0f,
      left.y, up.y , forward.y, 0.0f,
      left.z, up.z , forward.z, 0.0f,
      dot.x , dot.y, dot.z    , 1.0f
  }};
}

AtlrMat4 atlrPerspectiveProjection(const float fov, const float ratio, const float near, const float far)
{
  const float cot = 1.0f / tanf(fov * M_PI / 360);
  const float zDelta = far - near; 
  return (AtlrMat4)
  {{
      cot,          0.0f, 0.0f               , 0.0f ,
      0.0f, -cot / ratio, 0.0f               , 0.0f ,
      0.0f, 0.0f        , -near / zDelta     , -1.0f,
      0.0f, 0.0f        , near * far / zDelta,  0.0f
  }};
}

AtlrNodeTransform atlrNodeTransformMul(const AtlrNodeTransform* restrict node1, const AtlrNodeTransform* restrict node2)
{
  AtlrNodeTransform result;

  // scale = scale2 * scale1
  result.scale = atlrVec3Mul(&node2->scale, &node1->scale);

  // quat = normalize(quat2 ** quat1)
  result.rotate = atlrQuatMul(&node2->rotate, &node1->rotate);
  result.rotate = atlrVec4Normalize(&result.rotate);

  // translate = translate2 + rot2 . (scale2 * translate1) where rot2 is the rotation matrix made from the unit quaternion quat2
  result.translate = atlrVec3Mul(&node2->scale, &node1->translate);
  const AtlrMat3 rot2 = atlrRotFromUnitQuat(&result.rotate);
  result.translate = atlrMat3MulVec3(&rot2, &result.translate);
  result.translate = atlrVec3Add(&node2->translate, &result.translate);

  return result;
}

AtlrNodeTransform atlrNodeTransformInterpolate(const AtlrNodeTransform* restrict node1, const AtlrNodeTransform* restrict node2, const float L)
{
  AtlrNodeTransform result;
  result.scale = atlrVec3Lerp(&node1->scale, &node2->scale, L);
  result.rotate = atlrUnitQuatSlerp(&node1->rotate, &node2->rotate, L);
  result.translate = atlrVec3Lerp(&node1->translate, &node2->translate, L);
  return result;
}

AtlrMat4 atlrMat4FromNodeTransform(const AtlrNodeTransform* restrict node)
{
  AtlrMat3 mat3 = atlrRotFromUnitQuat(&node->rotate);
  mat3 = atlrMat3MulDiag3(&mat3, &node->scale);
  const AtlrVec3* t = &node->translate;

  return (AtlrMat4)
  {{
      mat3.xx, mat3.yx, mat3.zx, 0.0f,
      mat3.xy, mat3.yy, mat3.zy, 0.0f,
      mat3.xz, mat3.yz, mat3.zz, 0.0f,
      t->x   , t->y   , t->z   , 1.0f
  }};
}

AtlrMat4 atlrMat4NormalFromNodeTransform(const AtlrNodeTransform* restrict node)
{
  AtlrMat3 mat3 = atlrRotFromUnitQuat(&node->rotate);
  AtlrVec3 scale = {{1.0f / node->scale.x, 1.0f / node->scale.y, 1.0f / node->scale.z}};
  mat3 = atlrMat3MulDiag3(&mat3, &scale);

  return (AtlrMat4)
  {{
      mat3.xx, mat3.yx, mat3.zx, 0.0f,
      mat3.xy, mat3.yy, mat3.zy, 0.0f,
      mat3.xz, mat3.yz, mat3.zz, 0.0f,
      0.0f   , 0.0f   , 0.0f   , 0.0f
  }};
}
