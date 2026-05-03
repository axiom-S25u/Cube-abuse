#include "PhysicsWorld.h"

#include "ModTuning.h"

#include <cfloat>
#include "box2d-lite/World.h"
#include "box2d-lite/Body.h"
#include "box2d-lite/Arbiter.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

using namespace kti_b2l;

static float lerpAngleRad(float aRad, float bRad, float t) {
    float const pi = std::numbers::pi_v<float>;
    float d = bRad - aRad;
    while (d > pi) {
        d -= 2.0f * pi;
    }
    while (d < -pi) {
        d += 2.0f * pi;
    }
    return aRad + d * t;
}

static float bodySpeedPx(Body const& body) {
    return std::hypot(body.velocity.x, body.velocity.y) * kPixelsPerMeter;
}

static float relativeSpeedPx(Body const& a, Body const& b) {
    float const rvx = a.velocity.x - b.velocity.x;
    float const rvy = a.velocity.y - b.velocity.y;
    return std::hypot(rvx, rvy) * kPixelsPerMeter;
}

struct DragTuning {
    float spring = 0.0f;
    float damping = 0.0f;
    float angularDamping = 0.0f;
};

static std::vector<Vec2> makeBoxPolygon(float widthMeters, float heightMeters) {
    float const hx = widthMeters * 0.5f;
    float const hy = heightMeters * 0.5f;
    return {
        Vec2(-hx, -hy),
        Vec2(hx, -hy),
        Vec2(hx, hy),
        Vec2(-hx, hy),
    };
}

static void bodyWorldExtents(Body const& body, float& outXExtent, float& outYExtent) {
    if (body.vertices.empty()) {
        outXExtent = 0.0f;
        outYExtent = 0.0f;
        return;
    }
    Mat22 const rot(body.rotation);
    float maxX = 0.0f;
    float maxY = 0.0f;
    for (Vec2 const& local : body.vertices) {
        Vec2 const w = rot * local;
        maxX = std::max(maxX, std::fabs(w.x));
        maxY = std::max(maxY, std::fabs(w.y));
    }
    outXExtent = maxX;
    outYExtent = maxY;
}

static void applyDragForces(
    Body& body,
    float grabLocalX,
    float grabLocalY,
    float dragTargetX,
    float dragTargetY,
    DragTuning const& tuning
) {
    float const th = body.rotation;
    float const c = std::cos(th);
    float const s = std::sin(th);
    float const rx = c * grabLocalX - s * grabLocalY;
    float const ry = s * grabLocalX + c * grabLocalY;
    float const gpx = body.position.x + rx;
    float const gpy = body.position.y + ry;
    float const tx = dragTargetX / kPixelsPerMeter;
    float const ty = dragTargetY / kPixelsPerMeter;
    float const ex = tx - gpx;
    float const ey = ty - gpy;
    float const vpx = body.velocity.x - body.angularVelocity * ry;
    float const vpy = body.velocity.y + body.angularVelocity * rx;
    float const forceX = tuning.spring * ex - tuning.damping * vpx;
    float const forceY = tuning.spring * ey - tuning.damping * vpy;
    body.AddForce(Vec2(forceX, forceY));
    body.torque += rx * forceY - ry * forceX;
    body.torque -= tuning.angularDamping * body.angularVelocity;
}

static void clampBodyToScreenBorder(Body& body, float worldWidthMeters, float worldHeightMeters) {
    float ex = 0.0f;
    float ey = 0.0f;
    bodyWorldExtents(body, ex, ey);

    float const left = body.position.x - ex;
    float const right = body.position.x + ex;
    float const bottom = body.position.y - ey;
    float const top = body.position.y + ey;

    float const padX = worldWidthMeters * (kOutsideBarrierSlack - 1.0f);
    float const padY = worldHeightMeters * (kOutsideBarrierSlack - 1.0f);

    float const snapMinX = ex;
    float const snapMaxX = worldWidthMeters - ex;
    float const snapMinY = ey;
    float const snapMaxY = worldHeightMeters - ey;

    if (snapMinX <= snapMaxX) {
        if (left < -padX) {
            body.position.x = snapMinX;
        } else if (right > worldWidthMeters + padX) {
            body.position.x = snapMaxX;
        }
    } else {
        body.position.x = worldWidthMeters * 0.5f;
    }

    if (snapMinY <= snapMaxY) {
        if (bottom < -padY) {
            body.position.y = snapMinY;
        } else if (top > worldHeightMeters + padY) {
            body.position.y = snapMaxY;
        }
    } else {
        body.position.y = worldHeightMeters * 0.5f;
    }
}

