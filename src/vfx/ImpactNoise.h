#pragma once

#include "VfxTypes.h"

namespace vfx::impact_noise {

void update(ImpactNoiseState& state, float dt, bool flashActive);

} // namespace vfx::impact_noise
