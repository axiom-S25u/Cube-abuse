#include "ObjectMotionBlurPipeline.h"

#include <Geode/utils/cocos.hpp>

using namespace geode::prelude;

namespace vfx::object_motion_blur {

bool attach(
    ObjectMotionBlurPipelineState& state,
    cocos2d::CCNode* overlayLayer,
    cocos2d::CCSize captureSize,
    cocos2d::CCSize outputSize,
    int outputZOrder,
    std::array<overlay_rendering::MotionBlurObjectSeed, overlay_rendering::kMotionBlurObjectCount> const& seeds
) {
    auto const result = overlay_rendering::attachObjectMotionBlur(
        overlayLayer,
        captureSize,
        outputSize,
        outputZOrder,
        seeds
    );
    if (!result.ok) {
        return false;
    }

    state.objects = result.objects;
    state.mergeRoot = Ref<cocos2d::CCNode>(result.mergeRoot);
    state.unifiedMergeTexture = Ref<cocos2d::CCRenderTexture>::adopt(result.unifiedMergeTexture);
    state.finalCompositeSprite = Ref<cocos2d::CCSprite>(result.finalCompositeSprite);
    state.whiteFlashSprite = Ref<cocos2d::CCSprite>(result.whiteFlashSprite);
    state.blurProgram = Ref<cocos2d::CCGLProgram>::adopt(result.blurProgram);
    state.whiteFlashProgram = Ref<cocos2d::CCGLProgram>::adopt(result.whiteFlashProgram);
    state.colorInvertProgram = Ref<cocos2d::CCGLProgram>::adopt(result.colorInvertProgram);
    return true;
}

void refresh(
    ObjectMotionBlurPipelineState& state,
    overlay_rendering::ImpactFlashMode flashMode
) {
    overlay_rendering::refreshObjectMotionBlurComposite({
        .objects = &state.objects,
        .mergeRoot = state.mergeRoot,
        .unifiedMergeTexture = state.unifiedMergeTexture,
        .finalCompositeSprite = state.finalCompositeSprite,
        .whiteFlashSprite = state.whiteFlashSprite,
        .whiteFlashProgram = state.whiteFlashProgram,
        .colorInvertProgram = state.colorInvertProgram,
        .impactFlashMode = flashMode,
    });
}

void release(ObjectMotionBlurPipelineState& state) {
    state = {};
}

} // namespace vfx::object_motion_blur
