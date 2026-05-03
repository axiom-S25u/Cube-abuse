#include "OverlayRendering.h"
#include "OverlayShaders.h"
#include "ModTuning.h"
#include "PhysicsWorld.h"

#include <Geode/binding/GameManager.hpp>
#include <Geode/cocos/cocoa/CCArray.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCScene.h>
#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/cocos/platform/CCGL.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/cocos/textures/CCTexture2D.h>
#include <Geode/utils/cocos.hpp>
#include <Geode/utils/random.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>

using namespace geode::prelude;

namespace overlay_rendering {

namespace {

CCGLProgram* createLinkedProgram(char const* vert, char const* frag) {
    Ref<CCGLProgram> p = Ref<CCGLProgram>::adopt(new CCGLProgram());
    if (!p->initWithVertexShaderByteArray(vert, frag)) {
        return nullptr;
    }
    p->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
    p->addAttribute(kCCAttributeNameColor, kCCVertexAttrib_Color);
    p->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);
    if (!p->link()) {
        return nullptr;
    }
    p->updateUniforms();
    p->retain();
    return p.take();
}

CCTexture2D* createOneByOneWhiteTexture() {
    static unsigned char const pixels[4] = {255, 255, 255, 255};
    auto tex = std::unique_ptr<CCTexture2D>(new CCTexture2D());
    if (!tex->initWithData(
            pixels,
            kCCTexture2DPixelFormat_RGBA8888,
            1,
            1,
            CCSizeMake(1, 1)
        )) {
        return nullptr;
    }
    tex->autorelease();
    return tex.release();
}

int objectCompositeOrder(MotionBlurObjectId) {
    return 0;
}

void resetObjectVisualState(MotionBlurObjectCapture& object) {
    if (object.sourceRoot) {
        object.sourceRoot->setVisible(true);
    }
    if (object.blurSprite) {
        object.blurSprite->setVisible(false);
    }
}

} // namespace

void MotionBlurSprite::setBlurUniforms(CCGLProgram* prog, GLint locBlurDir) {
    m_blurProg = prog;
    m_locBlurDir = locBlurDir;
}

void MotionBlurSprite::setBlurStep(float x, float y) {
    m_stepX = x;
    m_stepY = y;
}

void MotionBlurSprite::draw() {
    if (m_blurProg && m_locBlurDir >= 0) {
        m_blurProg->use();
        m_blurProg->setUniformLocationWith2f(m_locBlurDir, m_stepX, m_stepY);
    }
    CCSprite::draw();
}

MotionBlurSprite* MotionBlurSprite::create(CCTexture2D* tex, CCGLProgram* prog, GLint locBlurDir) {
    auto* s = new MotionBlurSprite();
    s->setBlurUniforms(prog, locBlurDir);
    if (s->initWithTexture(tex)) {
        s->autorelease();
        return s;
    }
    delete s;
    return nullptr;
}

void ImpactNoiseSprite::setNoiseUniforms(CCGLProgram* prog, GLint locTime, GLint locAlpha) {
    m_noiseProg = prog;
    m_locTime = locTime;
    m_locAlpha = locAlpha;
}

void ImpactNoiseSprite::setNoiseState(float time, float alpha) {
    m_time = time;
    m_alpha = alpha;
}

void ImpactNoiseSprite::draw() {
    if (m_noiseProg && m_locTime >= 0 && m_locAlpha >= 0) {
        m_noiseProg->use();
        m_noiseProg->setUniformLocationWith1f(m_locTime, m_time);
        m_noiseProg->setUniformLocationWith1f(m_locAlpha, m_alpha);
    }
    CCSprite::draw();
}

ImpactNoiseSprite* ImpactNoiseSprite::create(CCTexture2D* tex, CCGLProgram* prog, GLint locTime, GLint locAlpha) {
    auto* s = new ImpactNoiseSprite();
    s->setNoiseUniforms(prog, locTime, locAlpha);
    if (s->initWithTexture(tex)) {
        s->setColor(ccc3(255, 255, 255));
        s->autorelease();
        return s;
    }
    delete s;
    return nullptr;
}

