#pragma once

#include "cpp-compat.hpp"
extern "C"
{ 
#include "antler.h"
#include "transforms.h"
}
#include <vector>

namespace Atlr
{

  class ImguiContext
  {
    const AtlrDevice* device;
    
    AtlrImage fontImage;
    VkSampler fontSampler;

    AtlrDescriptorSetLayout descriptorSetLayout;
    AtlrDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    AtlrPipeline pipeline;

    std::vector<AtlrBuffer> vertexBuffers;
    std::vector<AtlrU32> vertexCounts;
    std::vector<AtlrBuffer> indexBuffers;
    std::vector<AtlrU32> indexCounts;

  public:
    struct Transform
    {
      AtlrVec2 translate;
      AtlrVec2 scale;
      
    } transform;

    ImguiContext() = default;
    void init(const AtlrU8 frameCount, const AtlrSwapchain* restrict, const AtlrSingleRecordCommandContext* restrict);
    void deinit();

    void bind(const VkCommandBuffer, const AtlrU8 currentFrame);
    void draw(const VkCommandBuffer, const AtlrU8 currentFrame);
  };

}
