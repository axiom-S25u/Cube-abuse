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
    { "Hammer",   0,     15,  ccc3(255, 255, 255), "hammer.png"_spr   },
    { "Boot",     0,     10,  ccc3(255, 255, 255), "boot.png"_spr     },
    { "Knife",    0,     20,  ccc3(255, 255, 255), "knife.png"_spr    },
    { "Brick",    50,    30,  ccc3(255, 255, 255), "brick.png"_spr    },
    { "Bat",      200,   45,  ccc3(255, 255, 255), "bat.png"_spr      },
    { "Taser",    500,   60,  ccc3(255, 255, 255), "taser.png"_spr    },
    { "Chainsaw", 1500,  85,  ccc3(255, 255, 255), "chainsaw.png"_spr },
    { "Nuke",     15000, 1000000, ccc3(255, 255, 255), "nuke.png"_spr     }, //damage *should* be enough
}; // this is for ery, if it wasnt clean enough before
inline const int WEP_COUNT = 8;
inline bool g_weapUnlock[] = { true, true, true, false, false, false, false, false };

inline double g_lastNuke = -99999.0;

inline const int HP_MAX_UPG = 999999;
inline int g_hpLvl = 0;

inline const int DMG_MAX_UPG = 999999;
inline int g_dmgLvl = 0;

inline const int COIN_MAX_UPG = 999999;
inline int g_coinLvl = 0;

struct WepEnt {
    CCSprite* spr;
    int id;
    bool dragging;
    CCPoint offset;
    bool hasHit;
    float idleTimer;
    bool dying;
};

inline void saveWeaps() {
    for (int i = 0; i < WEP_COUNT; i++) {
        Mod::get()->setSavedValue<bool>(fmt::format("weapon_unlocked_{}", i), g_weapUnlock[i]);
    }
    Mod::get()->setSavedValue<int>("hp_upgrade_level", g_hpLvl);
    Mod::get()->setSavedValue<int>("dmg_upgrade_level", g_dmgLvl);
    Mod::get()->setSavedValue<int>("coin_upgrade_level", g_coinLvl);
    Mod::get()->setSavedValue<int>("total_coins", g_coins);
}

inline void loadWeaps() {
    for (int i = 0; i < WEP_COUNT; i++) {
        std::string key = fmt::format("weapon_unlocked_{}", i);
        if (Mod::get()->hasSavedValue(key)) {
            g_weapUnlock[i] = Mod::get()->getSavedValue<bool>(key);
        }
    }
    if (Mod::get()->hasSavedValue("hp_upgrade_level")) g_hpLvl = Mod::get()->getSavedValue<int>("hp_upgrade_level");
    if (Mod::get()->hasSavedValue("dmg_upgrade_level")) g_dmgLvl = Mod::get()->getSavedValue<int>("dmg_upgrade_level");
    if (Mod::get()->hasSavedValue("coin_upgrade_level")) g_coinLvl = Mod::get()->getSavedValue<int>("coin_upgrade_level");
    if (Mod::get()->hasSavedValue("total_coins")) g_coins = Mod::get()->getSavedValue<int>("total_coins");
}

static float calcMaxHp() {
    return 100.0f * powf(1.35f, (float)g_hpLvl);
}

static float calcDmgBonus() {
    return g_dmgLvl * 40.0f;
}

static float calcCoinMult() {
    return 1.0f + g_coinLvl * 0.5f;
}

static int hpCost(int lvl) {
    return (int)(200 * powf(2.4f, (float)lvl));
}

static int dmgCost(int lvl) {
    return (int)(300 * powf(2.5f, (float)lvl));
}

static int coinCost(int lvl) {
    return (int)(500 * powf(2.6f, (float)lvl));
}

static int killReward(float maxHp) {
    float base = 30.0f + rand() % 60;
    float bonus = powf(maxHp / 100.0f, 0.72f);
    return (int)(base * bonus * calcCoinMult());
}

class ShopLayer : public Popup {
public:
    std::function<void()> closeCB;
    CCLabelBMFont* coinText;
    CCNode* weapNode;
    CCNode* upgNode;