void FireAuraSprite::setFireUniforms(
    CCGLProgram* prog,
    GLint locVelocity,
    GLint locTime,
    GLint locIntensity,
    GLint locColorPrimary,
    GLint locColorSecondary
) {
    m_fireProg = prog;
    m_locVelocity = locVelocity;
    m_locTime = locTime;
    m_locIntensity = locIntensity;
    m_locColorPrimary = locColorPrimary;
    m_locColorSecondary = locColorSecondary;
}

void FireAuraSprite::setFireState(float velX, float velY, float time, float intensity) {
    m_velX = velX;
    m_velY = velY;
    m_time = time;
    m_intensity = intensity;
}

void FireAuraSprite::setFireColors(ccColor3B primaryRgb, ccColor3B secondaryRgb) {
    constexpr float kInv = 1.0f / 255.0f;
    m_colorPrimaryR = static_cast<float>(primaryRgb.r) * kInv;
    m_colorPrimaryG = static_cast<float>(primaryRgb.g) * kInv;
    m_colorPrimaryB = static_cast<float>(primaryRgb.b) * kInv;
    m_colorSecondaryR = static_cast<float>(secondaryRgb.r) * kInv;
    m_colorSecondaryG = static_cast<float>(secondaryRgb.g) * kInv;
    m_colorSecondaryB = static_cast<float>(secondaryRgb.b) * kInv;
}

void FireAuraSprite::draw() {
    if (m_fireProg && m_locVelocity >= 0 && m_locTime >= 0 && m_locIntensity >= 0 && m_locColorPrimary >= 0
        && m_locColorSecondary >= 0) {
        m_fireProg->use();
        m_fireProg->setUniformLocationWith2f(m_locVelocity, m_velX, m_velY);
        m_fireProg->setUniformLocationWith1f(m_locTime, m_time);
        m_fireProg->setUniformLocationWith1f(m_locIntensity, m_intensity);
        m_fireProg->setUniformLocationWith3f(
            m_locColorPrimary,
            m_colorPrimaryR,
            m_colorPrimaryG,
            m_colorPrimaryB
        );
        m_fireProg->setUniformLocationWith3f(
            m_locColorSecondary,
            m_colorSecondaryR,
            m_colorSecondaryG,
            m_colorSecondaryB
        );
    }
    CCSprite::draw();
}

FireAuraSprite* FireAuraSprite::create(
    CCTexture2D* tex,
    CCGLProgram* prog,
    GLint locVelocity,
    GLint locTime,
    GLint locIntensity,
    GLint locColorPrimary,
    GLint locColorSecondary
) {
    auto* s = new FireAuraSprite();
    s->setFireUniforms(prog, locVelocity, locTime, locIntensity, locColorPrimary, locColorSecondary);
    if (s->initWithTexture(tex)) {
        s->setColor(ccc3(255, 255, 255));
        s->autorelease();
        return s;
    }
    delete s;
    return nullptr;
}

CCGLProgram* createMotionBlurProgram(GLint* outBlurDir) {
    auto* p = createLinkedProgram(shaders::kMotionBlurVert, shaders::kMotionBlurFrag);
    if (!p) {
        return nullptr;
    }
    *outBlurDir = p->getUniformLocationForName("u_blurDir");
    return p;
}

CCGLProgram* createWhiteFlashProgram() {
    return createLinkedProgram(shaders::kMotionBlurVert, shaders::kWhiteFlashFrag);
}

CCGLProgram* createColorInvertProgram() {
    return createLinkedProgram(shaders::kMotionBlurVert, shaders::kColorInvertFrag);
}

CCGLProgram* createImpactNoiseProgram(GLint* outTime, GLint* outAlpha) {
    auto* p = createLinkedProgram(shaders::kMotionBlurVert, shaders::kImpactNoiseFrag);
    if (!p) {
        return nullptr;
    }
    *outTime = p->getUniformLocationForName("u_time");
    *outAlpha = p->getUniformLocationForName("u_alpha");
    return p;
}

CCGLProgram* createFireAuraProgram(
    GLint* outVelocity,
    GLint* outTime,
    GLint* outIntensity,
    GLint* outColorPrimary,
    GLint* outColorSecondary
) {
    auto* p = createLinkedProgram(shaders::kMotionBlurVert, shaders::kFireAuraFrag);
    if (!p) {
        return nullptr;
    }
    *outVelocity = p->getUniformLocationForName("u_velocity");
    *outTime = p->getUniformLocationForName("u_time");
    *outIntensity = p->getUniformLocationForName("u_intensity");
    *outColorPrimary = p->getUniformLocationForName("u_colorPrimary");
    *outColorSecondary = p->getUniformLocationForName("u_colorSecondary");
    return p;
}

