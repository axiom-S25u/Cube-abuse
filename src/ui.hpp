#pragma once
#include <Geode/Geode.hpp>
#include <Geode/binding/SimplePlayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <functional>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>

using namespace geode::prelude;

static int topZ(CCNode* p) {
    int m = 0;
    if (p) {
        for (auto node : p->getChildrenExt()) {
            if (node->getZOrder() > m) m = node->getZOrder();
        }
    }
    return m;
}

inline bool g_sandboxOpen = false;
inline int g_coins = 0;
inline int g_victimFrame = 0;
inline int g_victimColor1 = 0;
inline int g_victimColor2 = 3;
inline bool g_bodyInit = false;

struct BodyShit {
    CCNode* node;
    CCPoint vel;
    CCPoint pos;
    float angVel;
    float angle;
    bool dragging;
    CCPoint dragOffset;
};
inline BodyShit g_body;

struct WepDef {
    const char* name;
    int cost;
    int damage;
    ccColor3B color;
    const char* iconName;
};

static WepDef g_weaps[] = {
    { "Hammer", 0, 15, ccc3(255, 200, 100), "axiom.cube-abuse/hammer.png" },
    { "Boot", 0, 10, ccc3(150, 220, 150), "axiom.cube-abuse/boot.png" },
    { "Knife", 0, 20, ccc3(200, 150, 255), "axiom.cube-abuse/knife.png" },
    { "Brick", 50, 30, ccc3(180, 120, 80), "axiom.cube-abuse/brick.png" },
    { "Bat", 200, 45, ccc3(255, 255, 100), "axiom.cube-abuse/bat.png" },
    { "Taser", 500, 60, ccc3(100, 255, 255), "axiom.cube-abuse/taser.png" },
    { "Chainsaw", 1500, 85, ccc3(200, 50, 50), "axiom.cube-abuse/chainsaw.png" },
    { "Nuke", 15000, 999, ccc3(255, 255, 255), "axiom.cube-abuse/nuke.png" },
};
inline const int WEP_COUNT = 8;
inline bool g_weapUnlock[] = { true, true, true, false, false, false, false, false };

