#pragma once
#include "antler.h"
#include "transforms.h"

typedef struct _AtlrPerspectiveCamera
{
  const AtlrDevice* device;
  AtlrU8 frameCount;
  AtlrBuffer* uniformBuffers;
  AtlrDescriptorSetLayout descriptorSetLayout;
  AtlrDescriptorPool descriptorPool;
  VkDescriptorSet* descriptorSets;
  
  float fov, nearPlane, farPlane;
  struct
  {
    AtlrVec4 eyePos;
    AtlrMat4 view;
    AtlrMat4 perspective;
    
  } uniformData;
  
} AtlrPerspectiveCamera;

AtlrU8 atlrInitPerspectiveCameraHostGLFW(AtlrPerspectiveCamera* restrict, const AtlrU8 frameCount, const float fov, const float nearPlane, const float farPlane,
					 const AtlrDevice* restrict);
void atlrDeinitPerspectiveCameraHostGLFW(const AtlrPerspectiveCamera* restrict);
void atlrUpdatePerspectiveCameraHostGLFW(AtlrPerspectiveCamera* restrict, const AtlrU8 currentFrame);
void atlrPerspectiveCameraLookAtHostGLFW(AtlrPerspectiveCamera* restrict, const AtlrVec3* restrict eyePos, const AtlrVec3* restrict targetPos, const AtlrVec3* restrict worldUpDir);
