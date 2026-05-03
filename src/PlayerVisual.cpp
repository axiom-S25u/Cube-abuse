#include "PlayerVisual.h"

#include <Geode/Enums.hpp>
#include <Geode/cocos/actions/CCActionInterval.h>
#include <Geode/cocos/actions/CCActionInstant.h>
#include <Geode/cocos/cocoa/CCArray.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_set>

using namespace geode::prelude;

namespace {

std::atomic<std::uint32_t> s_trailGhostSerial{0};

void applyAdditiveBlendRecursive(CCNode* n, int depth, std::unordered_set<CCNode*>& visited) {
    if (!n || depth > player_visual::kMaxWorldBoundsTreeDepth) {
        return;
    }
    if (!visited.insert(n).second) {
        return;
    }
    if (auto* const spr = typeinfo_cast<CCSprite*>(n)) {
        spr->setBlendFunc({GL_SRC_ALPHA, GL_ONE});
    }
    if (CCArray* const ch = n->getChildren()) {
        for (unsigned i = 0; i < static_cast<unsigned>(ch->count()); ++i) {
            applyAdditiveBlendRecursive(typeinfo_cast<CCNode*>(ch->objectAtIndex(i)), depth + 1, visited);
        }
    }
}

inline GLubyte lerpByte(GLubyte a, GLubyte b, float u) {
    return static_cast<GLubyte>(std::round(
        static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * u
    ));
}

inline ccColor3B lerpRgb(ccColor3B const& x, ccColor3B const& y, float u) {
    u = std::clamp(u, 0.0f, 1.0f);
    return ccc3(lerpByte(x.r, y.r, u), lerpByte(x.g, y.g, u), lerpByte(x.b, y.b, u));
}

ccColor3B sandevistanTrailColorAtUnit(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    ccColor3B const orange = ccc3(
        static_cast<GLubyte>(kSandevistanTrailHueOrangeR),
        static_cast<GLubyte>(kSandevistanTrailHueOrangeG),
        static_cast<GLubyte>(kSandevistanTrailHueOrangeB)
    );
    ccColor3B const purple = ccc3(
        static_cast<GLubyte>(kSandevistanTrailHuePurpleR),
        static_cast<GLubyte>(kSandevistanTrailHuePurpleG),
        static_cast<GLubyte>(kSandevistanTrailHuePurpleB)
    );
    ccColor3B const cyan = ccc3(
        static_cast<GLubyte>(kSandevistanTrailHueCyanR),
        static_cast<GLubyte>(kSandevistanTrailHueCyanG),
        static_cast<GLubyte>(kSandevistanTrailHueCyanB)
    );
    float const third = 1.0f / 3.0f;
    if (t < third) {
        return lerpRgb(orange, purple, t / third);
    }
    if (t < 2.0f * third) {
        return lerpRgb(purple, cyan, (t - third) / third);
    }
    return cyan;
}

class SandevistanPlayerHueAction : public CCActionInterval {
public:
    static SandevistanPlayerHueAction* create(float duration) {
        auto a = std::unique_ptr<SandevistanPlayerHueAction>(new SandevistanPlayerHueAction());
        if (!a->initWithDuration(duration)) {
            return nullptr;
        }
        a->autorelease();
        return a.release();
    }

    void update(float time) override {
        auto* const p = typeinfo_cast<SimplePlayer*>(getTarget());
        if (!p) {
            return;
        }
        ccColor3B const col = sandevistanTrailColorAtUnit(time);
        p->setColors(col, col);
        p->setGlowOutline(col);
        p->updateColors();
    }
};

} // namespace