struct PhysicsWorld::Impl {
    World world;
    Body wallBottom;
    Body wallTop;
    Body wallLeft;
    Body wallRight;
    Body player;

    Impl(float worldW, float worldH, float bodyW, float bodyH)
        : world(Vec2(0.0f, -kEarthGravity * kGravityScale), kWorldIterations)
    {
        float ww = worldW / kPixelsPerMeter;
        float wh = worldH / kPixelsPerMeter;
        float bw = bodyW / kPixelsPerMeter;
        float bh = bodyH / kPixelsPerMeter;
        std::vector<Vec2> const horizontalWallPoly = makeBoxPolygon(ww + kWallLengthPadding, kWallThickness);
        std::vector<Vec2> const verticalWallPoly = makeBoxPolygon(kWallThickness, wh + kWallLengthPadding);
        std::vector<Vec2> const playerPoly = makeBoxPolygon(bw, bh);

        wallBottom.Set(horizontalWallPoly.data(), static_cast<int>(horizontalWallPoly.size()), FLT_MAX);
        wallBottom.position.Set(ww * kArenaCenterFrac, -kWallHalfThickness);

        wallTop.Set(horizontalWallPoly.data(), static_cast<int>(horizontalWallPoly.size()), FLT_MAX);
        wallTop.position.Set(ww * kArenaCenterFrac, wh + kWallHalfThickness);

        wallLeft.Set(verticalWallPoly.data(), static_cast<int>(verticalWallPoly.size()), FLT_MAX);
        wallLeft.position.Set(-kWallHalfThickness, wh * kArenaCenterFrac);

        wallRight.Set(verticalWallPoly.data(), static_cast<int>(verticalWallPoly.size()), FLT_MAX);
        wallRight.position.Set(ww + kWallHalfThickness, wh * kArenaCenterFrac);

        player.Set(playerPoly.data(), static_cast<int>(playerPoly.size()), kPlayerDensity);
        player.position.Set(ww * kPlayerInitialXFrac, wh * kPlayerInitialYFrac);
        player.velocity.Set(kPlayerInitialVelX, kPlayerInitialVelY);
        player.angularVelocity = kPlayerInitialAngularVel;
        player.friction = kPlayerFriction;

        world.Add(&wallBottom);
        world.Add(&wallTop);
        world.Add(&wallLeft);
        world.Add(&wallRight);
        world.Add(&player);
    }
};

PhysicsWorld::PhysicsWorld(float worldW, float worldH, float bodyW, float bodyH)
    : m_impl(std::make_unique<Impl>(worldW, worldH, bodyW, bodyH))
    , m_worldW(worldW)
    , m_worldH(worldH)
    , m_dragging(false)
    , m_dragTargetX(worldW * kDefaultDragTargetXFrac)
    , m_dragTargetY(worldH * kDefaultDragTargetYFrac)
{
    m_playerPrevRender = getPlayerState();
}

PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::clampPlayerToScreenBorder() {
    float const ww = m_worldW / kPixelsPerMeter;
    float const wh = m_worldH / kPixelsPerMeter;
    clampBodyToScreenBorder(m_impl->player, ww, wh);
}

void PhysicsWorld::setDragging(bool on) {
    m_dragging = on;
}