ObjectMotionBlurAttachResult attachObjectMotionBlur(
    CCNode* overlayLayer,
    CCSize captureSize,
    CCSize outputSize,
    int outputZOrder,
    std::array<MotionBlurObjectSeed, kMotionBlurObjectCount> const& objectSeeds
) {
    ObjectMotionBlurAttachResult out{};
    if (!overlayLayer || captureSize.width <= 0.0f || captureSize.height <= 0.0f || outputSize.width <= 0.0f
        || outputSize.height <= 0.0f) {
        return out;
    }

    struct Rollback {
        Ref<CCRenderTexture> unifiedTexture{};
        Ref<CCGLProgram> blurProgram{};
        Ref<CCGLProgram> whiteFlashProgram{};
        Ref<CCGLProgram> colorInvertProgram{};
        CCSprite* finalComposite = nullptr;
        CCSprite* whiteFlashSprite = nullptr;
        CCNode* mergeRoot = nullptr;
        bool armed = true;
        ~Rollback() {
            if (!armed) {
                return;
            }
            if (mergeRoot) {
                mergeRoot->removeFromParentAndCleanup(true);
            }
            if (whiteFlashSprite) {
                whiteFlashSprite->removeFromParentAndCleanup(true);
            }
            if (finalComposite) {
                finalComposite->removeFromParentAndCleanup(true);
            }
        }
        void disarm() { armed = false; }
    } rb;

    auto* unifiedTextureRaw = CCRenderTexture::create(
        static_cast<int>(std::ceil(captureSize.width)),
        static_cast<int>(std::ceil(captureSize.height)),
        kCCTexture2DPixelFormat_RGBA8888
    );
    if (!unifiedTextureRaw) {
        return out;
    }
    rb.unifiedTexture = unifiedTextureRaw;
    CCRenderTexture* unifiedTexture = rb.unifiedTexture;

    auto* finalComposite = CCSprite::createWithTexture(unifiedTexture->getSprite()->getTexture());
    if (!finalComposite) {
        return out;
    }
    finalComposite->setID("object-blur-final-composite"_spr);
    finalComposite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    finalComposite->setAnchorPoint({0.0f, 0.0f});
    finalComposite->setPosition({0.0f, 0.0f});
    finalComposite->setVisible(false);
    finalComposite->setFlipY(true);
    {
        float const cw = finalComposite->getContentSize().width;
        float const ch = finalComposite->getContentSize().height;
        finalComposite->setScaleX(cw > 0.0f ? outputSize.width / cw : outputSize.width);
        finalComposite->setScaleY(ch > 0.0f ? outputSize.height / ch : outputSize.height);
    }
    overlayLayer->addChild(finalComposite, outputZOrder);
    rb.finalComposite = finalComposite;

    auto* whiteFlashSprite = CCSprite::createWithTexture(unifiedTexture->getSprite()->getTexture());
    if (!whiteFlashSprite) {
        return out;
    }
    whiteFlashSprite->setID("object-blur-flash-sprite"_spr);
    whiteFlashSprite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    whiteFlashSprite->setAnchorPoint({0.0f, 0.0f});
    whiteFlashSprite->setPosition({0.0f, 0.0f});
    whiteFlashSprite->setVisible(false);
    whiteFlashSprite->setFlipY(true);
    {
        float const cw = whiteFlashSprite->getContentSize().width;
        float const ch = whiteFlashSprite->getContentSize().height;
        whiteFlashSprite->setScaleX(cw > 0.0f ? outputSize.width / cw : outputSize.width);
        whiteFlashSprite->setScaleY(ch > 0.0f ? outputSize.height / ch : outputSize.height);
    }
    overlayLayer->addChild(whiteFlashSprite, outputZOrder);
    rb.whiteFlashSprite = whiteFlashSprite;

    GLint locBlurDir = -1;
    auto* blurProgramRaw = createMotionBlurProgram(&locBlurDir);
    if (!blurProgramRaw || locBlurDir < 0) {
        if (blurProgramRaw) {
            blurProgramRaw->release();
        }
        return out;
    }
    rb.blurProgram = Ref<CCGLProgram>::adopt(blurProgramRaw);
    CCGLProgram* blurProgram = rb.blurProgram;

    auto* whiteFlashProgramRaw = createWhiteFlashProgram();
    if (!whiteFlashProgramRaw) {
        return out;
    }
    rb.whiteFlashProgram = Ref<CCGLProgram>::adopt(whiteFlashProgramRaw);
    CCGLProgram* whiteFlashProgram = rb.whiteFlashProgram;
    whiteFlashSprite->setShaderProgram(whiteFlashProgram);

    auto* colorInvertProgramRaw = createColorInvertProgram();
    if (!colorInvertProgramRaw) {
        return out;
    }
    rb.colorInvertProgram = Ref<CCGLProgram>::adopt(colorInvertProgramRaw);

    auto* mergeRoot = CCNode::create();
    if (!mergeRoot) {
        return out;
    }
    mergeRoot->setID("object-merge-root"_spr);
    mergeRoot->setPosition({0.0f, 0.0f});
    mergeRoot->setVisible(false);
    overlayLayer->addChild(mergeRoot, outputZOrder - 1);
    rb.mergeRoot = mergeRoot;

    for (int i = 0; i < kMotionBlurObjectCount; ++i) {
        auto const& seed = objectSeeds[static_cast<size_t>(i)];
        MotionBlurObjectCapture capture{};
        capture.id = seed.id;
        capture.sourceRoot = seed.sourceRoot;
        capture.enabled = seed.enabled;
        capture.tuning = seed.tuning;

        auto* rt = CCRenderTexture::create(
            static_cast<int>(std::ceil(captureSize.width)),
            static_cast<int>(std::ceil(captureSize.height)),
            kCCTexture2DPixelFormat_RGBA8888
        );
        if (!rt) {
            out.objects[static_cast<size_t>(i)] = capture;
            continue;
        }
        capture.renderTexture = rt;

        auto* objectBlur = MotionBlurSprite::create(rt->getSprite()->getTexture(), blurProgram, locBlurDir);
        if (!objectBlur) {
            capture.renderTexture = nullptr;
            capture.enabled = false;
            out.objects[static_cast<size_t>(i)] = capture;
            continue;
        }
        objectBlur->setID("object-motion-blur-sprite"_spr);
        objectBlur->setShaderProgram(blurProgram);
        objectBlur->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
        objectBlur->setAnchorPoint({0.0f, 0.0f});
        objectBlur->setPosition({0.0f, 0.0f});
        objectBlur->setVisible(true);
        objectBlur->setFlipY(true);
        {
            float const cw = objectBlur->getContentSize().width;
            float const ch = objectBlur->getContentSize().height;
            objectBlur->setScaleX(cw > 0.0f ? captureSize.width / cw : captureSize.width);
            objectBlur->setScaleY(ch > 0.0f ? captureSize.height / ch : captureSize.height);
        }
        mergeRoot->addChild(objectBlur, objectCompositeOrder(capture.id));
        capture.blurSprite = objectBlur;
        out.objects[static_cast<size_t>(i)] = capture;
    }

    rb.disarm();
    // ok reflects shared pipeline only, individual object captures may still be nullptr
    out.ok = true;
    out.blurProgram = rb.blurProgram.take();
    out.whiteFlashProgram = rb.whiteFlashProgram.take();
    out.colorInvertProgram = rb.colorInvertProgram.take();
    out.unifiedMergeTexture = rb.unifiedTexture.take();
    out.mergeRoot = mergeRoot;
    out.finalCompositeSprite = finalComposite;
    out.whiteFlashSprite = whiteFlashSprite;
    return out;
}

