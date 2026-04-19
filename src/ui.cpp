#include "ui.hpp"
#include <Geode/modify/MenuLayer.hpp>
#include <filesystem>

// axiom was here again
static float g_cubeMaxHp = 100.0f;
static float g_cubeHp = 100.0f;
static bool g_cubeAlive = true;
static bool g_respawning = false;

static void saveWeaps() {
    for (int i = 0; i < WEP_COUNT; i++) {
        std::string key = "weapon_unlocked_" + std::to_string(i);
        Mod::get()->setSavedValue<bool>(key, g_weapUnlock[i]);
    }
    Mod::get()->setSavedValue<int>("hp_upgrade_level", g_hpLvl);
}

static void loadWeaps() {
    for (int i = 0; i < WEP_COUNT; i++) {
        std::string key = "weapon_unlocked_" + std::to_string(i);
        if (Mod::get()->hasSavedValue(key)) {
            g_weapUnlock[i] = Mod::get()->getSavedValue<bool>(key);
        }
    }
    if (Mod::get()->hasSavedValue("hp_upgrade_level")) {
        g_hpLvl = Mod::get()->getSavedValue<int>("hp_upgrade_level");
    }
}

class SandboxLayer : public CCLayer {
public:
    SimplePlayer* iconPlayer;
    CCLayerColor* hpBg;
    CCLayerColor* hpFill;
    CCLabelBMFont* hpText;
    CCLabelBMFont* coinText;
    CCLabelBMFont* nukeWarn;

    std::vector<WepEnt> ents;
    float floorY;
    float wallL;
    float wallR;
    bool shopOpen;

    CCPoint prevDrag;
    CCPoint dragVel;

    static SandboxLayer* create() {
        SandboxLayer* ret = new SandboxLayer();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
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
        this->addChild(bg, 0);

        spawnCube();
        setupHpBar();
        refreshHpBar();
        setupCoinHud();
        refreshCoinHud();
        setupWeapButtons();
        setupCloseButton();

        CCTouchDispatcher::get()->addTargetedDelegate(this, -128, true);
        this->schedule(schedule_selector(SandboxLayer::physicsUpdate), 1.0f / 60.0f);
        return true;
    }

    void spawnCube() {
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        GameManager* gm = GameManager::sharedState();
        iconPlayer = SimplePlayer::create(0);
        iconPlayer->updatePlayerFrame(g_vicFrames[g_victimFrame], IconType::Cube);
        iconPlayer->setColor(gm->colorForIdx(g_vicCols1[g_victimColor1]));
        iconPlayer->setSecondColor(gm->colorForIdx(g_vicCols2[g_victimColor2]));
        iconPlayer->updateColors();
        iconPlayer->setScale(2.4f);
        iconPlayer->setPosition(ccp(winSize.width * 0.5f, floorY + 85.0f));
        this->addChild(iconPlayer, 3);

        g_bodyInit = true;
        g_body.node = iconPlayer;
        g_body.vel = ccp(0.0f, 0.0f);
        g_body.pos = iconPlayer->getPosition();
        g_body.angVel = 0.0f;
        g_body.angle = 0.0f;
        g_body.dragging = false;
        g_cubeMaxHp = calcMaxHp();
        g_cubeHp = g_cubeMaxHp;
        g_cubeAlive = true;
    }

    void setupHpBar() {
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        hpBg = CCLayerColor::create(ccc4(60, 10, 10, 200), 160.0f, 14.0f);
        hpBg->setPosition(ccp(winSize.width * 0.5f - 80.0f, winSize.height - 46.0f));
        this->addChild(hpBg, 6);
        hpFill = CCLayerColor::create(ccc4(60, 220, 60, 255), 160.0f, 14.0f);
        hpFill->setPosition(hpBg->getPosition());
        this->addChild(hpFill, 7);
        hpText = CCLabelBMFont::create("100/100", "chatFont.fnt");
        hpText->setScale(0.5f);
        hpText->setPosition(ccp(winSize.width * 0.5f, winSize.height - 39.0f));
        this->addChild(hpText, 8);
    }

    void refreshHpBar() {
        float ratio = g_cubeHp / g_cubeMaxHp;
        if (ratio < 0.0f) ratio = 0.0f;
        hpFill->setContentSize(CCSizeMake(160.0f * ratio, 14.0f));
        char buf[32];
        snprintf(buf, sizeof(buf), "%d/%d", (int)g_cubeHp, (int)g_cubeMaxHp);
        hpText->setString(buf);
    }

