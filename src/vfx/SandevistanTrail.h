#pragma once

#include "../PlayerVisual.h"
#include "VfxTypes.h"

namespace vfx::trail {

void stopIfSlowOrGrab(SandevistanTrailState& state, bool grabActive, float playerSpeedPx);
void updateAndSpawn(
    SandevistanTrailState& state,
    cocos2d::CCNode* playerRoot,
    SimplePlayer* player,
    float targetSize,
    int frameId,
    int iconTypeInt,
    float dt
);

} // namespace vfx::trail