FireAuraAttachResult attachFireAura(CCNode* playerRoot, float auraDiameterPx) {
    FireAuraAttachResult out{};
    if (!playerRoot || auraDiameterPx <= 0.0f) {
        return out;
    }

    CCTexture2D* tex = createOneByOneWhiteTexture();
    if (!tex) {
        return out;
    }

    GLint locVelocity = -1;
    GLint locTime = -1;
    GLint locIntensity = -1;
    GLint locColorPrimary = -1;
    GLint locColorSecondary = -1;
    CCGLProgram* program = createFireAuraProgram(
        &locVelocity,
        &locTime,
        &locIntensity,
        &locColorPrimary,
        &locColorSecondary
    );
    if (!program || locVelocity < 0 || locTime < 0 || locIntensity < 0 || locColorPrimary < 0
        || locColorSecondary < 0) {
        if (program) {
            program->release();
        }
        return out;
    }

    FireAuraSprite* sprite = FireAuraSprite::create(
        tex,
        program,
        locVelocity,
        locTime,
        locIntensity,
        locColorPrimary,
        locColorSecondary
    );
    if (!sprite) {
        program->release();
        return out;
    }
    sprite->setID("fire-aura-sprite"_spr);
    sprite->setShaderProgram(program);
    sprite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    float const cw = sprite->getContentSize().width;
    sprite->setScale(cw > 0.0f ? auraDiameterPx / cw : auraDiameterPx);
    sprite->setPosition({0, 0});
    sprite->setVisible(false);
    playerRoot->addChild(sprite, kFireAuraZOrder);

    out.ok = true;
    out.sprite = sprite;
    out.program = program;
    return out;
}