    void setupCoinHud() {
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        coinText = CCLabelBMFont::create("$0", "chatFont.fnt");
        coinText->setScale(0.62f);
        coinText->setColor(ccc3(255, 215, 0));
        coinText->setPosition(ccp(winSize.width * 0.5f, winSize.height - 62.0f));
        this->addChild(coinText, 6);
    }

    void refreshCoinHud() {
        char buf[32];
        snprintf(buf, sizeof(buf), "$%d", g_coins);
        coinText->setString(buf);
    }

    void dealDmg(float dmg) {
        if (!g_cubeAlive || g_respawning) return;
        g_cubeHp -= dmg;
        if (g_cubeHp < 0.0f) g_cubeHp = 0.0f;
        refreshHpBar();
        if (g_cubeHp <= 0.0f) killCube();
    }

    void killCube() {
        if (!g_cubeAlive) return;
        g_cubeAlive = false;
        g_respawning = true;
        g_bodyInit = false;
        g_coins += killReward(g_cubeMaxHp);
        refreshCoinHud();
        iconPlayer->runAction(CCSequence::create(CCFadeOut::create(0.5f), CCCallFunc::create(this, callfunc_selector(SandboxLayer::doRespawn)), nullptr));
    }

    void doRespawn() {
        if (iconPlayer) iconPlayer->removeFromParentAndCleanup(true);
        g_respawning = false;
        spawnCube();
        refreshHpBar();
    }

    void setupWeapButtons() {
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        CCMenu* weapMenu = CCMenu::create();
        weapMenu->setPosition(ccp(0.0f, 0.0f));
        this->addChild(weapMenu, 5);
        float btnX = winSize.width - 55.0f;
        float startY = winSize.height - 40.0f;
        for (int i = 0; i < WEP_COUNT; i++) {
            CCLabelBMFont* lbl = CCLabelBMFont::create(g_weaps[i].name, "chatFont.fnt");
            lbl->setScale(0.48f);
            lbl->setColor(g_weaps[i].color);
            CCMenuItemLabel* btn = CCMenuItemLabel::create(lbl, this, menu_selector(SandboxLayer::onWeapon));
            btn->setPosition(ccp(btnX, startY - 22.0f * i));
            btn->setTag(i);
            weapMenu->addChild(btn);
        }
        CCLabelBMFont* shopLbl = CCLabelBMFont::create("SHOP", "chatFont.fnt");
        shopLbl->setScale(0.6f);
        shopLbl->setColor(ccc3(80, 220, 255));
        CCMenuItemLabel* shopBtn = CCMenuItemLabel::create(shopLbl, this, menu_selector(SandboxLayer::onShop));
        shopBtn->setPosition(ccp(btnX, 30.0f));
        weapMenu->addChild(shopBtn);
    }

    void onWeapon(CCObject* sender) {
        int idx = sender->getTag();
        if (!g_weapUnlock[idx]) {
            FLAlertLayer::create("Locked", "Buy this in the shop", "OK")->show();
            return;
        }
        if (idx == 7) { // boom
            double now = CCDirector::sharedDirector()->getTotalFrames() / 60.0;
            if (now - g_lastNuke < 300.0) {
                FLAlertLayer::create("Chill", "Nuke cooling down 5 min delay", "OK")->show();
                return;
            }
            startNuke();
            return;
        }
        WepEnt ent;
        ent.id = idx;

        static const char* weapSprites[] = {
            "hammer.png",
            "boot.png",
            "knife.png",
            "brick.png",
            "bat.png",
            "taser.png",
            "chainsaw.png",
            "nuke.png",
        };

        std::filesystem::path resPath = Mod::get()->getResourcesDir();
        resPath /= weapSprites[idx];
        ent.spr = CCSprite::create(resPath.string().c_str());
        if (!ent.spr) ent.spr = CCSprite::createWithSpriteFrameName("edit_eBtn_001.png");
        ent.spr->setColor(g_weaps[idx].color);
        ent.spr->setPosition(ccp(100, 100));
        ent.dragging = false;
        ent.hasHit = false;
        ent.idleTimer = 0.0f;
        ent.dying = false;
        int entZ = 0;
        for (CCNode* kid : this->getChildrenExt()) {
            if (kid->getZOrder() > entZ) entZ = kid->getZOrder();
        }
        this->addChild(ent.spr, entZ + 1);
        ents.push_back(ent);
    }

