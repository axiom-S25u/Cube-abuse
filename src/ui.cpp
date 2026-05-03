#include "ui.hpp"
#include "bg.hpp"
#include "OverlayRendering.h"
#include "vfx/ImpactFlash.h"
#include "vfx/ImpactNoise.h"
#include "vfx/ObjectMotionBlurPipeline.h"
#include <Geode/modify/MenuLayer.hpp>
#include <cstdlib>
#include <chrono>

// ui.hpp handles the common globals with inline now
// axiom was here again
static float g_cubeMaxHp = 100.0f;
static float g_cubeHp = 100.0f;
static bool g_cubeAlive = true;
static bool g_respawning = false;

static double getRealTime() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() / 1000.0;
}

class SandboxLayer : public CCLayer {
public:
    SimplePlayer* iconPlayer = nullptr;
    CCLayerColor* hpBg = nullptr;
    CCLayerColor* hpFill = nullptr;
    CCLabelBMFont* hpText = nullptr;
    CCLabelBMFont* coinText = nullptr;
    CCLabelBMFont* nukeWarn = nullptr;
    CCLabelBMFont* nukeCoolLabel = nullptr;
    CCLabelBMFont* speedLabel = nullptr;
    WallHitVfxState wallHitVfx;
    vfx::ImpactFlashState impactFlash;
    vfx::ImpactNoiseState impactNoise;
    vfx::ObjectMotionBlurPipelineState objectBlur;
    std::vector<WepEnt> ents;
    float floorY;
    float wallL;
    float wallR;
    bool shopOpen;
    CCPoint prevDrag;
    CCPoint dragVel;

