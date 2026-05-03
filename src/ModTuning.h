#pragma once

#include <numbers>

// App-wide tuning (single header), box2d-lite collision/restitution/vertex cap
// included here for one place to edit, see kB2* symbols
//
// SPLIT: constexpr = compile-time-required (array sizes, static_asserts, derived)
//        inline    = runtime-tunable via Geode settings (seeded by ModSettings.cpp)

// Important
constexpr float kHideModOverlayOffsetX = 676767.0f;
constexpr float kHideModOverlayOffsetY = 676767.0f;
inline bool kHideModOverlay = false;

namespace player_visual {

// Player Visual
inline int kMaxWorldBoundsTreeDepth = 64;
inline float kMinVisualWidthPx = 1.0f;
inline float kPlayerTargetSizeFraction = 0.125f;
inline int kMinPlayerFrameId = 1;

inline float kPlayerRootAnchorXFrac = 0.5f;
inline float kPlayerRootAnchorYFrac = 0.5f;
inline int kPlayerVisualLocalZOrder = 0;

} // namespace player_visual

// Physics Overlay
inline int kPhysicsOverlayZOrder = 1000;
inline int kPhysicsOverlayTouchPriority = -6767;
inline int kPhysicsOverlaySchedulerPriority = 0;

// Player Root
inline int kPlayerRootZOrder = 1;
constexpr float kFixedPhysicsDt = 1.0f / 120.0f;
static_assert(kFixedPhysicsDt > 0.0f);
constexpr int kMaxPhysicsSubsteps = 16;
constexpr float kMaxSimulationFrameDt = 1.0f / 30.0f;
static_assert(kMaxSimulationFrameDt > 0.0f);
constexpr float kPhysicsAccumulatorCap = kFixedPhysicsDt * static_cast<float>(kMaxPhysicsSubsteps);
static_assert(kPhysicsAccumulatorCap > 0.0f);
static_assert(kMaxSimulationFrameDt <= kPhysicsAccumulatorCap);
constexpr float kRadToDeg = 180.0f / std::numbers::pi_v<float>;
inline float kMinSpeedForInverse = 1e-6f;
inline float kGrabRadiusFraction = 2.0f / 3.0f;

// VFX Z Orders
inline int kImpactFlashBackdropZOrder = -3;
inline int kFireAuraZOrder = -1;
inline int kStarBurstZOrder = 0;
inline int kUnifiedWorldCaptureZOrder = 1;
inline int kUnifiedBlurCompositeZOrder = 2;
inline int kImpactNoiseZOrder = 3;
inline int kGlobalStartBurstZOrder = 4;
inline int kLayerTrailZOrderOffset = -1;
inline int kLayerWorldZOrderOffset = 0;
inline int kLayerUiZOrderOffset = 2;

// VFX Toggles
inline bool kEnablePlayerImpactTrail = true;
inline bool kEnablePlayerImpactFlashStack = true;

// Impact Flash
inline float kImpactHitstopSeconds = 0.15f;
constexpr float kImpactFlashTotalSeconds = 0.15f;
constexpr float kImpactFlashPhaseSeconds = 0.05f;
inline float kImpactFlashCooldownSeconds = 0.4f;
inline int kImpactFlashInvertPhaseEndPhaseCount = 2;
constexpr int kStarBurstMaxPhaseIndex =
    static_cast<int>(kImpactFlashTotalSeconds / kImpactFlashPhaseSeconds) - 1;

// Player Impact
inline float kPlayerImpactMinFlashSpeed = 1600.0f;
inline float kPlayerImpactMinShakeSpeed = 300.0f;
inline float kPlayerImpactShakeDuration = 0.25f;
inline float kPlayerImpactShakeSpeedToStrength = 0.005f;
inline float kPlayerImpactMaxShakeStrength = 6.7f;

// Screen Shake
inline int kScreenShakeIntervals = 10;
inline float kScreenShakeSampleMin = -1.0f;
inline float kScreenShakeSampleMax = 1.0f;
inline float kScreenShakeCooldownExtraSeconds = 0.1f;

// Impact Noise
inline float kImpactNoiseFadeSeconds = 1.75f;
inline float kImpactNoiseStackedImpactTimeSkip = 73.0f;
inline bool kImpactNoiseCompositeNearestFilter = true;
inline float kImpactNoiseRenderScale = 0.1f;

// Star Burst
constexpr int kStarBurstSpriteSlots = 5;
inline int kStarBurstCount = kStarBurstSpriteSlots;
inline int kBigStarCount = 2;
inline int kSmallStarCount = 3;
inline float kBigStarRadiusMin = 0.05f;
inline float kBigStarRadiusMax = 0.3f;
inline float kSmallStarRadiusMin = 0.2f;
inline float kSmallStarRadiusMax = 0.6f;
inline float kBigStarScreenFrac = 0.9f;
inline float kSmallStarScreenFrac = 0.2f;
inline float kStarScaleVariance = 0.15f;