    void startNuke() {
        g_lastNuke = CCDirector::sharedDirector()->getTotalFrames() / 60.0;
        nukeWarn = CCLabelBMFont::create("NUKE INCOMING!!!!!!!!!!!!", "bigFont.fnt");
        nukeWarn->setPosition(CCDirector::sharedDirector()->getWinSize() / 2);
        int topZ = 0;
        for (CCNode* kid : this->getChildrenExt()) {
            if (kid->getZOrder() > topZ) topZ = kid->getZOrder();
        }
        this->addChild(nukeWarn, topZ + 1);
        nukeWarn->runAction(CCRepeatForever::create(CCSequence::create(CCTintTo::create(0.1f, 255, 0, 0), CCTintTo::create(0.1f, 255, 255, 255), nullptr)));
        this->scheduleOnce(schedule_selector(SandboxLayer::nukeExplode), 3.5f);
    }

    void nukeExplode(float) {
        if (nukeWarn) nukeWarn->removeFromParentAndCleanup(true);
        CCLayerColor* flash = CCLayerColor::create(ccc4(255, 255, 255, 255));
        int topZ = 0;
        for (CCNode* kid : this->getChildrenExt()) {
            if (kid->getZOrder() > topZ) topZ = kid->getZOrder();
        }
        this->addChild(flash, topZ + 1);
        flash->runAction(CCSequence::create(CCFadeOut::create(1.2f), CCRemoveSelf::create(), nullptr));
        dealDmg(999);
        g_coins += 1500 + rand() % 1000;
        refreshCoinHud();
    }

    void onShop(CCObject*) {
        if (shopOpen) return;
        shopOpen = true;
        ShopLayer::create([this]() {
            shopOpen = false;
            saveWeaps();
        }, [this](int f, int c1, int c2) {
            iconPlayer->updatePlayerFrame(g_vicFrames[f], IconType::Cube);
        })->show();
    }

    void setupCloseButton() {
        CCMenu* m = CCMenu::create();
        m->setPosition(ccp(0, 0));
        CCMenuItemSpriteExtra* b = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"), this, menu_selector(SandboxLayer::onClose));
        b->setPosition(ccp(25, CCDirector::sharedDirector()->getWinSize().height - 25));
        m->addChild(b);
        int topZ = 0;
        for (CCNode* kid : this->getChildrenExt()) {
            if (kid->getZOrder() > topZ) topZ = kid->getZOrder();
        }
        this->addChild(m, topZ + 1);
    }

    void onClose(CCObject*) {
        CCTouchDispatcher::get()->removeDelegate(this);
        g_sandboxOpen = false;
        this->removeFromParentAndCleanup(true);
    }

    void wallDmgCheck(float speed, bool isFloor) {
        float minSpeed = isFloor ? 480.0f : 200.0f;
        float maxSpeed = isFloor ? 950.0f : 700.0f;
        float maxDmg   = isFloor ? 12.0f  : 22.0f;
        if (speed < minSpeed) return;
        float t = (speed - minSpeed) / (maxSpeed - minSpeed);
        if (t > 1.0f) t = 1.0f;
        dealDmg(t * maxDmg);
    }

    void physicsUpdate(float dt) {
        if (!g_bodyInit) return;
        if (g_body.dragging) {
            prevDrag = g_body.pos;
            return;
        }

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
            float impactSpeed = fabsf(g_body.vel.x);
            g_body.pos.x = wallL + 30.0f;
            g_body.vel.x = -g_body.vel.x * 0.6f;
            wallDmgCheck(impactSpeed, false);
        }
        if (g_body.pos.x > wallR - 30.0f) {
            float impactSpeed = fabsf(g_body.vel.x);
            g_body.pos.x = wallR - 30.0f;
            g_body.vel.x = -g_body.vel.x * 0.6f;
            wallDmgCheck(impactSpeed, false);
        }

        iconPlayer->setPosition(g_body.pos);

