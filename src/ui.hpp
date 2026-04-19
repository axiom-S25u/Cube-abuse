#pragma once
#include <Geode/Geode.hpp>
#include <Geode/binding/SimplePlayer.hpp>
#include <Geode/binding/GameManager.hpp>

using namespace geode::prelude;

// axiom was here
static bool g_sandboxOpen = false;
static int g_coins = 0;

static int g_victimFrame = 0;
static int g_victimColor1 = 0;
static int g_victimColor2 = 3;

struct BodyShit {
    CCNode* node;
    CCPoint vel;
    CCPoint pos;
    float angVel;
    float angle;
    bool dragging;
    CCPoint dragOffset;
};

static BodyShit g_body;
static bool g_bodyInit = false;

struct WepDef {
    const char* name;
    int cost;
    int damage;
    ccColor3B color;
    const char* iconName;
};

static WepDef g_weaps[] = {
    { "Hammer", 0, 15, ccc3(255, 200, 100), "hammer.png" },
    { "Boot", 0, 10, ccc3(150, 220, 150), "boot.png" },
    { "Knife", 0, 20, ccc3(200, 150, 255), "knife.png" },
    { "Brick", 50, 30, ccc3(180, 120, 80), "brick.png" },
    { "Bat", 200, 45, ccc3(255, 255, 100), "bat.png" },
    { "Taser", 500, 60, ccc3(100, 255, 255), "taser.png" },
    { "Chainsaw", 1500, 85, ccc3(200, 50, 50), "chainsaw.png" },
    { "Nuke", 15000, 999, ccc3(255, 255, 255), "nuke.png" }, // why is remove.bg fully free without an account free anyway?
};;
static const int WEP_COUNT = 8;
static bool g_weapUnlock[] = { true, true, true, false, false, false, false, false };