    static ShopLayer* create(std::function<void()> cb) {
        auto ret = new ShopLayer();
        if (ret && ret->init(440.f, 290.f)) {
            ret->closeCB = cb;
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool init(float w, float h) {
        if (!Popup::init(w, h)) return false;

        float panelW = m_mainLayer->getContentSize().width;
        float panelH = m_mainLayer->getContentSize().height;

        this->setTitle("Shop");

        coinText = CCLabelBMFont::create("", "chatFont.fnt");
        coinText->setPosition(ccp(panelW * 0.85f, panelH - 25.0f));
        m_mainLayer->addChild(coinText, topZ(m_mainLayer) + 1);
        refreshCoins();

        CCMenu* shopMenu = CCMenu::create();
        shopMenu->setPosition(ccp(0.0f, 0.0f));
        m_mainLayer->addChild(shopMenu, topZ(m_mainLayer) + 1);

        weapNode = CCNode::create();
        m_mainLayer->addChild(weapNode, topZ(m_mainLayer) + 1);
        buildWeapList(shopMenu, panelW, panelH);

        upgNode = CCNode::create();
        m_mainLayer->addChild(upgNode, topZ(m_mainLayer) + 1);
        buildUpgSection(shopMenu, panelW, panelH);

        return true;
    }

    void onClose(CCObject* o) override {
        saveWeaps();
        if (closeCB) closeCB();
        Popup::onClose(o);
    }

    void onUpgradeHp(CCObject*) {
        int cost = hpCost(g_hpLvl);
        if (g_coins < cost) { FLAlertLayer::create("Broke", "Not enough coins", "OK")->show(); return; }
        g_coins -= cost; g_hpLvl++; saveWeaps(); rebuildDynamicContent();
    }

    void onUpgradeDmg(CCObject*) {
        int cost = dmgCost(g_dmgLvl);
        if (g_coins < cost) { FLAlertLayer::create("Broke", "Not enough coins", "OK")->show(); return; }
        g_coins -= cost; g_dmgLvl++; saveWeaps(); rebuildDynamicContent();
    }

    void onUpgradeCoin(CCObject*) {
        int cost = coinCost(g_coinLvl);
        if (g_coins < cost) { FLAlertLayer::create("Broke", "Not enough coins", "OK")->show(); return; }
        g_coins -= cost; g_coinLvl++; saveWeaps(); rebuildDynamicContent();
    }

    void rebuildDynamicContent(); // decl

    void buildWeapList(CCMenu* shopMenu, float panelW, float panelH) {
        if (weapNode) weapNode->removeAllChildren();
        float colW = panelW * 0.55f;
        float leftX = panelW * 0.05f + colW * 0.28f;
        float startY = panelH - 68.0f;
        float gap = 24.0f;
        for (int i = 0; i < WEP_COUNT; i++) {
            WepDef& wd = g_weaps[i];
            float rowY = startY - gap * (i + 1);
            CCLayerColor* rowBg = CCLayerColor::create(
                (i % 2 == 0) ? ccc4(255, 255, 255, 10) : ccc4(0, 0, 0, 10),
                colW - 12.0f, gap - 2.0f
            );
            rowBg->setPosition(ccp(6.0f, rowY - (gap - 2.0f) * 0.5f));
            weapNode->addChild(rowBg, topZ(weapNode) + 1);

            CCSprite* iconSpr = CCSprite::create(wd.iconName);
            if (!iconSpr) iconSpr = CCSprite::createWithSpriteFrameName("edit_eBtn_001.png");
            if (iconSpr) {
                iconSpr->setPosition(ccp(leftX - 22.0f, rowY));
                iconSpr->setScale(18.0f / iconSpr->getContentSize().height);
                weapNode->addChild(iconSpr, topZ(weapNode) + 1);
            }

            CCLabelBMFont* nameLbl = CCLabelBMFont::create(wd.name, "chatFont.fnt");
            nameLbl->setScale(0.46f);
            nameLbl->setAnchorPoint(ccp(0.0f, 0.5f));
            nameLbl->setPosition(ccp(leftX - 8.0f, rowY));
            weapNode->addChild(nameLbl, topZ(weapNode) + 1);

            if (i == 7) {
                CCLabelBMFont* coolLbl = CCLabelBMFont::create("5m Cooldown", "chatFont.fnt");
                coolLbl->setScale(0.25f);
                coolLbl->setColor(ccc3(255, 100, 100));
                coolLbl->setPosition(ccp(leftX + 25.0f, rowY - 8.0f));
                weapNode->addChild(coolLbl, topZ(coolLbl) + 1);
            }

            if (g_weapUnlock[i]) {
                CCLabelBMFont* oL = CCLabelBMFont::create("OWNED", "chatFont.fnt");
                oL->setScale(0.38f);
                oL->setColor(ccc3(80, 255, 120));
                oL->setPosition(ccp(leftX + 122.0f, rowY));
                weapNode->addChild(oL, topZ(weapNode) + 1);
            } else {
                ButtonSprite* buyBtnSpr = ButtonSprite::create(fmt::format("${}", wd.cost).c_str(), "chatFont.fnt", "GJ_button_01.png", 0.5f);
                CCMenuItemSpriteExtra* bB = CCMenuItemSpriteExtra::create(buyBtnSpr, this, menu_selector(ShopLayer::onBuy));
                bB->setPosition(ccp(leftX + 122.0f, rowY));
                bB->setTag(i);
                shopMenu->addChild(bB, topZ(shopMenu) + 1);
            }
        }
    }

    void buildUpgSection(CCMenu* shopMenu, float panelW, float panelH) {
        if (upgNode) upgNode->removeAllChildren();
        float cx = panelW * 0.75f;
        float topY = panelH - 42.0f;

        CCLabelBMFont* hpInfo = CCLabelBMFont::create(
            fmt::format("HP: {:.0f} [Lv {}]", calcMaxHp(), g_hpLvl).c_str(),
            "chatFont.fnt"
        );
        hpInfo->setScale(0.45f);
        hpInfo->setColor(ccc3(100, 230, 100));
        hpInfo->setPosition(ccp(cx, topY - 16.0f));
        upgNode->addChild(hpInfo, topZ(upgNode) + 1);

        ButtonSprite* hpBtnSpr = ButtonSprite::create(fmt::format("Upgrade HP  ${}", hpCost(g_hpLvl)).c_str(), "chatFont.fnt", "GJ_button_01.png", 0.5f);
        CCMenuItemSpriteExtra* uBtn = CCMenuItemSpriteExtra::create(hpBtnSpr, this, menu_selector(ShopLayer::onUpgradeHp));
        uBtn->setPosition(ccp(cx, topY - 40.0f));
        uBtn->setTag(9999);
        shopMenu->addChild(uBtn, topZ(shopMenu) + 1);

        CCLabelBMFont* dmgInfo = CCLabelBMFont::create(
            fmt::format("+{:.0f} DMG bonus [Lv {}]", calcDmgBonus(), g_dmgLvl).c_str(),
            "chatFont.fnt"
        );
        dmgInfo->setScale(0.45f);
        dmgInfo->setColor(ccc3(255, 140, 80));
        dmgInfo->setPosition(ccp(cx, topY - 74.0f));
        upgNode->addChild(dmgInfo, topZ(upgNode) + 1);

        ButtonSprite* dmgBtnSpr = ButtonSprite::create(fmt::format("Upgrade DMG  ${}", dmgCost(g_dmgLvl)).c_str(), "chatFont.fnt", "GJ_button_01.png", 0.5f);
        CCMenuItemSpriteExtra* dBtnFixed = CCMenuItemSpriteExtra::create(dmgBtnSpr, this, menu_selector(ShopLayer::onUpgradeDmg));
        dBtnFixed->setPosition(ccp(cx, topY - 98.0f));
        dBtnFixed->setTag(10000);
        shopMenu->addChild(dBtnFixed, topZ(shopMenu) + 1);

        CCLabelBMFont* coinInfo = CCLabelBMFont::create(
            fmt::format("x{:.1f} Multi [Lv {}]", calcCoinMult(), g_coinLvl).c_str(),
            "chatFont.fnt"
        );
        coinInfo->setScale(0.45f);
        coinInfo->setColor(ccc3(255, 215, 0));
        coinInfo->setPosition(ccp(cx, topY - 132.0f));
        upgNode->addChild(coinInfo, topZ(upgNode) + 1);

        ButtonSprite* coinBtnSpr = ButtonSprite::create(fmt::format("Upgrade Greed ${}", coinCost(g_coinLvl)).c_str(), "chatFont.fnt", "GJ_button_01.png", 0.5f);
        CCMenuItemSpriteExtra* cBtn = CCMenuItemSpriteExtra::create(coinBtnSpr, this, menu_selector(ShopLayer::onUpgradeCoin));
        cBtn->setPosition(ccp(cx, topY - 156.0f));
        cBtn->setTag(10001);
        shopMenu->addChild(cBtn, topZ(shopMenu) + 1);
    }

    void refreshCoins() {
        if (coinText) coinText->setString(fmt::format("Coins: ${}", g_coins).c_str());
    }

    void onBuy(CCObject* sender) {
        int idx = sender->getTag();
        if (g_coins < g_weaps[idx].cost) { FLAlertLayer::create("Broke", "Not enough coins", "OK")->show(); return; }
        g_coins -= g_weaps[idx].cost;
        g_weapUnlock[idx] = true;
        saveWeaps();
        rebuildDynamicContent();
    }
};

inline void ShopLayer::rebuildDynamicContent() {
    float panelW = m_mainLayer->getContentSize().width;
    float panelH = m_mainLayer->getContentSize().height;

    CCMenu* shopMenu = nullptr;
    for (auto node : m_mainLayer->getChildrenExt()) {
        CCMenu* m = dynamic_cast<CCMenu*>(node);
        if (m) { shopMenu = m; break; }
    }
    if (!shopMenu) return;

    std::vector<CCNode*> toRemove;
    for (auto n : shopMenu->getChildrenExt()) {
        int tag = n->getTag();
        if ((tag >= 0 && tag < WEP_COUNT) || tag == 9999 || tag == 10000 || tag == 10001) toRemove.push_back(n);
    }
    for (CCNode* n : toRemove) n->removeFromParent();

    buildWeapList(shopMenu, panelW, panelH);
    buildUpgSection(shopMenu, panelW, panelH);
    refreshCoins();
}