void globalScreenShake(float duration, float strength) {
    using clock = std::chrono::steady_clock;
    static double s_nextShakeAllowedSec = 0.0;
    auto const now = clock::now();
    double const nowSec =
        std::chrono::duration<double>(now.time_since_epoch()).count();
    if (nowSec < s_nextShakeAllowedSec) {
        return;
    }

    CCScene* scene = CCScene::get();
    if (!scene) {
        return;
    }

    CCPoint const base = scene->getPosition();
    scene->stopActionByTag(kScreenShakeActionTag);

    int const intervals = kScreenShakeIntervals;
    float const stepDuration = duration / static_cast<float>(intervals);
    float const totalShakeSec =
        stepDuration * static_cast<float>(intervals + 1);
    s_nextShakeAllowedSec =
        nowSec + static_cast<double>(totalShakeSec + kScreenShakeCooldownExtraSeconds);

    CCArray* actions = CCArray::create();
    for (int i = 0; i < intervals; ++i) {
        float const t = static_cast<float>(i) / static_cast<float>(intervals);
        float const falloff = (1.0f - t) * (1.0f - t);
        float const offX =
            geode::utils::random::generate<float>(kScreenShakeSampleMin, kScreenShakeSampleMax) * strength * falloff;
        float const offY =
            geode::utils::random::generate<float>(kScreenShakeSampleMin, kScreenShakeSampleMax) * strength * falloff;
        actions->addObject(CCMoveTo::create(stepDuration, ccp(base.x + offX, base.y + offY)));
    }
    actions->addObject(CCMoveTo::create(stepDuration, ccp(base.x, base.y)));
    auto* shakeAction = CCSequence::create(actions);
    shakeAction->setTag(kScreenShakeActionTag);
    scene->runAction(shakeAction);
}

