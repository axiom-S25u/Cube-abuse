#pragma once

#include <Geode/cocos/base_nodes/CCNode.h>
#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/cocos/shaders/CCGLProgram.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/cocos/support/CCPointExtension.h>
#include <Geode/utils/cocos.hpp>

#include <array>

#include "../OverlayRendering.h"
#include "../ModTuning.h"

namespace vfx {

struct ImpactFlashState {
    float hitstopRemaining = 0.0f;
    float whiteFlashRemaining = 0.0f;
    float impactFlashCooldownRemaining = 0.0f;
};

struct ImpactNoiseState {
    overlay_rendering::ImpactNoiseSprite* sprite = nullptr;
    geode::Ref<cocos2d::CCRenderTexture> renderTexture{};
    cocos2d::CCSprite* composite = nullptr;
    geode::Ref<cocos2d::CCGLProgram> program{};
    float remaining = 0.0f;
    float time = 0.0f;
    float extraTimeSkip = 0.0f;
};

struct StarBurstState {
    cocos2d::CCNode* layer = nullptr;
    std::array<cocos2d::CCSprite*, kStarBurstSpriteSlots> sprites{};
    int phaseIndex = -1;
};

struct SandevistanTrailState {
    cocos2d::CCNode* layer = nullptr;
    bool active = false;
    float spawnAccumulator = 0.0f;
};

struct FireAuraState {
    overlay_rendering::FireAuraSprite* sprite = nullptr;
    geode::Ref<cocos2d::CCGLProgram> program{};
    float time = 0.0f;
};

struct ObjectMotionBlurPipelineState {
    std::array<overlay_rendering::MotionBlurObjectCapture, overlay_rendering::kMotionBlurObjectCount> objects = {};
    geode::Ref<cocos2d::CCNode> mergeRoot{};
    geode::Ref<cocos2d::CCRenderTexture> unifiedMergeTexture{};
    geode::Ref<cocos2d::CCSprite> finalCompositeSprite{};
    geode::Ref<cocos2d::CCSprite> whiteFlashSprite{};
    geode::Ref<cocos2d::CCGLProgram> blurProgram{};
    geode::Ref<cocos2d::CCGLProgram> whiteFlashProgram{};
    geode::Ref<cocos2d::CCGLProgram> colorInvertProgram{};
};

} // namespace vfx
