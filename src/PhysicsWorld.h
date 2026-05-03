#pragma once

#include <memory>

struct PhysicsState {
    float x, y, angle;
};

struct PhysicsVelocity {
    float vx;
    float vy;
};

struct PhysicsImpactEvent {
    bool triggered = false;
    float preSpeedPx = 0.0f;
    float postSpeedPx = 0.0f;
    float impactSpeedPx = 0.0f;
};

class PhysicsWorld {
public:
    PhysicsWorld(float worldW, float worldH, float bodyW, float bodyH);
    ~PhysicsWorld();
    void step(float dt);
    PhysicsState getPlayerState() const;
    PhysicsState getPlayerRenderState(float alpha) const;

    float getPlayerSpeed() const;
    PhysicsVelocity getPlayerVelocityPixels() const;

    void setDragging(bool on);
    void setDragTargetPixels(float x, float y);
    void setDragGrabOffsetPixels(float offsetX, float offsetY);

    PhysicsImpactEvent consumePlayerImpactAny();

    int getBodyCount() const;
    int getJointCount() const;
    int getArbiterCount() const;

private:
    void clampPlayerToScreenBorder();
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    float m_worldW = 0.0f;
    float m_worldH = 0.0f;
    bool m_dragging = false;
    float m_dragTargetX = 0.0f;
    float m_dragTargetY = 0.0f;
    float m_grabLocalX = 0.0f;
    float m_grabLocalY = 0.0f;
    bool m_wasPlayerAgainstAnyBody = false;
    float m_preStepSpeedPx = 0.0f;
    PhysicsState m_playerPrevRender{};
};