static int g_vicFrames[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
static const int VIC_POOL_SIZE = 20;

static int g_vicCols1[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static int g_vicCols2[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static const int COL_POOL_SIZE = 12;

static double g_lastNuke = -99999.0;

struct WepEnt {
    CCSprite* spr;
    int id;
    bool dragging;
    CCPoint offset;
    bool hasHit;
    float idleTimer;
    bool dying;
};

static const int HP_MAX_UPG = 5;
static int g_hpLvl = 0;

// these multipliers are cursed but if you change them the universe explodes
static float calcMaxHp() {
    float base = 100.0f;
    float val = base;
    float mults[] = { 1.0f, 1.35f, 1.5f, 1.4f, 1.3f, 1.45f };
    for (int i = 0; i < g_hpLvl && i < HP_MAX_UPG; i++) {
        val *= mults[i + 1];
    }
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

// hardcoded everything because dynamic layouts are for android devs
class ShopLayer : public CCLayer {
public:
    std::function<void()> closeCB;
    std::function<void(int, int, int)> victimCB;

    SimplePlayer* prevPlr;
    CCLabelBMFont* coinText;

    CCNode* weapNode;
    CCNode* upgNode;

    int pFrame;
    int pC1;
    int pC2;

    static ShopLayer* create(std::function<void()> cb, std::function<void(int, int, int)> vcb) {
        ShopLayer* ret = new ShopLayer();
        ret->closeCB = cb;
        ret->victimCB = vcb;
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

        pFrame = g_victimFrame;
        pC1 = g_victimColor1;
        pC2 = g_victimColor2;

        CCLayerColor* dimmer = CCLayerColor::create(ccc4(0, 0, 0, 160));
        dimmer->setContentSize(winSize);
        this->addChild(dimmer, 0);

        float panelW = winSize.width * 0.88f;
        float panelH = winSize.height * 0.86f;
        float panelX = (winSize.width - panelW) * 0.5f;
        float panelY = (winSize.height - panelH) * 0.5f;

        CCLayerColor* panel = CCLayerColor::create(ccc4(14, 8, 28, 248));
        panel->setContentSize(CCSizeMake(panelW, panelH));
        panel->setPosition(ccp(panelX, panelY));
        this->addChild(panel, 1);

        CCLayerColor* titleBar = CCLayerColor::create(ccc4(30, 10, 60, 255));
        titleBar->setContentSize(CCSizeMake(panelW, 32.0f));
        titleBar->setPosition(ccp(panelX, panelY + panelH - 32.0f));
        this->addChild(titleBar, 2);

        CCLabelBMFont* title = CCLabelBMFont::create("WEAPON SHOP", "bigFont.fnt");
        title->setScale(0.48f);
        title->setColor(ccc3(255, 215, 0));
        title->setPosition(ccp(winSize.width * 0.5f, panelY + panelH - 16.0f));
        this->addChild(title, 3);

        coinText = CCLabelBMFont::create("", "chatFont.fnt");
        coinText->setScale(0.62f);
        coinText->setColor(ccc3(255, 220, 50));
        coinText->setPosition(ccp(winSize.width * 0.5f, panelY + panelH - 46.0f));
        this->addChild(coinText, 3);
        refreshCoins();

        CCLayerColor* dividerLeft = CCLayerColor::create(ccc4(50, 20, 90, 200));
        dividerLeft->setContentSize(CCSizeMake(panelW * 0.55f - 4.0f, panelH - 64.0f));
        dividerLeft->setPosition(ccp(panelX + 4.0f, panelY + 4.0f));
        this->addChild(dividerLeft, 2);

        CCLayerColor* dividerRight = CCLayerColor::create(ccc4(30, 15, 55, 200));
        dividerRight->setContentSize(CCSizeMake(panelW * 0.45f - 8.0f, panelH - 64.0f));
        dividerRight->setPosition(ccp(panelX + panelW * 0.55f, panelY + 4.0f));
        this->addChild(dividerRight, 2);

        CCMenu* shopMenu = CCMenu::create();
        shopMenu->setPosition(ccp(0.0f, 0.0f));
        this->addChild(shopMenu, 4);
        shopMenu->setTouchPriority(-130);

        weapNode = CCNode::create();
        weapNode->setTag(991);
        this->addChild(weapNode, 3);
        buildWeapList(shopMenu, winSize, panelX, panelY, panelH, panelW);

        upgNode = CCNode::create();
        upgNode->setTag(992);
        this->addChild(upgNode, 3);
        buildUpgSection(shopMenu, winSize, panelX, panelY, panelH, panelW);

        setupVictimMenu(shopMenu, winSize, panelX, panelY, panelH, panelW);

        CCSprite* closeSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        CCMenuItemSpriteExtra* closeBtn = CCMenuItemSpriteExtra::create(closeSpr, this, menu_selector(ShopLayer::onClose));
        closeBtn->setPosition(ccp(panelX + 18.0f, panelY + panelH - 14.0f));
        shopMenu->addChild(closeBtn);

        CCTouchDispatcher::get()->addTargetedDelegate(this, -129, true);
        return true;
    }

    void buildWeapList(CCMenu* shopMenu, CCSize winSize, float panelX, float panelY, float panelH, float panelW) {
        if (weapNode) weapNode->removeAllChildrenWithCleanup(true);

        float colW = panelW * 0.55f;
        float leftX = panelX + colW * 0.28f;

        float upgradeBlockH = 80.0f;
        float availableH = panelH - 64.0f - upgradeBlockH - 16.0f;
        float gap = availableH / (WEP_COUNT + 1);
        if (gap > 22.0f) gap = 22.0f;

        float startY = panelY + panelH - 68.0f;

        CCLabelBMFont* secTitle = CCLabelBMFont::create("WEAPONS", "chatFont.fnt");
        secTitle->setScale(0.5f);
        secTitle->setColor(ccc3(180, 120, 255));
        secTitle->setPosition(ccp(panelX + colW * 0.5f, startY + 8.0f));
        weapNode->addChild(secTitle);

        for (int i = 0; i < WEP_COUNT; i++) {
            WepDef& wd = g_weaps[i];

            float rowY = startY - gap * (i + 1);

            CCLayerColor* rowBg = CCLayerColor::create(
                (i % 2 == 0) ? ccc4(255, 255, 255, 10) : ccc4(0, 0, 0, 10),
                colW - 12.0f, gap - 2.0f
            );
            rowBg->setPosition(ccp(panelX + 6.0f, rowY - (gap - 2.0f) * 0.5f));
            weapNode->addChild(rowBg);

            std::string sprPath = (Mod::get()->getResourcesDir() / wd.iconName).string();
            CCSprite* iconSpr = CCSprite::create(sprPath.c_str());
            if (iconSpr) {
                iconSpr->setPosition(ccp(leftX - 22.0f, rowY));
                float s = 18.0f / iconSpr->getContentSize().height;
                iconSpr->setScale(s);
                weapNode->addChild(iconSpr);
            }

            CCLabelBMFont* nameLbl = CCLabelBMFont::create(wd.name, "chatFont.fnt");
            nameLbl->setScale(0.46f);
            nameLbl->setColor(wd.color);
            nameLbl->setAnchorPoint(ccp(0.0f, 0.5f));
            nameLbl->setPosition(ccp(leftX - 8.0f, rowY));
            weapNode->addChild(nameLbl);

            CCLabelBMFont* dmgLbl = CCLabelBMFont::create("", "chatFont.fnt");
            char dmgBuf[16];
            snprintf(dmgBuf, sizeof(dmgBuf), "%ddmg", wd.damage);
            dmgLbl->setString(dmgBuf);
            dmgLbl->setScale(0.36f);
            dmgLbl->setColor(ccc3(255, 120, 120));
            dmgLbl->setPosition(ccp(leftX + 55.0f, rowY));
            weapNode->addChild(dmgLbl);

            if (g_weapUnlock[i]) {
                CCLabelBMFont* ownedLbl = CCLabelBMFont::create("OWNED", "chatFont.fnt");
                ownedLbl->setScale(0.38f);
                ownedLbl->setColor(ccc3(80, 255, 120));
                ownedLbl->setPosition(ccp(leftX + 122.0f, rowY));
                weapNode->addChild(ownedLbl);
            } else {
                char costBuf[32];
                snprintf(costBuf, sizeof(costBuf), "$%d", wd.cost);
                CCLabelBMFont* costLbl = CCLabelBMFont::create(costBuf, "chatFont.fnt");
                costLbl->setScale(0.42f);
                bool canAfford = g_coins >= wd.cost;
                costLbl->setColor(canAfford ? ccc3(255, 200, 50) : ccc3(150, 80, 80));
                CCMenuItemLabel* buyBtn = CCMenuItemLabel::create(costLbl, this, menu_selector(ShopLayer::onBuy));
                buyBtn->setPosition(ccp(leftX + 122.0f, rowY));
                buyBtn->setTag(i);
                shopMenu->addChild(buyBtn);
            }
        }
    }

    void buildUpgSection(CCMenu* shopMenu, CCSize winSize, float panelX, float panelY, float panelH, float panelW) {
        if (upgNode) upgNode->removeAllChildrenWithCleanup(true);

        float colW = panelW * 0.55f;
        float cx = panelX + colW * 0.5f;
        float topY = panelY + 72.0f;

        CCLayerColor* sep = CCLayerColor::create(ccc4(120, 60, 200, 120));
        sep->setContentSize(CCSizeMake(colW - 20.0f, 2.0f));
        sep->setPosition(ccp(panelX + 10.0f, topY + 12.0f));
        upgNode->addChild(sep);

        CCLabelBMFont* upgTitle = CCLabelBMFont::create("UPGRADES", "chatFont.fnt");
        upgTitle->setScale(0.5f);
        upgTitle->setColor(ccc3(180, 120, 255));
        upgTitle->setPosition(ccp(cx, topY));
        upgNode->addChild(upgTitle);

        char hpBuf[64];
        snprintf(hpBuf, sizeof(hpBuf), "HP: %.0f  [Lv %d/%d]", calcMaxHp(), g_hpLvl, HP_MAX_UPG);
        CCLabelBMFont* hpInfo = CCLabelBMFont::create(hpBuf, "chatFont.fnt");
        hpInfo->setScale(0.45f);
        hpInfo->setColor(ccc3(100, 230, 100));
        hpInfo->setPosition(ccp(cx, topY - 16.0f));
        upgNode->addChild(hpInfo);

        if (g_hpLvl < HP_MAX_UPG) {
            int cost = hpCost(g_hpLvl);
            char upBuf[64];
            snprintf(upBuf, sizeof(upBuf), "Upgrade HP  $%d", cost);
            CCLabelBMFont* upLbl = CCLabelBMFont::create(upBuf, "chatFont.fnt");
            upLbl->setScale(0.48f);
            bool canAfford = g_coins >= cost;
            upLbl->setColor(canAfford ? ccc3(255, 160, 80) : ccc3(130, 70, 50));
            CCMenuItemLabel* upBtn = CCMenuItemLabel::create(upLbl, this, menu_selector(ShopLayer::onUpgradeHp));
            upBtn->setPosition(ccp(cx, topY - 34.0f));
            upBtn->setTag(9999);
            shopMenu->addChild(upBtn);
        } else {
            CCLabelBMFont* maxLbl = CCLabelBMFont::create("MAX HP REACHED", "chatFont.fnt");
            maxLbl->setScale(0.45f);
            maxLbl->setColor(ccc3(255, 80, 80));
            maxLbl->setPosition(ccp(cx, topY - 34.0f));
            upgNode->addChild(maxLbl);
        }

        float killBonus = powf(calcMaxHp() / 100.0f, 0.72f);
        char bonusBuf[64];
        snprintf(bonusBuf, sizeof(bonusBuf), "Kill coins: ~%.1fx base", killBonus);
        CCLabelBMFont* bonusLbl = CCLabelBMFont::create(bonusBuf, "chatFont.fnt");
        bonusLbl->setScale(0.4f);
        bonusLbl->setColor(ccc3(200, 200, 80));
        bonusLbl->setPosition(ccp(cx, topY - 50.0f));
        upgNode->addChild(bonusLbl);
    }

    void rebuildDynamicContent() {
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        float panelW = winSize.width * 0.88f;
        float panelH = winSize.height * 0.86f;
        float panelX = (winSize.width - panelW) * 0.5f;
        float panelY = (winSize.height - panelH) * 0.5f;

        CCMenu* shopMenu = nullptr;
        for (CCNode* child : this->getChildrenExt()) {
            CCMenu* m = dynamic_cast<CCMenu*>(child);
            if (m && m->getTouchPriority() == -130) {
                shopMenu = m;
                break;
            }
        }
        if (!shopMenu) return;

        std::vector<CCNode*> toRemove;
        for (CCNode* n : shopMenu->getChildrenExt()) {
            int tag = n->getTag();
            if (tag >= 0 && tag < WEP_COUNT) toRemove.push_back(n);
            if (tag == 9999) toRemove.push_back(n);
        }
        for (CCNode* n : toRemove) {
            n->removeFromParentAndCleanup(true);
        }

        buildWeapList(shopMenu, winSize, panelX, panelY, panelH, panelW);
        buildUpgSection(shopMenu, winSize, panelX, panelY, panelH, panelW);
        refreshCoins();
    }

    void onUpgradeHp(CCObject*) {
        if (g_hpLvl >= HP_MAX_UPG) return;
        int cost = hpCost(g_hpLvl);
        if (g_coins < cost) {
            FLAlertLayer::create("Broke", "Not enough coins", "OK")->show();
            return;
        }
        g_coins -= cost;
        g_hpLvl++;
        rebuildDynamicContent();
    }

    void setupVictimMenu(CCMenu* shopMenu, CCSize winSize, float panelX, float panelY, float panelH, float panelW) {
        float colW = panelW * 0.55f;
        float rightCx = panelX + colW + (panelW * 0.45f) * 0.5f;
        float topY = panelY + panelH - 72.0f;

        CCLabelBMFont* victimTitle = CCLabelBMFont::create("VICTIM SKIN", "chatFont.fnt");
        victimTitle->setScale(0.55f);
        victimTitle->setColor(ccc3(255, 100, 100));
        victimTitle->setPosition(ccp(rightCx, topY));
        this->addChild(victimTitle, 3);

        prevPlr = SimplePlayer::create(0);
        refreshPreview();
        prevPlr->setScale(1.8f);
        prevPlr->setPosition(ccp(rightCx, topY - 48.0f));
        this->addChild(prevPlr, 4);

        CCLabelBMFont* arrowFL = CCLabelBMFont::create("<", "bigFont.fnt");
        arrowFL->setScale(0.45f);
        CCMenuItemLabel* prevFrameBtn = CCMenuItemLabel::create(arrowFL, this, menu_selector(ShopLayer::onPrevFrame));
        prevFrameBtn->setPosition(ccp(rightCx - 40.0f, topY - 48.0f));
        shopMenu->addChild(prevFrameBtn);

        CCLabelBMFont* arrowFR = CCLabelBMFont::create(">", "bigFont.fnt");
        arrowFR->setScale(0.45f);
        CCMenuItemLabel* nextFrameBtn = CCMenuItemLabel::create(arrowFR, this, menu_selector(ShopLayer::onNextFrame));
        nextFrameBtn->setPosition(ccp(rightCx + 40.0f, topY - 48.0f));
        shopMenu->addChild(nextFrameBtn);

        CCLabelBMFont* colorTitle = CCLabelBMFont::create("COLORS", "chatFont.fnt");
        colorTitle->setScale(0.44f);
        colorTitle->setColor(ccc3(180, 180, 255));
        colorTitle->setPosition(ccp(rightCx, topY - 86.0f));
        this->addChild(colorTitle, 3);

        CCLabelBMFont* c1Lbl = CCLabelBMFont::create("P1", "chatFont.fnt");
        c1Lbl->setScale(0.44f);
        c1Lbl->setColor(ccc3(180, 180, 255));
        c1Lbl->setPosition(ccp(rightCx - 22.0f, topY - 104.0f));
        this->addChild(c1Lbl, 3);

        CCLabelBMFont* arrowC1L = CCLabelBMFont::create("<", "chatFont.fnt");
        arrowC1L->setScale(0.44f);
        CCMenuItemLabel* prevC1Btn = CCMenuItemLabel::create(arrowC1L, this, menu_selector(ShopLayer::onPrevC1));
        prevC1Btn->setPosition(ccp(rightCx - 44.0f, topY - 104.0f));
        shopMenu->addChild(prevC1Btn);

        CCLabelBMFont* arrowC1R = CCLabelBMFont::create(">", "chatFont.fnt");
        arrowC1R->setScale(0.44f);
        CCMenuItemLabel* nextC1Btn = CCMenuItemLabel::create(arrowC1R, this, menu_selector(ShopLayer::onNextC1));
        nextC1Btn->setPosition(ccp(rightCx, topY - 104.0f));
        shopMenu->addChild(nextC1Btn);

        CCLabelBMFont* c2Lbl = CCLabelBMFont::create("P2", "chatFont.fnt");
        c2Lbl->setScale(0.44f);
        c2Lbl->setColor(ccc3(255, 180, 180));
        c2Lbl->setPosition(ccp(rightCx + 22.0f, topY - 104.0f));
        this->addChild(c2Lbl, 3);

        CCLabelBMFont* arrowC2L = CCLabelBMFont::create("<", "chatFont.fnt");
        arrowC2L->setScale(0.44f);
        CCMenuItemLabel* prevC2Btn = CCMenuItemLabel::create(arrowC2L, this, menu_selector(ShopLayer::onPrevC2));
        prevC2Btn->setPosition(ccp(rightCx + 8.0f, topY - 104.0f));
        shopMenu->addChild(prevC2Btn);

        CCLabelBMFont* arrowC2R = CCLabelBMFont::create(">", "chatFont.fnt");
        arrowC2R->setScale(0.44f);
        CCMenuItemLabel* nextC2Btn = CCMenuItemLabel::create(arrowC2R, this, menu_selector(ShopLayer::onNextC2));
        nextC2Btn->setPosition(ccp(rightCx + 50.0f, topY - 104.0f));
        shopMenu->addChild(nextC2Btn);

        CCLabelBMFont* randLbl = CCLabelBMFont::create("Random!", "chatFont.fnt");
        randLbl->setScale(0.52f);
        randLbl->setColor(ccc3(255, 160, 80));
        CCMenuItemLabel* randBtn = CCMenuItemLabel::create(randLbl, this, menu_selector(ShopLayer::onRandom));
        randBtn->setPosition(ccp(rightCx, topY - 130.0f));
        shopMenu->addChild(randBtn);

        CCLayerColor* applyBg = CCLayerColor::create(ccc4(40, 160, 80, 200));
        applyBg->setContentSize(CCSizeMake(80.0f, 22.0f));
        applyBg->setPosition(ccp(rightCx - 40.0f, topY - 158.0f));
        this->addChild(applyBg, 3);

        CCLabelBMFont* applyLbl = CCLabelBMFont::create("Apply", "chatFont.fnt");
        applyLbl->setScale(0.52f);
        applyLbl->setColor(ccc3(255, 255, 255));
        CCMenuItemLabel* applyBtn = CCMenuItemLabel::create(applyLbl, this, menu_selector(ShopLayer::onApply));
        applyBtn->setPosition(ccp(rightCx, topY - 147.0f));
        shopMenu->addChild(applyBtn);
    }

    void refreshPreview() {
        GameManager* gm = GameManager::sharedState();
        prevPlr->updatePlayerFrame(g_vicFrames[pFrame], IconType::Cube);
        prevPlr->setColor(gm->colorForIdx(g_vicCols1[pC1]));
        prevPlr->setSecondColor(gm->colorForIdx(g_vicCols2[pC2]));
        prevPlr->updateColors();
    }

    void onPrevFrame(CCObject*) {
        pFrame--;
        if (pFrame < 0) pFrame = VIC_POOL_SIZE - 1;
        refreshPreview();
    }

    void onNextFrame(CCObject*) {
        pFrame++;
        if (pFrame >= VIC_POOL_SIZE) pFrame = 0;
        refreshPreview();
    }

    void onPrevC1(CCObject*) {
        pC1--;
        if (pC1 < 0) pC1 = COL_POOL_SIZE - 1;
        refreshPreview();
    }

    void onNextC1(CCObject*) {
        pC1++;
        if (pC1 >= COL_POOL_SIZE) pC1 = 0;
        refreshPreview();
    }

    void onPrevC2(CCObject*) {
        pC2--;
        if (pC2 < 0) pC2 = COL_POOL_SIZE - 1;
        refreshPreview();
    }

    void onNextC2(CCObject*) {
        pC2++;
        if (pC2 >= COL_POOL_SIZE) pC2 = 0;
        refreshPreview();
    }

    void onRandom(CCObject*) {
        pFrame = rand() % VIC_POOL_SIZE;
        pC1 = rand() % COL_POOL_SIZE;
        pC2 = rand() % COL_POOL_SIZE;
        refreshPreview();
    }

    void onApply(CCObject*) {
        g_victimFrame = pFrame;
        g_victimColor1 = pC1;
        g_victimColor2 = pC2;
        if (victimCB) victimCB(g_victimFrame, g_victimColor1, g_victimColor2);
    }

    void refreshCoins() {
        char buf[64];
        snprintf(buf, sizeof(buf), "Coins: $%d", g_coins);
        coinText->setString(buf);
    }

    void onBuy(CCObject* sender) {
        CCNode* btn = dynamic_cast<CCNode*>(sender);
        if (!btn) return;
        int idx = btn->getTag();
        if (idx < 0 || idx >= WEP_COUNT) return;
        if (g_weapUnlock[idx]) return;
        if (g_coins < g_weaps[idx].cost) {
            FLAlertLayer::create("Broke", "Not enough coins", "OK")->show();
            return;
        }
        g_coins -= g_weaps[idx].cost;
        g_weapUnlock[idx] = true;
        rebuildDynamicContent();
    }

    void onClose(CCObject*) {
        CCTouchDispatcher::get()->removeDelegate(this);
        if (closeCB) closeCB();
        this->removeFromParentAndCleanup(true);
    }

    bool ccTouchBegan(CCTouch*, CCEvent*) override { return true; }
    void ccTouchMoved(CCTouch*, CCEvent*) override {}
    void ccTouchEnded(CCTouch*, CCEvent*) override {}
    void ccTouchCancelled(CCTouch*, CCEvent*) override {}

    void show() {
        CCScene* scene = CCDirector::sharedDirector()->getRunningScene();
        int topZ = 0;
        for (CCNode* kid : scene->getChildrenExt()) {
            if (kid->getZOrder() > topZ) topZ = kid->getZOrder();
        }
        scene->addChild(this, topZ + 1);
    }
};