void refreshObjectMotionBlurComposite(ObjectMotionBlurRefreshArgs const& args) {
    auto* objects = args.objects;
    CCNode* const mergeRoot = args.mergeRoot;
    CCRenderTexture* const unifiedMergeTexture = args.unifiedMergeTexture;
    CCSprite* const finalCompositeSprite = args.finalCompositeSprite;
    CCSprite* const whiteFlashSprite = args.whiteFlashSprite;
    CCGLProgram* const whiteFlashProgram = args.whiteFlashProgram;
    CCGLProgram* const colorInvertProgram = args.colorInvertProgram;
    ImpactFlashMode const impactFlashMode = args.impactFlashMode;

    if (!objects || !mergeRoot || !unifiedMergeTexture || !finalCompositeSprite) {
        return;
    }

    bool const impactFlashActive = impactFlashMode != ImpactFlashMode::None;
    bool needCapture = impactFlashActive;
    for (auto const& object : *objects) {
        if (!object.enabled || !object.sourceRoot) {
            continue;
        }
        if (object.tuning.alwaysCaptureWhenEnabled) {
            needCapture = true;
            break;
        }
        float const speed = std::hypot(object.velocity.vx, object.velocity.vy);
        if (speed >= object.tuning.minBlurSpeedPx) {
            needCapture = true;
            break;
        }
    }

    if (!needCapture) {
        for (auto& object : *objects) {
            resetObjectVisualState(object);
        }
        finalCompositeSprite->setVisible(false);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
        mergeRoot->setVisible(false);
        return;
    }

    for (auto& object : *objects) {
        if (!object.enabled || !object.sourceRoot || !object.renderTexture || !object.blurSprite) {
            resetObjectVisualState(object);
            continue;
        }
        object.blurSprite->setVisible(true);

        float const speed = std::hypot(object.velocity.vx, object.velocity.vy);
        float const maxSpeed = std::max(object.tuning.maxBlurSpeedPx, object.tuning.minBlurSpeedPx + 1.0f);
        float const normT = std::clamp(speed / maxSpeed, 0.0f, 1.0f);
        float const spreadUv = normT * object.tuning.blurUvSpread;
        float const invSpeed = speed > kMinSpeedForInverse ? 1.0f / speed : 0.0f;
        float const nx = -object.velocity.vx * invSpeed;
        float const ny = -object.velocity.vy * invSpeed;
        int const divisor = std::max(object.tuning.blurStepDivisor, 1);
        float const stepUv = spreadUv * (1.0f / static_cast<float>(divisor));
        object.blurSprite->setBlurStep(nx * stepUv, ny * stepUv);

        object.sourceRoot->setVisible(true);
        object.renderTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
        object.sourceRoot->visit();
        object.renderTexture->end();
        object.sourceRoot->setVisible(object.tuning.keepBaseVisible && !impactFlashActive);
    }

    mergeRoot->setVisible(true);
    unifiedMergeTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
    mergeRoot->visit();
    unifiedMergeTexture->end();
    mergeRoot->setVisible(false);

    if (impactFlashMode == ImpactFlashMode::WhiteSilhouette && whiteFlashSprite && whiteFlashProgram) {
        whiteFlashSprite->setShaderProgram(whiteFlashProgram);
        whiteFlashSprite->setVisible(true);
        finalCompositeSprite->setVisible(false);
    } else if (impactFlashMode == ImpactFlashMode::InvertSilhouette && whiteFlashSprite && colorInvertProgram) {
        whiteFlashSprite->setShaderProgram(colorInvertProgram);
        whiteFlashSprite->setVisible(true);
        finalCompositeSprite->setVisible(false);
    } else {
        finalCompositeSprite->setVisible(true);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
    }
}

void refreshFireAura(FireAuraRefreshArgs const& args) {
    FireAuraSprite* const fireAura = args.fireAura;
    PhysicsWorld* const physics = args.physics;
    float const dt = args.dt;
    ImpactFlashMode const impactFlashMode = args.impactFlashMode;
    float* const fireTime = args.fireTime;

    if (!fireAura || !physics || !fireTime) {
        return;
    }

    bool const impactFlashActive = impactFlashMode != ImpactFlashMode::None;
    if (impactFlashActive) {
        fireAura->setVisible(false);
        return;
    }

    PhysicsVelocity const vel = physics->getPlayerVelocityPixels();
    float const speed = std::hypot(vel.vx, vel.vy);
    float const denom = kMaxFireAuraSpeedPx - kMinFireAuraSpeedPx;
    float intensity = 0.0f;
    if (denom > 1e-5f && speed > kMinFireAuraSpeedPx) {
        intensity = (speed - kMinFireAuraSpeedPx) / denom;
        if (intensity > 1.0f) {
            intensity = 1.0f;
        }
    }

    if (intensity <= 0.0f) {
        fireAura->setVisible(false);
        return;
    }

    *fireTime += dt;
    float const tWrapped = std::fmod(*fireTime, 1000.0f);

    float const vx = vel.vx * kFireAuraVelocityToShader;
    float const vy = vel.vy * kFireAuraVelocityToShader;

    ccColor3B primaryRgb = {255, 255, 255};
    ccColor3B secondaryRgb = {200, 200, 200};
    if (GameManager* gm = GameManager::get()) {
        primaryRgb = gm->colorForIdx(gm->getPlayerColor());
        secondaryRgb = gm->colorForIdx(gm->getPlayerColor2());
    }
    fireAura->setFireColors(primaryRgb, secondaryRgb);
    fireAura->setFireState(vx, vy, tWrapped, intensity);
    fireAura->setVisible(true);
}

