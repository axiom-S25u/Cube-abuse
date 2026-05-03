#include "StarBurst.h"

#include <Geode/Geode.hpp>

#include <Geode/utils/random.hpp>

#include <cmath>
#include <numbers>
#include <string>

#include "../ModTuning.h"

using namespace geode::prelude;

namespace vfx::star_burst {
namespace {

int computeCurrentPhase(float whiteFlashRemaining) {
    if (whiteFlashRemaining <= 0.0f) {
        return -1;
    }
    float const elapsed = kImpactFlashTotalSeconds - whiteFlashRemaining;
    int const phase = static_cast<int>(elapsed / kImpactFlashPhaseSeconds);
    return std::min(phase, kStarBurstMaxPhaseIndex);
}

void applyTint(StarBurstState& state, overlay_rendering::ImpactFlashMode flashMode) {
    bool const whiteBackdrop = flashMode == overlay_rendering::ImpactFlashMode::InvertSilhouette;
    cocos2d::ccColor3B const tint = whiteBackdrop ? ccc3(0, 0, 0) : ccc3(255, 255, 255);
    for (auto* sprite : state.sprites) {
        if (sprite) {
            sprite->setColor(tint);
        }
    }
}

void reposition(StarBurstState& state, cocos2d::CCSize winSize, overlay_rendering::ImpactFlashMode flashMode) {
    float const screenSmaller = winSize.width < winSize.height ? winSize.width : winSize.height;
    float const twoPi = 2.0f * std::numbers::pi_v<float>;

    for (int i = 0; i < kBigStarCount; ++i) {
        auto* sprite = state.sprites[static_cast<size_t>(i)];
        if (!sprite) {
            continue;
        }
        float const sector =
            (static_cast<float>(i) + geode::utils::random::generate<float>(0.0f, 1.0f)) / static_cast<float>(kBigStarCount);
        float const angle = sector * twoPi;
        float const radius = geode::utils::random::generate<float>(
            screenSmaller * kBigStarRadiusMin,
            screenSmaller * kBigStarRadiusMax
        );
        float const cw = sprite->getContentSize().width;
        float const baseScale = cw > 0.0f ? (screenSmaller * kBigStarScreenFrac) / cw : 1.0f;
        sprite->setPosition({std::cos(angle) * radius, std::sin(angle) * radius});
        sprite->setRotation(0.0f);
        sprite->setScale(
            baseScale
            * (1.0f + geode::utils::random::generate<float>(-kStarScaleVariance, kStarScaleVariance))
        );
        sprite->setVisible(true);
    }

    for (int i = 0; i < kSmallStarCount; ++i) {
        auto* sprite = state.sprites[static_cast<size_t>(kBigStarCount + i)];
        if (!sprite) {
            continue;
        }
        float const angle = geode::utils::random::generate<float>(0.0f, twoPi);
        float const radius = geode::utils::random::generate<float>(
            screenSmaller * kSmallStarRadiusMin,
            screenSmaller * kSmallStarRadiusMax
        );
        float const cw = sprite->getContentSize().width;
        float const baseScale = cw > 0.0f ? (screenSmaller * kSmallStarScreenFrac) / cw : 1.0f;
        sprite->setPosition({std::cos(angle) * radius, std::sin(angle) * radius});
        sprite->setRotation(0.0f);
        sprite->setScale(
            baseScale
            * (1.0f + geode::utils::random::generate<float>(-kStarScaleVariance, kStarScaleVariance))
        );
        sprite->setVisible(true);
    }

    applyTint(state, flashMode);
}

} // namespace

void createSprites(StarBurstState& state) {
    if (!state.layer) {
        return;
    }
    for (int i = 0; i < kStarBurstCount; ++i) {
        auto* star = cocos2d::CCSprite::create("img_star1.png"_spr);
        if (!star) {
            continue;
        }
        star->setID(std::string(GEODE_MOD_ID) + "/star-burst-" + std::to_string(i));
        star->setVisible(false);
        star->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
        state.layer->addChild(star, kStarBurstZOrder);
        state.sprites[static_cast<size_t>(i)] = star;
    }
}

void hideAll(StarBurstState& state) {
    for (auto* sprite : state.sprites) {
        if (!sprite) {
            continue;
        }
        sprite->setVisible(false);
        sprite->setColor(ccc3(255, 255, 255));
    }
}

void reset(StarBurstState& state) {
    hideAll(state);
    state.phaseIndex = -1;
}

void update(
    StarBurstState& state,
    float whiteFlashRemaining,
    cocos2d::CCSize winSize,
    overlay_rendering::ImpactFlashMode flashMode
) {
    int const newPhase = computeCurrentPhase(whiteFlashRemaining);
    if (newPhase < 0) {
        if (state.phaseIndex >= 0) {
            hideAll(state);
            state.phaseIndex = -1;
        }
        return;
    }
    if (newPhase != state.phaseIndex) {
        state.phaseIndex = newPhase;
        reposition(state, winSize, flashMode);
    } else {
        applyTint(state, flashMode);
    }
}

} // namespace vfx::star_burst