// Sandevistan
inline float kSandevistanEndSpeedPx = 200.0f;
inline float kSandevistanSpawnIntervalSec = 0.04f;
inline float kSandevistanGhostFadeSec = 0.4f;
inline int kSandevistanGhostStartOpacity = 128;
inline int kSandevistanMaxConcurrentGhosts = 24;
inline int kSandevistanTrailLayerZOrder = 0;
inline int kSandevistanTrailHueOrangeR = 255;
inline int kSandevistanTrailHueOrangeG = 175;
inline int kSandevistanTrailHueOrangeB = 55;
inline int kSandevistanTrailHuePurpleR = 185;
inline int kSandevistanTrailHuePurpleG = 95;
inline int kSandevistanTrailHuePurpleB = 255;
inline int kSandevistanTrailHueCyanR = 80;
inline int kSandevistanTrailHueCyanG = 230;
inline int kSandevistanTrailHueCyanB = 255;

// Fire Aura
inline float kMinFireAuraSpeedPx = 600.0f;
inline float kMaxFireAuraSpeedPx = 2800.0f;
inline float kFireAuraDiameterScale = 2.25f;
inline float kFireAuraVelocityToShader = 0.002f;

inline float kFireAuraDefaultPrimaryR = 1.0f;
inline float kFireAuraDefaultPrimaryG = 0.9f;
inline float kFireAuraDefaultPrimaryB = 0.5f;
inline float kFireAuraDefaultSecondaryR = 0.32f;
inline float kFireAuraDefaultSecondaryG = 0.02f;
inline float kFireAuraDefaultSecondaryB = 0.0f;

// Player Motion Blur
inline float kPlayerMinBlurSpeedPx = 120.0f;
inline float kPlayerMaxBlurSpeedPx = 3200.0f;
inline float kPlayerBlurUvSpread = 0.035f;
inline int kPlayerBlurStepDivisor = 6;
inline bool kPlayerKeepBaseVisible = true;

// Debug Label
inline bool kDebugLabelEnabled = false;
inline float kDebugLabelMarginX = 4.0f;
inline float kDebugLabelMarginY = 4.0f;
constexpr int kDebugLabelZOrder = 6767;
constexpr int kDebugLabelBackgroundZOrder = kDebugLabelZOrder - 1;
constexpr float kDebugLabelUpdateHz = 20.0f;
constexpr float kDebugLabelUpdateInterval = 1.0f / kDebugLabelUpdateHz;
inline float kDebugLabelFontScale = 0.5f;
inline float kDebugLabelBoxPadX = 1.0f;
inline float kDebugLabelBoxPadY = 0.0f;
inline float kDebugLabelBoxColorR = 0.0f;
inline float kDebugLabelBoxColorG = 0.0f;
inline float kDebugLabelBoxColorB = 0.0f;
inline float kDebugLabelBoxAlpha = 0.35f;

// Physics World
inline float kPixelsPerMeter = 50.0f;

inline float kEarthGravity = 9.8f;
inline float kGravityScale = 1.75f;
inline int kWorldIterations = 10;

inline float kWallHalfThickness = 0.5f;
inline float kWallLengthPadding = 4.0f;
inline float kWallThickness = 1.0f;
inline float kArenaCenterFrac = 0.5f;

// Player
inline float kPlayerDensity = 1.0f;
inline float kPlayerInitialXFrac = 0.25f;
inline float kPlayerInitialYFrac = 0.25f;

// Player Initial
inline float kPlayerInitialVelX = 5.0f;
inline float kPlayerInitialVelY = 10.0f;
inline float kPlayerInitialAngularVel = 30.0f;

// Player Physics
inline float kPlayerFriction = 0.4f;
inline float kDragSpring = 220.0f;
inline float kDragDamping = 10.0f;
inline float kDragAngularDamping = 0.2f;
inline float kDefaultDragTargetXFrac = 0.5f;
inline float kDefaultDragTargetYFrac = 0.5f;

// Outside Barrier Slack
inline float kOutsideBarrierSlack = 1.2f;

// Screen Shake Action Tag
inline int kScreenShakeActionTag = 0x6B53484B;

// B2 Max Polygon Vertices
constexpr int kB2MaxPolygonVertices = 16;

// B2 Restitution In Speed Threshold
inline float kB2RestitutionInSpeedThreshold = 3.0f;

// B2 Restitution Coefficient
inline float kB2RestitutionCoefficient = 0.67f;

// B2 Collide Reference Edge Relative Tol
inline float kB2CollideReferenceEdgeRelativeTol = 0.98f;
inline float kB2CollideReferenceEdgeAbsoluteTol = 0.001f;