    static SandboxLayer* create() {
        SandboxLayer* ret = new SandboxLayer();
        if (ret && ret->init()) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        shopOpen = false;
        g_cubeMaxHp = calcMaxHp();
        g_cubeHp = g_cubeMaxHp;
        g_cubeAlive = true;
        g_respawning = false;
        floorY = 60.0f;
        wallL = 30.0f;
        wallR = winSize.width - 30.0f;
        prevDrag = ccp(0, 0);
        dragVel = ccp(0, 0);

        CCLayerColor* bg = CCLayerColor::create(ccc4(18, 8, 32, 255));
        bg->setContentSize(winSize);
        this->addChild(bg, topZ(this) + 1);

        g_bgFile = Mod::get()->getSavedValue<std::string>("bg_file", "");
        loadBgToLayer(this, topZ(this) + 1);

        spawnCube();
        setupHpBar();
        refreshHpBar();
        setupCoinHud();
        refreshCoinHud();
        setupWeapButtons();
        setupCloseButton();
        setupResetButton();
        setupNukeCooldownLabel();
        setupWallHitVfx();

        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);
        this->schedule(schedule_selector(SandboxLayer::physicsUpdate), 1.0f / 60.0f);
        return true;
    }

    void registerWithTouchDispatcher() override {
        CCTouchDispatcher::get()->addTargetedDelegate(this, -128, true);
    }

    void keyBackClicked() override {
        onClose(nullptr);
    }

    void onExit() override {
        CCTouchDispatcher::get()->removeDelegate(this);
        CCLayer::onExit();
    }

    void spawnCube() {
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        if (!g_customImagePath.empty()) {
            std::error_code ec;
            if (std::filesystem::exists(g_customImagePath, ec)) {
                geode::Result<geode::ByteVector> res = file::readBinary(g_customImagePath);
                if (res.isOk()) {
                    geode::ByteVector bytes = res.unwrap();
                    CCImage* img = new CCImage();
                    if (img->initWithImageData((void*)bytes.data(), (int)bytes.size())) {
                        CCTexture2D* tex = new CCTexture2D();
                        tex->initWithImage(img);
                        img->release();
                        CCSprite* customSpr = CCSprite::createWithTexture(tex);
                        tex->release();
                        if (customSpr) {
                            customSpr->setScale(2.4f);
                            customSpr->setPosition(ccp(winSize.width * 0.5f, floorY + 85.0f));
                            customSpr->setID("custom-cube-spr");
                            this->addChild(customSpr, topZ(this) + 1);
                            iconPlayer = SimplePlayer::create(0);
                            iconPlayer->setVisible(false);
                            this->addChild(iconPlayer, topZ(this) + 1);
                            g_bodyInit = true;
                            g_body.node = customSpr;
                            g_body.vel = ccp(0.0f, 0.0f);
                            g_body.pos = customSpr->getPosition();
                            g_body.angVel = 0.0f;
                            g_body.angle = 0.0f;
                            g_body.dragging = false;
                            configureMotionBlurSource();
                            g_cubeMaxHp = calcMaxHp();
                            g_cubeHp = g_cubeMaxHp;
                            g_cubeAlive = true;
                            return;
                        }
                    } else {
                        img->release();
                    }
                }
            }
        }
        GameManager* gm = GameManager::sharedState();
        iconPlayer = SimplePlayer::create(0);
        iconPlayer->updatePlayerFrame(1, IconType::Cube);
        iconPlayer->setColor(gm->colorForIdx(0));
        iconPlayer->setSecondColor(gm->colorForIdx(3));
        iconPlayer->updateColors();
        iconPlayer->setScale(2.4f);
        iconPlayer->setPosition(ccp(winSize.width * 0.5f, floorY + 85.0f));
        this->addChild(iconPlayer, topZ(this) + 1);
        g_bodyInit = true;
        g_body.node = iconPlayer;
        g_body.vel = ccp(0.0f, 0.0f);
        g_body.pos = iconPlayer->getPosition();
        g_body.angVel = 0.0f;
        g_body.angle = 0.0f;
        g_body.dragging = false;
        configureMotionBlurSource();
        g_cubeMaxHp = calcMaxHp();
        g_cubeHp = g_cubeMaxHp;
        g_cubeAlive = true;
    }

    void setupHpBar() {
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        hpBg = CCLayerColor::create(ccc4(60, 10, 10, 200), 160.0f, 14.0f);
        hpBg->setPosition(ccp(winSize.width * 0.5f - 80.0f, winSize.height - 46.0f));
        this->addChild(hpBg, topZ(this) + 1);
        hpFill = CCLayerColor::create(ccc4(60, 220, 60, 255), 160.0f, 14.0f);
        hpFill->setPosition(hpBg->getPosition());
        this->addChild(hpFill, topZ(this) + 1);
        hpText = CCLabelBMFont::create("100/100", "chatFont.fnt");
        hpText->setScale(0.5f);
        hpText->setPosition(ccp(winSize.width * 0.5f, winSize.height - 39.0f));
        this->addChild(hpText, topZ(this) + 1);
    }

    void refreshHpBar() {
        if (!hpFill || !hpText) return;
        float ratio = g_cubeHp / g_cubeMaxHp;
        if (ratio < 0.0f) ratio = 0.0f;
        hpFill->setContentSize(CCSizeMake(160.0f * ratio, 14.0f));
        hpText->setString(fmt::format("{}/{}", (int)g_cubeHp, (int)g_cubeMaxHp).c_str());
    }

    void setupCoinHud() {
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        coinText = CCLabelBMFont::create("$0", "chatFont.fnt");
        coinText->setScale(0.62f);
        coinText->setColor(ccc3(255, 215, 0));
        coinText->setPosition(ccp(winSize.width * 0.5f, winSize.height - 62.0f));
        this->addChild(coinText, topZ(this) + 1);
    }

    void refreshCoinHud() {
        if (coinText) coinText->setString(fmt::format("${}", g_coins).c_str());
    }

    void setupNukeCooldownLabel() {
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        nukeCoolLabel = CCLabelBMFont::create("", "chatFont.fnt");
        nukeCoolLabel->setScale(0.3f);
        nukeCoolLabel->setColor(ccc3(255, 100, 100));
        nukeCoolLabel->setPosition(ccp(winSize.width - 110.0f, 40.0f));
        this->addChild(nukeCoolLabel, topZ(this) + 1);
    }

    void dealDmg(float dmg) {
        if (!g_cubeAlive || g_respawning) return;
        g_cubeHp -= dmg;
        if (g_cubeHp < 0.0f) g_cubeHp = 0.0f;
        refreshHpBar();
        if (g_cubeHp <= 0.0f) killCube();
    }

    void killCube() {
        if (!g_cubeAlive || !iconPlayer) return;
        g_cubeAlive = false;
        g_respawning = true;
        g_bodyInit = false;
        g_body.dragging = false;
        g_coins += killReward(g_cubeMaxHp);
        refreshCoinHud();
        saveWeaps();
        iconPlayer->runAction(CCSequence::create(
            CCFadeOut::create(0.4f),
            CCCallFunc::create(this, callfunc_selector(SandboxLayer::doRespawn)),
            nullptr
        ));
    }

    void doRespawn() {
        if (g_body.node && g_body.node != iconPlayer) {
            g_body.node->removeFromParent();
        }
        g_body.node = nullptr;
        if (iconPlayer) { iconPlayer->removeFromParent(); iconPlayer = nullptr; }
        g_respawning = false;
        spawnCube();
        refreshHpBar();
    }

    void onReset(CCObject*) {
        if (g_respawning || !iconPlayer) return;
        g_body.dragging = false;
        g_bodyInit = false;
        iconPlayer->stopAllActions();
        if (g_body.node && g_body.node != iconPlayer) {
            g_body.node->removeFromParent();
        }
        g_body.node = nullptr;
        iconPlayer->removeFromParent();
        iconPlayer = nullptr;
        g_cubeAlive = false;
        this->doRespawn();
    }

    void setupResetButton() {
        CCMenu* m = CCMenu::create();
        m->setPosition(ccp(0, 0));
        ButtonSprite* bspr = ButtonSprite::create("RESET", 0, false, "goldFont.fnt", "GJ_button_06.png", 30.0f, 0.55f);
        CCMenuItemSpriteExtra* b = CCMenuItemSpriteExtra::create(bspr, this, menu_selector(SandboxLayer::onReset));
        b->setPosition(ccp(70, 30));
        m->addChild(b, topZ(m) + 1);
        this->addChild(m, topZ(this) + 1);
    }

    void setupWeapButtons() {
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        CCMenu* weapMenu = CCMenu::create();
        weapMenu->setPosition(ccp(0.0f, 0.0f));
        this->addChild(weapMenu, topZ(this) + 1);
        float btnX = winSize.width - 45.0f;
        float btnXLeft = 45.0f;
        float startY = winSize.height - 60.0f;
        for (int i = 0; i < WEP_COUNT; i++) {
            ButtonSprite* bspr = ButtonSprite::create(g_weaps[i].name, 0, false, "goldFont.fnt", "GJ_button_01.png", 30.0f, 0.25f);
            CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(bspr, this, menu_selector(SandboxLayer::onWeapon));
            if (i == 7) { // NUKE
                btn->setPosition(ccp(winSize.width - 110.0f, 20.0f));
            } else if (i >= 8) {
                btn->setPosition(ccp(btnXLeft, startY - 34.0f * (float)(i - 8)));
            } else {
                btn->setPosition(ccp(btnX, startY - 34.0f * (float)i));
            }
            btn->setTag(i);
            weapMenu->addChild(btn, topZ(weapMenu) + 1);
        }
        ButtonSprite* shopSpr = ButtonSprite::create("SHOP", 0, false, "goldFont.fnt", "GJ_button_02.png", 30.0f, 0.5f);
        CCMenuItemSpriteExtra* shopBtn = CCMenuItemSpriteExtra::create(shopSpr, this, menu_selector(SandboxLayer::onShop));
        shopBtn->setPosition(ccp(btnX, 20.0f));
        weapMenu->addChild(shopBtn, topZ(weapMenu) + 1);

        ButtonSprite* iconSpr = ButtonSprite::create("ICON", 0, false, "goldFont.fnt", "GJ_button_05.png", 30.0f, 0.45f);
        CCMenuItemSpriteExtra* iconBtn = CCMenuItemSpriteExtra::create(iconSpr, this, menu_selector(SandboxLayer::onIconCustomizer));
        iconBtn->setPosition(ccp(winSize.width - 75.0f, winSize.height - 25.0f));
        weapMenu->addChild(iconBtn, topZ(weapMenu) + 1);

        ButtonSprite* resetSpr = ButtonSprite::create("RESET", 0, false, "goldFont.fnt", "GJ_button_06.png", 30.0f, 0.32f);
        CCMenuItemSpriteExtra* resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(SandboxLayer::onResetIcon));
        resetBtn->setPosition(ccp(winSize.width - 115.0f, winSize.height - 25.0f));
        weapMenu->addChild(resetBtn, topZ(weapMenu) + 1);

        ButtonSprite* bgSpr = ButtonSprite::create("BG", 0, false, "goldFont.fnt", "GJ_button_04.png", 30.0f, 0.45f);
        CCMenuItemSpriteExtra* bgBtn = CCMenuItemSpriteExtra::create(bgSpr, this, menu_selector(SandboxLayer::onBgCustomizer));
        bgBtn->setPosition(ccp(winSize.width - 35.0f, winSize.height - 25.0f));
        weapMenu->addChild(bgBtn, topZ(weapMenu) + 1);
    }

    void onResetIcon(CCObject*) {
        g_customImagePath.clear();
        saveWeaps();
        if (g_bodyInit || iconPlayer || g_body.node) {
            doRespawn();
        }
    }

    void onBgCustomizer(CCObject*) {
        BgPicker* picker = BgPicker::create([this]() {
            removeBgFromLayer(this);
            loadBgToLayer(this, 2);
        });
        picker->show();
    }

    void onIconCustomizer(CCObject*) {
        async::spawn(file::pick(file::PickMode::OpenFile, {
            .filters = {{ "Images", { "*.png", "*.jpg", "*.jpeg", "*.webp", "*.bmp" } }}
        }), [this](Result<std::optional<std::filesystem::path>> result) {
            if (result.isOk()) {
                if (auto path = result.unwrap()) {
                    g_customImagePath = geode::utils::string::pathToString(*path);
                    saveWeaps();
                    if (iconPlayer) {
                        iconPlayer->removeFromParent();
                        iconPlayer = nullptr;
                    }
                    CCNode* oldCustom = this->getChildByID("custom-cube-spr");
                    if (oldCustom) oldCustom->removeFromParent();
                    g_bodyInit = false;
                    spawnCube();
                }
            }
        });
    }

    void onWeapon(CCObject* sender) {
        int idx = sender->getTag();
        if (!g_weapUnlock[idx]) {
            FLAlertLayer::create("Locked", "Buy this in the shop!", "OK")->show();
            return;
        }
        if (idx == 7) {
            double now = getRealTime();
            double elapsed = now - g_lastNuke;
            if (elapsed < 300.0) {
                int secsLeft = (int)(300.0 - elapsed);
                FLAlertLayer::create("Chill", fmt::format("Nuke cooldown: {}s remaining", secsLeft).c_str(), "OK")->show();
                return;
            }
            startNuke();
            return;
        }
        WepEnt ent;
        ent.id = idx;
        ent.spr = CCSprite::create(g_weaps[idx].iconName);
        if (!ent.spr) ent.spr = CCSprite::createWithSpriteFrameName(g_weaps[idx].iconName);
        if (!ent.spr) ent.spr = CCSprite::createWithSpriteFrameName("edit_eBtn_001.png");
        ent.spr->setColor(g_weaps[idx].color);
        ent.spr->setPosition(ccp(100, 100));
        ent.dragging = false; ent.hasHit = false; ent.idleTimer = 0.0f; ent.dying = false;
        this->addChild(ent.spr, topZ(this) + 1);
        ents.push_back(ent);
    }

    void startNuke() {
        g_lastNuke = getRealTime();
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        nukeWarn = CCLabelBMFont::create("NUKE INCOMING!!!!!!!!!!!!", "bigFont.fnt");
        nukeWarn->setPosition(ccp(winSize.width / 2, winSize.height / 2));
        this->addChild(nukeWarn, topZ(this) + 1);
        nukeWarn->runAction(CCRepeatForever::create(CCSequence::create(
            CCTintTo::create(0.1f, 255, 0, 0),
            CCTintTo::create(0.1f, 255, 255, 255),
            nullptr
        )));
        this->scheduleOnce(schedule_selector(SandboxLayer::nukeExplode), 3.5f);
    }

    void nukeExplode(float) {
        if (nukeWarn) { nukeWarn->removeFromParent(); nukeWarn = nullptr; }
        CCLayerColor* flash = CCLayerColor::create(ccc4(255, 255, 255, 255));
        this->addChild(flash, topZ(this) + 1);
        flash->runAction(CCSequence::create(CCFadeOut::create(1.2f), CCRemoveSelf::create(), nullptr));
        FMODAudioEngine::sharedEngine()->playEffect("axiom.cube-abuse/boom.mp3"); // from deltarune btw lmao
        dealDmg(9999999.0f);
        g_coins += 1500 + rand() % 1000;
        refreshCoinHud(); saveWeaps();
    }

    void onShop(CCObject*) {
        if (shopOpen) return;
        shopOpen = true;
        ShopLayer* shop = ShopLayer::create(
            [this]() { shopOpen = false; saveWeaps(); this->refreshCoinHud(); },
            [this]() { this->refreshCoinHud(); }
        );
        shop->show();
    }

    void setupCloseButton() {
        CCMenu* m = CCMenu::create(); m->setPosition(ccp(0, 0));
        CCMenuItemSpriteExtra* b = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"), this, menu_selector(SandboxLayer::onClose));
        b->setPosition(ccp(25, CCDirector::sharedDirector()->getWinSize().height - 25));
        m->addChild(b, topZ(m) + 1); this->addChild(m, topZ(this) + 1);

        // Speed label
        speedLabel = CCLabelBMFont::create("0.0", "bigFont.fnt");
        speedLabel->setPosition(ccp(60, CCDirector::sharedDirector()->getWinSize().height - 25));
        speedLabel->setScale(0.5f);
        speedLabel->setAnchorPoint(ccp(0, 0.5f));
        this->addChild(speedLabel, topZ(this) + 1);
    }

    void setupWallHitVfx() {
        if (wallHitVfx.layer) return;
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        wallHitVfx.layer = CCNode::create();
        this->addChild(wallHitVfx.layer, topZ(this) + 1);

        // Use real kicktheiconlite impact noise system
        auto noiseResult = overlay_rendering::attachImpactNoise(wallHitVfx.layer, winSize);
        if (noiseResult.ok) {
            impactNoise.sprite = noiseResult.sprite;
            impactNoise.renderTexture = Ref<CCRenderTexture>::adopt(noiseResult.renderTexture);
            impactNoise.composite = noiseResult.compositeSprite;
            impactNoise.program = Ref<CCGLProgram>::adopt(noiseResult.program);
        }

        // Simple flash layer for hit feedback
        wallHitVfx.flash = CCLayerColor::create(ccc4(255, 255, 255, 0));
        wallHitVfx.flash->setContentSize(winSize);
        wallHitVfx.flash->setOpacity(0);
        wallHitVfx.flash->setVisible(false);
        wallHitVfx.layer->addChild(wallHitVfx.flash, topZ(wallHitVfx.layer) + 1);
    }

    void configureMotionBlurSource() {
        if (!g_body.node) return;
        if (!objectBlur.mergeRoot) {
            std::array<overlay_rendering::MotionBlurObjectSeed, overlay_rendering::kMotionBlurObjectCount> seeds{};
            seeds[0].id = overlay_rendering::MotionBlurObjectId::Player;
            seeds[0].sourceRoot = g_body.node;
            seeds[0].enabled = true;
            seeds[0].tuning.minBlurSpeedPx = kPlayerMinBlurSpeedPx;
            seeds[0].tuning.maxBlurSpeedPx = kPlayerMaxBlurSpeedPx;
            seeds[0].tuning.blurUvSpread = kPlayerBlurUvSpread;
            seeds[0].tuning.blurStepDivisor = kPlayerBlurStepDivisor;
            seeds[0].tuning.keepBaseVisible = kPlayerKeepBaseVisible;
            seeds[0].tuning.alwaysCaptureWhenEnabled = true;
            CCSize winSize = CCDirector::sharedDirector()->getWinSize();
            vfx::object_motion_blur::attach(objectBlur, this, winSize, winSize, topZ(this) + 1, seeds);
        } else {
            objectBlur.objects[0].sourceRoot = g_body.node;
            objectBlur.objects[0].enabled = true;
        }
    }

    void finishWallFlash() {
        if (!wallHitVfx.flash) return;
        wallHitVfx.flash->stopAllActions();
        wallHitVfx.flash->setOpacity(0);
        wallHitVfx.flash->setVisible(false);
    }

    void syncBodyVisuals() {
        if (iconPlayer) {
            iconPlayer->setPosition(g_body.pos);
            iconPlayer->setRotation(-CC_RADIANS_TO_DEGREES(g_body.angle));
        }
        if (g_body.node && g_body.node != iconPlayer) {
            g_body.node->setPosition(g_body.pos);
            g_body.node->setRotation(-CC_RADIANS_TO_DEGREES(g_body.angle));
        }
    }

    CCNode* cloneBodyVisual() const {
        if (!g_body.node) return nullptr;
        CCSprite* src = typeinfo_cast<CCSprite*>(g_body.node);
        if (!src) {
            SimplePlayer* playerClone = SimplePlayer::create(0);
            if (!playerClone) return nullptr;
            GameManager* gm = GameManager::sharedState();
            playerClone->updatePlayerFrame(1, IconType::Cube);
            playerClone->setColor(gm->colorForIdx(0));
            playerClone->setSecondColor(gm->colorForIdx(3));
            playerClone->updateColors();
            playerClone->setScale(g_body.node->getScale());
            playerClone->setScaleX(g_body.node->getScaleX());
            playerClone->setScaleY(g_body.node->getScaleY());
            playerClone->setRotation(g_body.node->getRotation());
            return playerClone;
        }
        CCSprite* clone = nullptr;
        if (src->displayFrame()) {
            clone = CCSprite::createWithSpriteFrame(src->displayFrame());
        }
        if (!clone) {
            clone = CCSprite::createWithTexture(src->getTexture(), src->getTextureRect());
        }
        if (!clone) return nullptr;
        clone->setScale(src->getScale());
        clone->setScaleX(src->getScaleX());
        clone->setScaleY(src->getScaleY());
        clone->setColor(src->getColor());
        clone->setOpacity(src->getOpacity());
        clone->setFlipX(src->isFlipX());
        clone->setFlipY(src->isFlipY());
        clone->setRotation(src->getRotation());
        return clone;
    }

    void spawnWallHitVfx(CCPoint const& hitPos, CCPoint const& hitVel, bool isFloor) {
        if (!wallHitVfx.layer) return;
        if (impactFlash.impactFlashCooldownRemaining <= 0.0f) {
            impactFlash.hitstopRemaining = kImpactHitstopSeconds;
            impactFlash.whiteFlashRemaining = kImpactFlashTotalSeconds;
            impactFlash.impactFlashCooldownRemaining = kImpactFlashCooldownSeconds;
            impactNoise.remaining = kImpactNoiseFadeSeconds;
        } else if (impactNoise.remaining > 0.0f) {
            impactNoise.extraTimeSkip += kImpactNoiseStackedImpactTimeSkip;
        }
        if (wallHitVfx.flash) {
            wallHitVfx.flash->stopAllActions();
            wallHitVfx.flash->setVisible(true);
            wallHitVfx.flash->setOpacity(isFloor ? 160 : 200);
            wallHitVfx.flash->runAction(CCSequence::create(
                CCFadeTo::create(0.04f, isFloor ? 120 : 140),
                CCFadeOut::create(0.14f),
                CCCallFunc::create(this, callfunc_selector(SandboxLayer::finishWallFlash)),
                nullptr
            ));
        }

        CCNode* base = cloneBodyVisual();
        if (!base) return;
        if (auto* rgba = typeinfo_cast<CCNodeRGBA*>(base)) {
            rgba->setOpacity(220);
        }

        float vx = hitVel.x;
        float vy = hitVel.y;
        float mag = sqrtf(vx * vx + vy * vy);
        if (mag < 0.001f) mag = 1.0f;
        float nx = vx / mag;
        float ny = vy / mag;

        base->setPosition(hitPos);
        wallHitVfx.layer->addChild(base, topZ(wallHitVfx.layer) + 1);
        base->runAction(CCSequence::create(
            CCSpawn::create(
                CCFadeOut::create(0.16f),
                CCScaleTo::create(0.16f, base->getScale() * 1.05f),
                nullptr
            ),
            CCRemoveSelf::create(),
            nullptr
        ));

        for (int i = 1; i < 5; i++) {
            CCNode* ghost = cloneBodyVisual();
            if (!ghost) continue;
            if (auto* rgba = typeinfo_cast<CCNodeRGBA*>(ghost)) {
                rgba->setOpacity((GLubyte)(170 - i * 30));
                rgba->setColor(ccc3(255, 255, 255));
            }
            float const back = (float)i * 10.0f;
            ghost->setPosition(ccp(hitPos.x - nx * back, hitPos.y - ny * back));
            wallHitVfx.layer->addChild(ghost, topZ(wallHitVfx.layer) + 1);
            ghost->runAction(CCSequence::create(
                CCSpawn::create(
                    CCFadeOut::create(0.12f + 0.03f * (float)i),
                    CCMoveBy::create(0.12f + 0.03f * (float)i, ccp(-nx * 8.0f, -ny * 8.0f)),
                    nullptr
                ),
                CCRemoveSelf::create(),
                nullptr
            ));
        }
    }

    void onClose(CCObject*) {
        saveWeaps();
        g_sandboxOpen = false;
        vfx::object_motion_blur::release(objectBlur);
        CCDirector::sharedDirector()->popScene();
    }

    void wallDmgCheck(float speed, bool isFloor) {
        float minSpeed = isFloor ? 480.0f : 200.0f;
        float maxSpeed = isFloor ? 950.0f : 700.0f;
        float maxDmg = isFloor ? 12.0f : 22.0f;
        if (speed < minSpeed) return;
        float t = (speed - minSpeed) / (maxSpeed - minSpeed);
        if (t > 1.0f) t = 1.0f; dealDmg(t * maxDmg);
    }

    void physicsUpdate(float dt) {
        vfx::impact_flash::decrementCooldown(impactFlash, dt);
        vfx::impact_flash::decrementWhiteFlash(impactFlash, dt);
        vfx::impact_noise::update(impactNoise, dt, impactFlash.whiteFlashRemaining > 0.0f);

        if (impactFlash.hitstopRemaining > 0.0f) {
            impactFlash.hitstopRemaining -= dt;
            if (impactFlash.hitstopRemaining < 0.0f) {
                impactFlash.hitstopRemaining = 0.0f;
            }
            vfx::object_motion_blur::refresh(objectBlur, vfx::impact_flash::currentMode(impactFlash));
            return;
        }

        if (nukeCoolLabel) {
            double now = getRealTime();
            double elapsed = now - g_lastNuke;
            if (elapsed < 300.0) {
                int secsLeft = (int)(300.0 - elapsed);
                nukeCoolLabel->setString(fmt::format("{}s", secsLeft).c_str());
            } else {
                nukeCoolLabel->setString("READY");
            }
        }
        if (!g_bodyInit || !iconPlayer) return;
        if (g_body.dragging) { prevDrag = g_body.pos; return; }
        g_body.vel.y -= 850.0f * dt;
        g_body.pos.x += g_body.vel.x * dt;
        g_body.pos.y += g_body.vel.y * dt;
        if (g_body.pos.y < floorY + 30.0f) {
            float impactSpeed = fabsf(g_body.vel.y);
            g_body.pos.y = floorY + 30.0f;
            g_body.vel.y = -g_body.vel.y * 0.45f;
            g_body.vel.x *= 0.88f;
            wallDmgCheck(impactSpeed, true);
        }
        if (g_body.pos.x < wallL + 30.0f) {
            CCPoint hitVel = g_body.vel;
            float impactSpeed = fabsf(g_body.vel.x);
            g_body.pos.x = wallL + 30.0f;
            g_body.vel.x = -g_body.vel.x * 0.6f;
            if (impactSpeed > 3000.0f) {
                spawnWallHitVfx(ccp(wallL + 30.0f, g_body.pos.y), hitVel, false);
            }
            wallDmgCheck(impactSpeed, false);
        }
        if (g_body.pos.x > wallR - 30.0f) {
            CCPoint hitVel = g_body.vel;
            float impactSpeed = fabsf(g_body.vel.x);
            g_body.pos.x = wallR - 30.0f;
            g_body.vel.x = -g_body.vel.x * 0.6f;
            if (impactSpeed > 3000.0f) {
                spawnWallHitVfx(ccp(wallR - 30.0f, g_body.pos.y), hitVel, false);
            }
            wallDmgCheck(impactSpeed, false);
        }
        syncBodyVisuals();
        // Update speed label
        if (speedLabel) {
            float speed = sqrtf(g_body.vel.x * g_body.vel.x + g_body.vel.y * g_body.vel.y);
            char buf[32];
            sprintf(buf, "%.1f", speed);
            speedLabel->setString(buf);
        }
        // Update motion blur velocity
        if (objectBlur.objects[0].enabled) {
            objectBlur.objects[0].velocity.vx = g_body.vel.x;
            objectBlur.objects[0].velocity.vy = g_body.vel.y;
        }
        vfx::object_motion_blur::refresh(objectBlur, vfx::impact_flash::currentMode(impactFlash));
        std::vector<int> toKill;
        for (int i = 0; i < (int)ents.size(); i++) {
            WepEnt& ent = ents[i];
            if (ent.dying) continue;
            if (ent.dragging) { ent.idleTimer = 0.0f; } else {
                ent.idleTimer += dt;
                if (ent.idleTimer >= 8.0f && ent.idleTimer < 10.0f) { ent.spr->setOpacity((GLubyte)((1.0f - (ent.idleTimer - 8.0f) / 2.0f) * 255)); }
                if (ent.idleTimer >= 10.0f) { ent.dying = true; toKill.push_back(i); }
            }
        }
        for (int i = (int)toKill.size() - 1; i >= 0; i--) {
            int idx = toKill[i];
            if (ents[idx].spr) ents[idx].spr->removeFromParent();
            ents.erase(ents.begin() + idx);
        }
        for (WepEnt& ent : ents) {
            if (!ent.dragging || !ent.spr) continue;
            bool overlapping = ent.spr->boundingBox().intersectsRect(iconPlayer->boundingBox());
            if (overlapping && !ent.hasHit) {
                float totalDmg = (float)g_weaps[ent.id].damage * calcDmgMulti();
                dealDmg(totalDmg);
                ent.hasHit = true;
                g_body.vel = ccp(-80 + rand() % 160, 250);
                ent.spr->runAction(CCSequence::create(CCScaleTo::create(0.05f, 1.3f), CCScaleTo::create(0.05f, 1.0f), nullptr));
            } else if (!overlapping && ent.hasHit) { ent.hasHit = false; }
        }
    }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        CCPoint p = t->getLocation();
        for (WepEnt& ent : ents) {
            if (ent.spr && ccpDistance(p, ent.spr->getPosition()) < 45.0f) {
                ent.dragging = true; ent.hasHit = false; ent.idleTimer = 0.0f;
                ent.spr->setOpacity(255); ent.offset = ent.spr->getPosition() - p; return true;
            }
        }
        if (ccpDistance(p, g_body.pos) < 65.0f) {
            g_body.dragging = true; g_body.dragOffset = g_body.pos - p; prevDrag = g_body.pos; dragVel = ccp(0, 0);
            // Hide motion blur when grabbing
            if (objectBlur.finalCompositeSprite) objectBlur.finalCompositeSprite->setVisible(false);
            if (objectBlur.objects[0].blurSprite) objectBlur.objects[0].blurSprite->setVisible(false);
            return true;
        }
        return true;
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        CCPoint p = t->getLocation();
        for (WepEnt& ent : ents) { if (ent.dragging && ent.spr) ent.spr->setPosition(p + ent.offset); }
        if (g_body.dragging) {
            CCPoint nP = p + g_body.dragOffset; dragVel = ccpSub(nP, g_body.pos);
            prevDrag = g_body.pos; g_body.pos = nP; if (iconPlayer) iconPlayer->setPosition(g_body.pos);
            if (g_body.node && g_body.node != iconPlayer) g_body.node->setPosition(g_body.pos);
        }
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        for (WepEnt& ent : ents) ent.dragging = false;
        if (g_body.dragging) { g_body.vel = ccp(dragVel.x * 60.0f, dragVel.y * 60.0f); g_body.dragging = false; dragVel = ccp(0, 0); }
    }
};

class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        loadWeaps();
        CCNode* bottomMenu = this->getChildByID("bottom-menu");
        CCSprite* iconSpr = CCSprite::create("logo.png"_spr);
        if (!iconSpr) iconSpr = CCSprite::createWithSpriteFrameName("GJ_hammerIcon_001.png");
        CCMenuItemSpriteExtra* b = CCMenuItemSpriteExtra::create(iconSpr, this, menu_selector(MyMenuLayer::onSandbox));
        b->setID("axiom-sandbox-btn");
        if (bottomMenu) { bottomMenu->addChild(b); bottomMenu->updateLayout(); }
        return true;
    }

    void onSandbox(CCObject*) {
        if (g_sandboxOpen) return;
        g_sandboxOpen = true;
        CCScene* scene = CCScene::create();
        scene->addChild(SandboxLayer::create());
        CCDirector::sharedDirector()->pushScene(scene);
    }
};
