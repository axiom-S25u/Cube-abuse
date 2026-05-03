#pragma once

#include "VfxTypes.h"

namespace vfx::object_motion_blur {

bool attach(
    ObjectMotionBlurPipelineState& state,
    cocos2d::CCNode* overlayLayer,
    cocos2d::CCSize captureSize,
    cocos2d::CCSize outputSize,
    int outputZOrder,
    std::array<overlay_rendering::MotionBlurObjectSeed, overlay_rendering::kMotionBlurObjectCount> const& seeds
);

void refresh(
    ObjectMotionBlurPipelineState& state,
    overlay_rendering::ImpactFlashMode flashMode
);

void release(ObjectMotionBlurPipelineState& state);

} // namespace vfx::object_motion_blur