namespace player_visual {

void requestCubeIconLoad(GameManager* gm, int iconId, int typeInt) {
    if (!gm->isIconLoaded(iconId, typeInt)) {
        int const requestId = gm->getIconRequestID();
        gm->loadIcon(iconId, typeInt, requestId);
    }
}

CCRect worldBoundsFromNode(CCNode* n) {
    CCRect const bb = n->boundingBox();
    CCNode* parent = n->getParent();
    if (!parent) {
        return bb;
    }
    auto const corner = [&](float x, float y) {
        return parent->convertToWorldSpace(ccp(x, y));
    };
    float const minX = bb.origin.x;
    float const minY = bb.origin.y;
    float const maxX = bb.origin.x + bb.size.width;
    float const maxY = bb.origin.y + bb.size.height;
    CCPoint const c0 = corner(minX, minY);
    CCPoint const c1 = corner(maxX, minY);
    CCPoint const c2 = corner(minX, maxY);
    CCPoint const c3 = corner(maxX, maxY);
    float minWx = std::min({c0.x, c1.x, c2.x, c3.x});
    float maxWx = std::max({c0.x, c1.x, c2.x, c3.x});
    float minWy = std::min({c0.y, c1.y, c2.y, c3.y});
    float maxWy = std::max({c0.y, c1.y, c2.y, c3.y});
    return CCRectMake(minWx, minWy, maxWx - minWx, maxWy - minWy);
}

CCRect unionRects(CCRect const& a, CCRect const& b) {
    float const minX = std::min(a.getMinX(), b.getMinX());
    float const minY = std::min(a.getMinY(), b.getMinY());
    float const maxX = std::max(a.getMaxX(), b.getMaxX());
    float const maxY = std::max(a.getMaxY(), b.getMaxY());
    return CCRectMake(minX, minY, maxX - minX, maxY - minY);
}

CCRect unionWorldBoundsTree(CCNode* n, int depth) {
    if (!n || depth > kMaxWorldBoundsTreeDepth) {
        return CCRectZero;
    }
    CCRect acc = worldBoundsFromNode(n);
    if (CCArray* children = n->getChildren()) {
        unsigned int const nChildren = children->count();
        for (unsigned int i = 0; i < nChildren; ++i) {
            auto* ch = typeinfo_cast<CCNode*>(children->objectAtIndex(i));
            if (!ch) {
                continue;
            }
            acc = unionRects(acc, unionWorldBoundsTree(ch, depth + 1));
        }
    }
    return acc;
}

float visualWidthForPlayer(SimplePlayer* player) {
    CCRect const world = unionWorldBoundsTree(player);
    float const ww = std::fabs(world.size.width);
    if (ww > kMinVisualWidthPx) {
        return ww;
    }
    float const cw = player->getContentSize().width;
    return cw > kMinVisualWidthPx ? cw : kMinVisualWidthPx;
}

void applyGmColorsAndFrame(SimplePlayer* player, int frameId) {
    auto* gm = GameManager::get();
    if (!gm || !player) {
        return;
    }

    player->updatePlayerFrame(frameId, IconType::Cube);
    player->setColors(
        gm->colorForIdx(gm->getPlayerColor()),
        gm->colorForIdx(gm->getPlayerColor2())
    );
    if (gm->getPlayerGlow()) {
        player->setGlowOutline(gm->colorForIdx(gm->getPlayerGlowColor()));
    } else {
        player->disableGlowOutline();
    }
    player->updateColors();
}

PlayerRootResult tryBuildPlayerRoot(
    CCLayer* overlay,
    CCSize const& winSize,
    float targetSize,
    int frameId,
    int iconTypeInt
) {
    PlayerRootResult out{};
    auto* gm = GameManager::get();
    if (!gm) {
        return out;
    }

    if (!gm->isIconLoaded(frameId, iconTypeInt)) {
        return out;
    }

    auto* player = SimplePlayer::create(frameId);
    if (!player) {
        return out;
    }
    player->setID("player-icon"_spr);

    auto* root = CCNode::create();
    root->setID("player-icon-root"_spr);
    root->setPosition({
        winSize.width * kPlayerRootAnchorXFrac,
        winSize.height * kPlayerRootAnchorYFrac
    });
    root->addChild(player, kPlayerVisualLocalZOrder);
    overlay->addChild(root);

    applyGmColorsAndFrame(player, frameId);

    float const w = visualWidthForPlayer(player);
    float const scale = targetSize / w;
    player->setScale(scale);
    player->setPosition({0, 0});

    out.ok = true;
    out.root = root;
    out.player = player;
    return out;
}

bool spawnFadingGhost(
    CCNode* parent,
    CCPoint const& position,
    float rotationDeg,
    float targetSize,
    int frameId,
    int iconTypeInt,
    float fadeSec,
    unsigned char startOpacity
) {
    auto* gm = GameManager::get();
    if (!gm || !parent) {
        return false;
    }
    if (!gm->isIconLoaded(frameId, iconTypeInt)) {
        return false;
    }

    auto* player = SimplePlayer::create(frameId);
    if (!player) {
        return false;
    }
    {
        std::string const ghostId =
            std::string(GEODE_MOD_ID) + "/trail-ghost-" + std::to_string(++s_trailGhostSerial);
        player->setID(ghostId);
    }

    player->updatePlayerFrame(frameId, IconType::Cube);
    ccColor3B const c0 = sandevistanTrailColorAtUnit(0.0f);
    player->setColors(c0, c0);
    player->setGlowOutline(c0);
    player->updateColors();
    std::unordered_set<CCNode*> blendVisited;
    blendVisited.reserve(32);
    applyAdditiveBlendRecursive(player, 0, blendVisited);

    float const w = visualWidthForPlayer(player);
    float const scale = targetSize / w;
    player->setScale(scale);
    player->setPosition(position);
    player->setRotation(rotationDeg);
    player->setOpacity(startOpacity);

    parent->addChild(player, kPlayerVisualLocalZOrder);
    player->runAction(CCSequence::create(
        CCSpawn::create(
            CCFadeOut::create(fadeSec),
            SandevistanPlayerHueAction::create(fadeSec),
            nullptr
        ),
        CCRemoveSelf::create(),
        nullptr
    ));
    return true;
}

} // namespace player_visual