void refreshImpactNoise(ImpactNoiseRefreshArgs const& args) {
    ImpactNoiseSprite* const sprite = args.sprite;
    float* const timePtr = args.time;
    if (!sprite || !timePtr) {
        return;
    }

    *timePtr += args.extraTimeSkip;
    if (args.visible) {
        *timePtr += args.dt;
    }

    if (!args.visible) {
        sprite->setVisible(false);
        if (args.compositeSprite) {
            args.compositeSprite->setVisible(false);
        }
        return;
    }

    float const tWrapped = std::fmod(*timePtr, 1000.0f);

    sprite->setNoiseState(tWrapped, args.alpha);

    if (args.renderTexture && args.compositeSprite) {
        args.renderTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
        sprite->setVisible(true);
        sprite->visit();
        sprite->setVisible(false);
        args.renderTexture->end();
        args.compositeSprite->setVisible(true);
    } else {
        sprite->setVisible(true);
    }
}

ImpactNoiseAttachResult attachImpactNoise(CCNode* overlayLayer, CCSize winSize) {
    ImpactNoiseAttachResult out{};
    if (!overlayLayer || winSize.width <= 0.0f || winSize.height <= 0.0f) {
        return out;
    }

    CCTexture2D* tex = createOneByOneWhiteTexture();
    if (!tex) {
        return out;
    }

    GLint locTime = -1;
    GLint locAlpha = -1;
    CCGLProgram* program = createImpactNoiseProgram(&locTime, &locAlpha);
    if (!program || locTime < 0 || locAlpha < 0) {
        if (program) {
            program->release();
        }
        return out;
    }

    ImpactNoiseSprite* sprite = ImpactNoiseSprite::create(tex, program, locTime, locAlpha);
    if (!sprite) {
        program->release();
        return out;
    }
    sprite->setID("impact-noise-sprite"_spr);
    sprite->setShaderProgram(program);
    sprite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    sprite->setAnchorPoint({0.0f, 0.0f});
    sprite->setPosition({0.0f, 0.0f});
    sprite->setVisible(false);

    int const rw = std::max(8, static_cast<int>(std::ceil(winSize.width * kImpactNoiseRenderScale)));
    int const rh = std::max(8, static_cast<int>(std::ceil(winSize.height * kImpactNoiseRenderScale)));

    CCRenderTexture* rtRaw = CCRenderTexture::create(rw, rh, kCCTexture2DPixelFormat_RGBA8888);
    Ref<CCRenderTexture> rtHold{};
    if (rtRaw) {
        rtHold = rtRaw;
    }
    CCSprite* composite = nullptr;
    if (rtHold) {
        composite = CCSprite::createWithTexture(rtHold->getSprite()->getTexture());
        if (composite) {
            composite->setID("impact-noise-composite"_spr);
            composite->setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor));
            composite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
            composite->setAnchorPoint({0.0f, 0.0f});
            composite->setPosition({0.0f, 0.0f});
            float const ccw = composite->getContentSize().width;
            float const cch = composite->getContentSize().height;
            composite->setScaleX(ccw > 0.0f ? winSize.width / ccw : winSize.width);
            composite->setScaleY(cch > 0.0f ? winSize.height / cch : winSize.height);
            composite->setFlipY(true);
            composite->setVisible(false);
            overlayLayer->addChild(composite, kImpactNoiseZOrder);

            float const cw = sprite->getContentSize().width;
            float const ch = sprite->getContentSize().height;
            sprite->setScaleX(cw > 0.0f ? static_cast<float>(rw) / cw : static_cast<float>(rw));
            sprite->setScaleY(ch > 0.0f ? static_cast<float>(rh) / ch : static_cast<float>(rh));
            overlayLayer->addChild(sprite, kImpactNoiseZOrder - 1);
            out.renderTexture = rtHold.take();
            out.compositeSprite = composite;
        }
    }

    if (!out.renderTexture) {
        float const cw = sprite->getContentSize().width;
        float const ch = sprite->getContentSize().height;
        sprite->setScaleX(cw > 0.0f ? winSize.width / cw : winSize.width);
        sprite->setScaleY(ch > 0.0f ? winSize.height / ch : winSize.height);
        overlayLayer->addChild(sprite, kImpactNoiseZOrder);
    }

    out.ok = true;
    out.sprite = sprite;
    out.program = program;
    return out;
}

} // namespace overlay_rendering
