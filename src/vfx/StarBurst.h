#pragma once

#include "VfxTypes.h"

namespace vfx::star_burst {

void createSprites(StarBurstState& state);
void hideAll(StarBurstState& state);
void reset(StarBurstState& state);
void update(
    StarBurstState& state,
    float whiteFlashRemaining,
    cocos2d::CCSize winSize,
    overlay_rendering::ImpactFlashMode flashMode
);

} // namespace vfx::star_burst
