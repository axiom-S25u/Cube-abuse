#pragma once

#include <Geode/cocos/draw_nodes/CCDrawNode.h>

#include "VfxTypes.h"

namespace vfx::impact_flash {

void decrementCooldown(ImpactFlashState& state, float dt);
void decrementWhiteFlash(ImpactFlashState& state, float dt);
overlay_rendering::ImpactFlashMode currentMode(ImpactFlashState const& state);
void updateBackdrops(
    overlay_rendering::ImpactFlashMode mode,
    cocos2d::CCDrawNode* blackBackdrop,
    cocos2d::CCDrawNode* whiteBackdrop
);

} // namespace vfx::impact_flash