static int g_vicFrames[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
inline const int VIC_POOL_SIZE = 20;

static int g_vicCols1[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static int g_vicCols2[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
inline const int COL_POOL_SIZE = 12;

inline double g_lastNuke = -99999.0;

struct WepEnt {
    CCSprite* spr;
    int id;
    bool dragging;
    CCPoint offset;
    bool hasHit;
    float idleTimer;
    bool dying;
};

inline const int HP_MAX_UPG = 5;
inline int g_hpLvl = 0;

inline void saveWeaps() {
    for (int i = 0; i < WEP_COUNT; i++) {
        std::string key = "weapon_unlocked_" + std::to_string(i);
        Mod::get()->setSavedValue<bool>(key, g_weapUnlock[i]);
    }
    Mod::get()->setSavedValue<int>("hp_upgrade_level", g_hpLvl);
    Mod::get()->setSavedValue<int>("total_coins", g_coins);
}

inline void loadWeaps() {
    for (int i = 0; i < WEP_COUNT; i++) {
        std::string key = "weapon_unlocked_" + std::to_string(i);
        if (Mod::get()->hasSavedValue(key)) {
            g_weapUnlock[i] = Mod::get()->getSavedValue<bool>(key);
        }
    }
    if (Mod::get()->hasSavedValue("hp_upgrade_level")) {
        g_hpLvl = Mod::get()->getSavedValue<int>("hp_upgrade_level");
    }
    if (Mod::get()->hasSavedValue("total_coins")) {
        g_coins = Mod::get()->getSavedValue<int>("total_coins");
    }
}

static float calcMaxHp() {
    float base = 100.0f;
    float val = base;
    float mults[] = { 1.0f, 1.35f, 1.5f, 1.4f, 1.3f, 1.45f };
    for (int i = 0; i < g_hpLvl && i < HP_MAX_UPG; i++) { val *= mults[i + 1]; }
    return val;
}

static int hpCost(int lvl) {
    int costs[] = { 200, 550, 1400, 3500, 9000 };
    if (lvl < 0 || lvl >= HP_MAX_UPG) return 99999;
    return costs[lvl];
}

static int killReward(float maxHp) {
    float base = 30.0f + rand() % 60;
    float bonus = powf(maxHp / 100.0f, 0.72f);
    return (int)(base * bonus);
}

class ShopLayer : public CCLayer {
public:
    std::function<void()> closeCB;
    std::function<void(int, int, int)> victimCB;
    SimplePlayer* prevPlr;
    CCLabelBMFont* coinText;
    CCNode* weapNode; CCNode* upgNode;
    int pFrame; int pC1; int pC2;

    static ShopLayer* create(std::function<void()> cb, std::function<void(int, int, int)> vcb) {
        ShopLayer* ret = new ShopLayer();
        ret->closeCB = cb; ret->victimCB = vcb;
        if (ret && ret->init()) { ret->autorelease(); return ret; }
        delete ret; return nullptr;
    }

    void onUpgradeHp(CCObject*) {
        if (g_hpLvl >= HP_MAX_UPG) return;
        int cost = hpCost(g_hpLvl);
        if (g_coins < cost) { FLAlertLayer::create("Broke", "Not enough coins", "OK")->show(); return; }
        g_coins -= cost; g_hpLvl++; saveWeaps(); rebuildDynamicContent();
    }

    void rebuildDynamicContent(); // decl

    void buildWeapList(CCMenu* shopMenu, CCSize winSize, float panelX, float panelY, float panelH, float panelW) {
        if (weapNode) weapNode->removeAllChildrenWithCleanup(true);
        float colW = panelW * 0.55f; float leftX = panelX + colW * 0.28f;
        float startY = panelY + panelH - 68.0f; float gap = 22.0f;
        for (int i = 0; i < WEP_COUNT; i++) {
            WepDef& wd = g_weaps[i]; float rowY = startY - gap * (i + 1);
            CCLayerColor* rowBg = CCLayerColor::create((i % 2 == 0) ? ccc4(255, 255, 255, 10) : ccc4(0, 0, 0, 10), colW - 12.0f, gap - 2.0f);
            rowBg->setPosition(ccp(panelX + 6.0f, rowY - (gap - 2.0f) * 0.5f)); weapNode->addChild(rowBg, topZ(weapNode) + 1);
            CCSprite* iconSpr = CCSprite::create(wd.iconName); if (!iconSpr) iconSpr = CCSprite::createWithSpriteFrameName("edit_eBtn_001.png");
            if (iconSpr) { iconSpr->setPosition(ccp(leftX - 22.0f, rowY)); iconSpr->setScale(18.0f / iconSpr->getContentSize().height); weapNode->addChild(iconSpr, topZ(weapNode) + 1); }
            CCLabelBMFont* nameLbl = CCLabelBMFont::create(wd.name, "chatFont.fnt"); nameLbl->setScale(0.46f); nameLbl->setColor(wd.color);
            nameLbl->setAnchorPoint(ccp(0.0f, 0.5f)); nameLbl->setPosition(ccp(leftX - 8.0f, rowY)); weapNode->addChild(nameLbl, topZ(weapNode) + 1);
            if (g_weapUnlock[i]) { CCLabelBMFont* oL = CCLabelBMFont::create("OWNED", "chatFont.fnt"); oL->setScale(0.38f); oL->setColor(ccc3(80, 255, 120)); oL->setPosition(ccp(leftX + 122.0f, rowY)); weapNode->addChild(oL, topZ(weapNode) + 1); }
            else { char cB[32]; snprintf(cB, sizeof(cB), "$%d", wd.cost); CCMenuItemSpriteExtra* bB = CCMenuItemSpriteExtra::create(CCLabelBMFont::create(cB, "chatFont.fnt"), this, menu_selector(ShopLayer::onBuy)); bB->setPosition(ccp(leftX + 122.0f, rowY)); bB->setTag(i); shopMenu->addChild(bB, topZ(shopMenu) + 1); }
        }
    }

    void buildUpgSection(CCMenu* shopMenu, CCSize winSize, float panelX, float panelY, float panelH, float panelW) {
        if (upgNode) upgNode->removeAllChildrenWithCleanup(true);
        float colW = panelW * 0.55f; float cx = panelX + colW * 0.5f; float topY = panelY + 72.0f;
        char hpBuf[64]; snprintf(hpBuf, sizeof(hpBuf), "HP: %.0f [Lv %d/%d]", calcMaxHp(), g_hpLvl, HP_MAX_UPG);
        CCLabelBMFont* hI = CCLabelBMFont::create(hpBuf, "chatFont.fnt"); hI->setScale(0.45f); hI->setColor(ccc3(100, 230, 100)); hI->setPosition(ccp(cx, topY - 16.0f)); upgNode->addChild(hI, topZ(upgNode) + 1);
        if (g_hpLvl < HP_MAX_UPG) { char uB[64]; snprintf(uB, sizeof(uB), "Upgrade HP $%d", hpCost(g_hpLvl)); CCMenuItemSpriteExtra* uBtn = CCMenuItemSpriteExtra::create(CCLabelBMFont::create(uB, "chatFont.fnt"), this, menu_selector(ShopLayer::onUpgradeHp)); uBtn->setPosition(ccp(cx, topY - 34.0f)); uBtn->setTag(9999); shopMenu->addChild(uBtn, topZ(shopMenu) + 1); }
    }

    void refreshCoins() { char buf[64]; snprintf(buf, sizeof(buf), "Coins: $%d", g_coins); if (coinText) coinText->setString(buf); }

    void onBuy(CCObject* sender) {
        int idx = sender->getTag();
        if (g_coins < g_weaps[idx].cost) { FLAlertLayer::create("Broke", "Not enough coins", "OK")->show(); return; }
        g_coins -= g_weaps[idx].cost; g_weapUnlock[idx] = true; saveWeaps(); rebuildDynamicContent();
    }

    bool init() override {
        if (!CCLayer::init()) return false;
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        pFrame = g_victimFrame; pC1 = g_victimColor1; pC2 = g_victimColor2;
        this->addChild(CCLayerColor::create(ccc4(0, 0, 0, 160), winSize.width, winSize.height), topZ(this) + 1);
        float panelW = winSize.width * 0.88f; float panelH = winSize.height * 0.86f;
        float panelX = (winSize.width - panelW) * 0.5f; float panelY = (winSize.height - panelH) * 0.5f;
        CCLayerColor* p = CCLayerColor::create(ccc4(14, 8, 28, 248), panelW, panelH); p->setPosition(ccp(panelX, panelY)); this->addChild(p, topZ(this) + 1);
        coinText = CCLabelBMFont::create("", "chatFont.fnt"); coinText->setPosition(ccp(winSize.width * 0.5f, panelY + panelH - 46.0f)); this->addChild(coinText, topZ(this) + 1); refreshCoins();
        CCMenu* shopMenu = CCMenu::create(); shopMenu->setPosition(ccp(0.0f, 0.0f)); this->addChild(shopMenu, topZ(this) + 1); shopMenu->setTouchPriority(-130);
        weapNode = CCNode::create(); this->addChild(weapNode, topZ(this) + 1); buildWeapList(shopMenu, winSize, panelX, panelY, panelH, panelW);
        upgNode = CCNode::create(); this->addChild(upgNode, topZ(this) + 1); buildUpgSection(shopMenu, winSize, panelX, panelY, panelH, panelW);
        setupVictimMenu(shopMenu, winSize, panelX, panelY, panelH, panelW);
        CCMenuItemSpriteExtra* cB = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"), this, menu_selector(ShopLayer::onClose));
        cB->setPosition(ccp(panelX + 18.0f, panelY + panelH - 14.0f)); shopMenu->addChild(cB, topZ(shopMenu) + 1);
        CCTouchDispatcher::get()->addTargetedDelegate(this, -129, true); return true;
    }

    void refreshPreview() { GameManager* gm = GameManager::sharedState(); prevPlr->updatePlayerFrame(g_vicFrames[pFrame], IconType::Cube); prevPlr->setColor(gm->colorForIdx(g_vicCols1[pC1])); prevPlr->setSecondColor(gm->colorForIdx(g_vicCols2[pC2])); prevPlr->updateColors(); }
    void onPrevFrame(CCObject*) { pFrame = (pFrame + VIC_POOL_SIZE - 1) % VIC_POOL_SIZE; refreshPreview(); }
    void onNextFrame(CCObject*) { pFrame = (pFrame + 1) % VIC_POOL_SIZE; refreshPreview(); }
    void onRandom(CCObject*) { pFrame = rand() % VIC_POOL_SIZE; pC1 = rand() % COL_POOL_SIZE; pC2 = rand() % COL_POOL_SIZE; refreshPreview(); }
    void onApply(CCObject*) { g_victimFrame = pFrame; g_victimColor1 = pC1; g_victimColor2 = pC2; if (victimCB) victimCB(pFrame, pC1, pC2); saveWeaps(); }
    void onClose(CCObject*) { CCTouchDispatcher::get()->removeDelegate(this); saveWeaps(); if (closeCB) closeCB(); this->removeFromParentAndCleanup(true); }
    void setupVictimMenu(CCMenu* shopMenu, CCSize winSize, float panelX, float panelY, float panelH, float panelW) {
        float rightCx = panelX + panelW * 0.77f; float topY = panelY + panelH - 72.0f;
        prevPlr = SimplePlayer::create(0); refreshPreview(); prevPlr->setScale(1.8f); prevPlr->setPosition(ccp(rightCx, topY - 48.0f)); this->addChild(prevPlr, topZ(this) + 1);
        CCMenuItemSpriteExtra* pF = CCMenuItemSpriteExtra::create(CCLabelBMFont::create("<", "bigFont.fnt"), this, menu_selector(ShopLayer::onPrevFrame)); pF->setPosition(ccp(rightCx - 40.0f, topY - 48.0f)); shopMenu->addChild(pF);
        CCMenuItemSpriteExtra* nF = CCMenuItemSpriteExtra::create(CCLabelBMFont::create(">", "bigFont.fnt"), this, menu_selector(ShopLayer::onNextFrame)); nF->setPosition(ccp(rightCx + 40.0f, topY - 48.0f)); shopMenu->addChild(nF);
        CCMenuItemSpriteExtra* rB = CCMenuItemSpriteExtra::create(CCLabelBMFont::create("Random!", "chatFont.fnt"), this, menu_selector(ShopLayer::onRandom)); rB->setPosition(ccp(rightCx, topY - 130.0f)); shopMenu->addChild(rB);
        CCMenuItemSpriteExtra* aB = CCMenuItemSpriteExtra::create(CCLabelBMFont::create("Apply", "chatFont.fnt"), this, menu_selector(ShopLayer::onApply)); aB->setPosition(ccp(rightCx, topY - 147.0f)); shopMenu->addChild(aB);
    }
    bool ccTouchBegan(CCTouch*, CCEvent*) override { return true; }
    void ccTouchMoved(CCTouch*, CCEvent*) override {}
    void ccTouchEnded(CCTouch*, CCEvent*) override {}
    void ccTouchCancelled(CCTouch*, CCEvent*) override {}
    void show() { CCScene* scene = CCDirector::sharedDirector()->getRunningScene(); scene->addChild(this, topZ(scene) + 1); }
};

inline void ShopLayer::rebuildDynamicContent() {
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    float panelW = winSize.width * 0.88f; float panelH = winSize.height * 0.86f;
    float panelX = (winSize.width - panelW) * 0.5f; float panelY = (winSize.height - panelH) * 0.5f;
    CCMenu* shopMenu = nullptr;
    for (auto node : this->getChildrenExt()) { CCMenu* m = dynamic_cast<CCMenu*>(node); if (m && m->getTouchPriority() == -130) { shopMenu = m; break; } }
    if (!shopMenu) return;
    std::vector<CCNode*> toRemove;
    for (auto n : shopMenu->getChildrenExt()) { int tag = n->getTag(); if ((tag >= 0 && tag < WEP_COUNT) || tag == 9999) toRemove.push_back(n); }
    for (CCNode* n : toRemove) n->removeFromParentAndCleanup(true);
    buildWeapList(shopMenu, winSize, panelX, panelY, panelH, panelW);
    buildUpgSection(shopMenu, winSize, panelX, panelY, panelH, panelW);
    refreshCoins();
}
