#pragma once

#include "ModTuning.h"

#include <Geode/Geode.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/SimplePlayer.hpp>

namespace player_visual {

void requestCubeIconLoad(GameManager* gm, int iconId, int typeInt);

cocos2d::CCRect worldBoundsFromNode(cocos2d::CCNode* n);
cocos2d::CCRect unionRects(cocos2d::CCRect const& a, cocos2d::CCRect const& b);
cocos2d::CCRect unionWorldBoundsTree(cocos2d::CCNode* n, int depth = 0);

float visualWidthForPlayer(SimplePlayer* player);

void applyGmColorsAndFrame(SimplePlayer* player, int frameId);

struct PlayerRootResult {
    bool ok = false;
    cocos2d::CCNode* root = nullptr;
    SimplePlayer* player = nullptr;
};

PlayerRootResult tryBuildPlayerRoot(
    cocos2d::CCLayer* overlay,
    cocos2d::CCSize const& winSize,
    float targetSize,
    int frameId,
    int iconTypeInt
);

bool spawnFadingGhost(
    cocos2d::CCNode* parent,
    cocos2d::CCPoint const& position,
    float rotationDeg,
    float targetSize,
    int frameId,
    int iconTypeInt,
    float fadeSec,
    unsigned char startOpacity
);

} // namespace player_visual
