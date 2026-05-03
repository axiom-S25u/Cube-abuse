#include "SandevistanTrail.h"

#include <Geode/cocos/cocoa/CCArray.h>

#include "../ModTuning.h"

namespace vfx::trail {

void stopIfSlowOrGrab(SandevistanTrailState& state, bool grabActive, float playerSpeedPx) {
    if (grabActive || playerSpeedPx < kSandevistanEndSpeedPx) {
        state.active = false;
    }
}

void updateAndSpawn(
    SandevistanTrailState& state,
    cocos2d::CCNode* playerRoot,
    SimplePlayer* player,
    float targetSize,
    int frameId,
    int iconTypeInt,
    float dt
) {
    if (!state.layer || !state.active || !playerRoot || !player) {
        return;
    }

    state.spawnAccumulator += dt;
    while (state.spawnAccumulator >= kSandevistanSpawnIntervalSec) {
        int childCount = 0;
        if (cocos2d::CCArray* const children = state.layer->getChildren()) {
            childCount = children->count();
        }
        if (childCount >= kSandevistanMaxConcurrentGhosts) {
            break;
        }
        state.spawnAccumulator -= kSandevistanSpawnIntervalSec;
        player_visual::spawnFadingGhost(
            state.layer,
            playerRoot->getPosition(),
            player->getRotation(),
            targetSize,
            frameId,
            iconTypeInt,
            kSandevistanGhostFadeSec,
            static_cast<unsigned char>(kSandevistanGhostStartOpacity)
        );
    }
}

} // namespace vfx::trail