        std::vector<int> toKill;
        for (int i = 0; i < (int)ents.size(); i++) {
            WepEnt& ent = ents[i];
            if (ent.dying) continue;
            if (ent.dragging) {
                ent.idleTimer = 0.0f;
            } else {
                ent.idleTimer += dt;
                if (ent.idleTimer >= 8.0f && ent.idleTimer < 10.0f) {
                    float fade = 1.0f - (ent.idleTimer - 8.0f) / 2.0f;
                    ent.spr->setOpacity((GLubyte)(fade * 255));
                }
                if (ent.idleTimer >= 10.0f) {
                    ent.dying = true;
                    toKill.push_back(i);
                }
            }
        }
        for (int i = (int)toKill.size() - 1; i >= 0; i--) {
            int idx = toKill[i];
            ents[idx].spr->removeFromParentAndCleanup(true);
            ents.erase(ents.begin() + idx);
        }

        for (WepEnt& ent : ents) {
            if (!ent.dragging) continue;
            bool overlapping = ent.spr->boundingBox().intersectsRect(iconPlayer->boundingBox());
            if (overlapping && !ent.hasHit) {
                dealDmg(g_weaps[ent.id].damage);
                ent.hasHit = true;
                g_body.vel = ccp(-80 + rand() % 160, 250);
                ent.spr->runAction(CCSequence::create(CCScaleTo::create(0.05f, 1.3f), CCScaleTo::create(0.05f, 1.0f), nullptr));
            } else if (!overlapping && ent.hasHit) {
                ent.hasHit = false;
            }
        }
    }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        CCPoint p = t->getLocation();
        for (WepEnt& ent : ents) {
            if (ccpDistance(p, ent.spr->getPosition()) < 45.0f) {
                ent.dragging = true;
                ent.hasHit = false;
                ent.idleTimer = 0.0f;
                ent.spr->setOpacity(255);
                ent.offset = ent.spr->getPosition() - p;
                return true;
            }
        }
        if (ccpDistance(p, g_body.pos) < 65.0f) {
            g_body.dragging = true;
            g_body.dragOffset = g_body.pos - p;
            prevDrag = g_body.pos;
            dragVel = ccp(0, 0);
            return true;
        }
        return true;
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        CCPoint p = t->getLocation();
        for (WepEnt& ent : ents) if (ent.dragging) ent.spr->setPosition(p + ent.offset);
        if (g_body.dragging) {
            CCPoint newPos = p + g_body.dragOffset;
            dragVel = ccpSub(newPos, g_body.pos);
            prevDrag = g_body.pos;
            g_body.pos = newPos;
            iconPlayer->setPosition(g_body.pos);
        }
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        for (WepEnt& ent : ents) ent.dragging = false;
        if (g_body.dragging) {
            g_body.vel = ccp(dragVel.x * 60.0f, dragVel.y * 60.0f);
            g_body.dragging = false;
            dragVel = ccp(0, 0);
        }
    }
};

class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        loadWeaps();

        std::filesystem::path logoPath = Mod::get()->getResourcesDir();
        logoPath /= "logo.png";
        CCSprite* iconSpr = CCSprite::create(logoPath.string().c_str());
        if (!iconSpr) {
            iconSpr = CCSprite::createWithSpriteFrameName("GJ_hammerIcon_001.png");
        }
        CCLabelBMFont* nameLbl = CCLabelBMFont::create("Cube Abuse", "chatFont.fnt");
        nameLbl->setScale(0.42f);
        nameLbl->setColor(ccc3(255, 180, 80));
        nameLbl->setPosition(ccp(iconSpr->getContentSize().width * 0.5f, -10.0f));
        iconSpr->addChild(nameLbl);

        CCMenuItemSpriteExtra* b = CCMenuItemSpriteExtra::create(iconSpr, this, menu_selector(MyMenuLayer::onSandbox));
        CCMenu* m = CCMenu::create();
        m->setPosition(ccp(35, 140));
        m->addChild(b);
        this->addChild(m);
        return true;
    }
    void onSandbox(CCObject*) {
        // if (g_sandboxOpen) return;
        g_sandboxOpen = true;
        CCScene* scene = CCDirector::sharedDirector()->getRunningScene();
        int topZ = 0;
        for (CCNode* kid : scene->getChildrenExt()) {
            if (kid->getZOrder() > topZ) topZ = kid->getZOrder();
        }
        scene->addChild(SandboxLayer::create(), topZ + 1);
    }
};