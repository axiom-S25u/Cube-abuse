#include "ImpactNoise.h"

#include "../ModTuning.h"

#include <algorithm>

namespace vfx::impact_noise {

void update(ImpactNoiseState& state, float dt, bool flashActive) {
    if (!flashActive && state.remaining > 0.0f) {
        state.remaining -= dt;
        state.remaining = std::max(0.0f, state.remaining);
    }

    float const alpha = std::clamp(state.remaining / kImpactNoiseFadeSeconds, 0.0f, 1.0f);

    bool const visible = !flashActive && state.remaining > 0.0f;
    float const extraSkip = state.extraTimeSkip;
    state.extraTimeSkip = 0.0f;

    overlay_rendering::refreshImpactNoise({
        .sprite = state.sprite,
        .renderTexture = state.renderTexture,
        .compositeSprite = state.composite,
        .dt = dt,
        .extraTimeSkip = extraSkip,
        .time = &state.time,
        .alpha = alpha,
        .visible = visible,
    });
}

} // namespace vfx::impact_noise