void PhysicsWorld::setDragGrabOffsetPixels(float offsetX, float offsetY) {
    Body const& p = m_impl->player;
    float const wx = offsetX / kPixelsPerMeter;
    float const wy = offsetY / kPixelsPerMeter;
    float const c = std::cos(p.rotation);
    float const s = std::sin(p.rotation);
    m_grabLocalX = c * wx + s * wy;
    m_grabLocalY = -s * wx + c * wy;
}

void PhysicsWorld::setDragTargetPixels(float x, float y) {
    float const minX = 0.0f;
    float const minY = 0.0f;
    float const maxX = m_worldW;
    float const maxY = m_worldH;
    if (x < minX) {
        x = minX;
    } else if (x > maxX) {
        x = maxX;
    }
    if (y < minY) {
        y = minY;
    } else if (y > maxY) {
        y = maxY;
    }
    m_dragTargetX = x;
    m_dragTargetY = y;
}

void PhysicsWorld::step(float dt) {
    if (!std::isfinite(dt) || dt <= 0.0f) {
        return;
    }

    m_playerPrevRender = getPlayerState();

    if (m_dragging) {
        applyDragForces(
            m_impl->player,
            m_grabLocalX,
            m_grabLocalY,
            m_dragTargetX,
            m_dragTargetY,
            DragTuning{kDragSpring, kDragDamping, kDragAngularDamping}
        );
    }

    m_preStepSpeedPx = bodySpeedPx(m_impl->player);

    m_impl->world.Step(dt);
    clampPlayerToScreenBorder();
}

PhysicsState PhysicsWorld::getPlayerState() const {
    return {
        m_impl->player.position.x * kPixelsPerMeter,
        m_impl->player.position.y * kPixelsPerMeter,
        m_impl->player.rotation
    };
}

PhysicsState PhysicsWorld::getPlayerRenderState(float alpha) const {
    PhysicsState const curr = getPlayerState();
    float const a = std::clamp(alpha, 0.0f, 1.0f);
    float const om = 1.0f - a;
    return {
        m_playerPrevRender.x * om + curr.x * a,
        m_playerPrevRender.y * om + curr.y * a,
        lerpAngleRad(m_playerPrevRender.angle, curr.angle, a),
    };
}

float PhysicsWorld::getPlayerSpeed() const {
    return bodySpeedPx(m_impl->player);
}

PhysicsVelocity PhysicsWorld::getPlayerVelocityPixels() const {
    Vec2 const& v = m_impl->player.velocity;
    return { v.x * kPixelsPerMeter, v.y * kPixelsPerMeter };
}

PhysicsImpactEvent PhysicsWorld::consumePlayerImpactAny() {
    Body* const player = &m_impl->player;
    bool now = false;
    float maxRelativeSpeedPx = 0.0f;
    for (auto const& kv : m_impl->world.arbiters) {
        Arbiter const& arb = kv.second;
        if (arb.numContacts <= 0) {
            continue;
        }
        if (arb.body1 == player || arb.body2 == player) {
            now = true;
            Body* const other = arb.body1 == player ? arb.body2 : arb.body1;
            if (other) {
                maxRelativeSpeedPx = std::max(maxRelativeSpeedPx, relativeSpeedPx(*player, *other));
            }
        }
    }

    PhysicsImpactEvent event{};
    event.triggered = now && !m_wasPlayerAgainstAnyBody;
    event.preSpeedPx = m_preStepSpeedPx;
    event.postSpeedPx = getPlayerSpeed();
    event.impactSpeedPx = std::max({m_preStepSpeedPx, event.postSpeedPx, maxRelativeSpeedPx});
    m_wasPlayerAgainstAnyBody = now;
    return event;
}

int PhysicsWorld::getBodyCount() const {
    return static_cast<int>(m_impl->world.bodies.size());
}

int PhysicsWorld::getJointCount() const {
    return static_cast<int>(m_impl->world.joints.size());
}

int PhysicsWorld::getArbiterCount() const {
    return static_cast<int>(m_impl->world.arbiters.size());
}
