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
#include <cstdint>
#include <chrono>

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
inline int64_t g_coins = 0;
inline bool g_bodyInit = false;

struct BodyShit {
    CCNode* node = nullptr;
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
    int64_t cost;
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
    { "Nuke",     15000, 1000000000, ccc3(255, 255, 255), "nuke.png"_spr     }, //damage *should* be enough, as a note it wasnt
    { "Sword",    400,   35,  ccc3(220, 220, 255), "sword.png"_spr   },
    { "Crowbar",  700,   50,  ccc3(255, 100, 80),  "crowbar.png"_spr   },
    { "Anvil",    1200,  95,  ccc3(120, 120, 130), "anvil.png"_spr   }, // minecraftttttttttttttttttttttttttttt
    { "FryPan",   1000,  65,  ccc3(80, 80, 80),    "frypan.png"_spr    },
    { "Pickaxe",  800,   55,  ccc3(200, 150, 100), "pick.png"_spr   }, // minecrafttttttttttttt
    { "Sledge",   2200,  130, ccc3(180, 180, 220), "sledge.png"_spr },
    { "Spikes",   25000, 35,  ccc3(200, 120, 255), "spike_01_001.png" },
}; // this is for ery, if it wasnt clean enough before
inline const int WEP_COUNT = 15;
inline const int SPIKE_WEP_IDX = 14;
inline bool g_weapUnlock[] = { true, true, true, false, false, false, false, false, false, false, false, false, false, false, false };

inline double g_lastNuke = -99999.0;
inline double g_lastSpike = -99999.0;

inline const int HP_MAX_UPG = 60;
inline int g_hpLvl = 0;

inline const int DMG_MAX_UPG = 60;
inline int g_dmgLvl = 0;

inline const int COIN_MAX_UPG = 60;
inline int g_coinLvl = 0;

inline const int SPIKE_AIM_MAX = 14;
inline int g_spikeAimLvl = 0;

inline const int SPIKE_MORE_MAX = 10;
inline int g_spikeMoreLvl = 0;

inline const int SPIKE_RAIN_MAX = 10;
inline int g_spikeRainLvl = 0;

struct WepEnt {
    CCSprite* spr = nullptr;
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
    Mod::get()->setSavedValue<int>("spike_aim_level", g_spikeAimLvl);
    Mod::get()->setSavedValue<int>("spike_more_level", g_spikeMoreLvl);
    Mod::get()->setSavedValue<int>("spike_rain_level", g_spikeRainLvl);
    Mod::get()->setSavedValue<int64_t>("total_coins_v2", g_coins);
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
    if (Mod::get()->hasSavedValue("spike_aim_level")) g_spikeAimLvl = Mod::get()->getSavedValue<int>("spike_aim_level");
    if (Mod::get()->hasSavedValue("spike_more_level")) g_spikeMoreLvl = Mod::get()->getSavedValue<int>("spike_more_level");
    if (Mod::get()->hasSavedValue("spike_rain_level")) g_spikeRainLvl = Mod::get()->getSavedValue<int>("spike_rain_level");
    if (Mod::get()->hasSavedValue("total_coins_v2")) {
        g_coins = Mod::get()->getSavedValue<int64_t>("total_coins_v2");
    } else if (Mod::get()->hasSavedValue("total_coins")) {
        g_coins = (int64_t)Mod::get()->getSavedValue<int>("total_coins");
    }
}

static float calcMaxHp() {
    return 100.0f * powf(1.15f, (float)g_hpLvl);
}

static float calcDmgMulti() {
    return 1.0f + (float)g_dmgLvl * 0.25f;
}

static float calcCoinMult() {
    return 1.0f + g_coinLvl * 0.15f;
}

static int64_t hpCost(int lvl) {
    if (lvl >= HP_MAX_UPG) return -1;
    return (int64_t)(200 * powf(1.8f, (float)lvl));
}

static int64_t dmgCost(int lvl) {
    if (lvl >= DMG_MAX_UPG) return -1;
    return (int64_t)(300 * powf(1.9f, (float)lvl));
}

static int64_t coinCost(int lvl) {
    if (lvl >= COIN_MAX_UPG) return -1;
    return (int64_t)(500 * powf(2.0f, (float)lvl));
}

static float spikeAimPct(int lvl) {
    if (lvl <= 0) return 0.0f;
    if (lvl > SPIKE_AIM_MAX) lvl = SPIKE_AIM_MAX;
    return 0.10f + (float)lvl * 0.10f;
}

static float spikeRateMult(int lvl) {
    if (lvl < 0) lvl = 0;
    if (lvl > SPIKE_MORE_MAX) lvl = SPIKE_MORE_MAX;
    return 1.0f + (float)lvl * 0.15f;
}

static int64_t spikeAimCost(int lvl) {
    if (lvl >= SPIKE_AIM_MAX) return -1;
    if (lvl >= 7) return (int64_t)50000 * (int64_t)(lvl - 6);
    return (int64_t)15000 * (int64_t)(lvl + 1);
}

static float spikeRainBonus(int lvl) {
    if (lvl < 0) lvl = 0;
    if (lvl > SPIKE_RAIN_MAX) lvl = SPIKE_RAIN_MAX;
    return (float)lvl * 3.0f;
}

static int64_t spikeRainCost(int lvl) {
    if (lvl >= SPIKE_RAIN_MAX) return -1;
    return (int64_t)15000;
}

static int64_t spikeMoreCost(int lvl) {
    if (lvl >= SPIKE_MORE_MAX) return -1;
    return (int64_t)17000 * (int64_t)(lvl + 1);
}

static int64_t killReward(float maxHp) {
    float base = 30.0f + rand() % 60;
    float bonus = powf(maxHp / 100.0f, 0.72f);
    int64_t res = (int64_t)(base * bonus * calcCoinMult());
    if (res < 0) res = 999999;
    return res;
}

class ShopLayer : public Popup {
public:
    std::function<void()> closeCB;
    std::function<void()> buyCB;
    CCLabelBMFont* coinText = nullptr;
    CCNode* upgNode = nullptr;
    ScrollLayer* weapScroll = nullptr;
    CCMenu* weapMenu = nullptr;

    static ShopLayer* create(std::function<void()> cb, std::function<void()> bcb) {
        auto ret = new ShopLayer();
        if (ret && ret->init(440.f, 290.f)) {
            ret->closeCB = cb;
            ret->buyCB = bcb;
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool init(float w, float h) {
        if (!Popup::init(w, h)) return false;
        m_mainLayer->setID("shop-main-layer");
        float panelW = m_mainLayer->getContentSize().width;
        float panelH = m_mainLayer->getContentSize().height;
        this->setTitle("Shop");
        coinText = CCLabelBMFont::create("", "chatFont.fnt");
        coinText->setPosition(ccp(panelW * 0.85f, panelH - 25.0f));
        m_mainLayer->addChild(coinText, topZ(m_mainLayer) + 1);
        refreshCoins();
        CCMenu* shopMenu = CCMenu::create();
        shopMenu->setPosition(ccp(0.0f, 0.0f));
        shopMenu->setID("purchase-menu");
        m_mainLayer->addChild(shopMenu, topZ(m_mainLayer) + 1);
        float colW = panelW * 0.55f;
        float scrollH = panelH - 60.0f;
        weapScroll = ScrollLayer::create(CCSizeMake(colW, scrollH));
        weapScroll->setPosition(ccp(8.0f, 8.0f));
        weapScroll->setAnchorPoint(ccp(0.f, 0.f));
        weapScroll->ignoreAnchorPointForPosition(false);
        m_mainLayer->addChild(weapScroll, topZ(m_mainLayer) + 1);
        weapMenu = CCMenu::create();
        weapMenu->setPosition(ccp(0.0f, 0.0f));
        weapMenu->setContentSize(CCSizeMake(colW, scrollH));
        weapScroll->m_contentLayer->addChild(weapMenu, topZ(weapScroll->m_contentLayer) + 1);
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
        int64_t cost = hpCost(g_hpLvl);
        if (cost < 0) return;
        if (g_coins < cost) { FLAlertLayer::create("Broke", "Not enough coins", "OK")->show(); return; }
        g_coins -= cost; g_hpLvl++; saveWeaps(); rebuildDynamicContent();
        if (buyCB) buyCB();
    }

    void onUpgradeDmg(CCObject*) {
        int64_t cost = dmgCost(g_dmgLvl);
        if (cost < 0) return;
        if (g_coins < cost) { FLAlertLayer::create("Broke", "Not enough coins", "OK")->show(); return; }
        g_coins -= cost; g_dmgLvl++; saveWeaps(); rebuildDynamicContent();
        if (buyCB) buyCB();
    }

    void onUpgradeCoin(CCObject*) {
        int64_t cost = coinCost(g_coinLvl);
        if (cost < 0) return;
        if (g_coins < cost) { FLAlertLayer::create("Broke", "Not enough coins", "OK")->show(); return; }
        g_coins -= cost; g_coinLvl++; saveWeaps(); rebuildDynamicContent();
        if (buyCB) buyCB();
    }

    void rebuildDynamicContent();

    void buildWeapList(CCMenu* shopMenu, float panelW, float panelH) {
        if (weapMenu) weapMenu->removeAllChildren();
        float colW = panelW * 0.55f;
        float leftX = colW * 0.33f;
        float gap = 22.0f;
        float padTop = 4.0f;
        float contentH = gap * (float)WEP_COUNT + padTop * 2.0f;
        float scrollH = panelH - 60.0f;
        if (contentH < scrollH) contentH = scrollH;
        if (weapScroll) {
            weapScroll->m_contentLayer->setContentSize(CCSizeMake(colW, contentH));
            weapMenu->setContentSize(CCSizeMake(colW, contentH));
        }
        for (int i = 0; i < WEP_COUNT; i++) {
            WepDef& wd = g_weaps[i];
            float rowY = contentH - padTop - gap * ((float)i + 0.5f);
            CCLayerColor* rowBg = CCLayerColor::create(
                (i % 2 == 0) ? ccc4(255, 255, 255, 10) : ccc4(0, 0, 0, 10),
                colW - 12.0f, gap - 2.0f
            );
            rowBg->setPosition(ccp(6.0f, rowY - (gap - 2.0f) * 0.5f));
            weapMenu->addChild(rowBg, topZ(weapMenu) + 1);
            CCSprite* iconSpr = nullptr;
            if (i == SPIKE_WEP_IDX) {
                iconSpr = CCSprite::createWithSpriteFrameName(wd.iconName);
            } else {
                iconSpr = CCSprite::create(wd.iconName);
            }
            if (!iconSpr) iconSpr = CCSprite::createWithSpriteFrameName(wd.iconName);
            if (!iconSpr) iconSpr = CCSprite::createWithSpriteFrameName("edit_eBtn_001.png");
            if (iconSpr) {
                iconSpr->setPosition(ccp(leftX - 22.0f, rowY));
                iconSpr->setScale(18.0f / iconSpr->getContentSize().height);
                weapMenu->addChild(iconSpr, topZ(weapMenu) + 1);
            }
            CCLabelBMFont* nameLbl = CCLabelBMFont::create(wd.name, "chatFont.fnt");
            nameLbl->setScale(0.46f);
            nameLbl->setAnchorPoint(ccp(0.0f, 0.5f));
            nameLbl->setPosition(ccp(leftX - 8.0f, rowY));
            weapMenu->addChild(nameLbl, topZ(weapMenu) + 1);
            if (i == 7) {
                CCLabelBMFont* coolLbl = CCLabelBMFont::create("5m Cooldown", "chatFont.fnt");
                coolLbl->setScale(0.25f);
                coolLbl->setColor(ccc3(255, 100, 100));
                coolLbl->setPosition(ccp(leftX + 25.0f, rowY - 8.0f));
                weapMenu->addChild(coolLbl, topZ(weapMenu) + 1);
            }
            if (i == SPIKE_WEP_IDX) {
                CCLabelBMFont* coolLbl = CCLabelBMFont::create("200s CD, 30s rain", "chatFont.fnt");
                coolLbl->setScale(0.25f);
                coolLbl->setColor(ccc3(200, 120, 255));
                coolLbl->setPosition(ccp(leftX + 30.0f, rowY - 8.0f));
                weapMenu->addChild(coolLbl, topZ(weapMenu) + 1);
            }
            if (g_weapUnlock[i]) {
                CCLabelBMFont* oL = CCLabelBMFont::create("OWNED", "chatFont.fnt");
                oL->setScale(0.38f);
                oL->setColor(ccc3(80, 255, 120));
                oL->setPosition(ccp(leftX + 122.0f, rowY));
                weapMenu->addChild(oL, topZ(weapMenu) + 1);
            } else {
                ButtonSprite* buyBtnSpr = ButtonSprite::create(fmt::format("${}", wd.cost).c_str(), "chatFont.fnt", "GJ_button_01.png", 0.5f);
                CCMenuItemSpriteExtra* bB = CCMenuItemSpriteExtra::create(buyBtnSpr, this, menu_selector(ShopLayer::onBuy));
                bB->setPosition(ccp(leftX + 122.0f, rowY));
                bB->setTag(i);
                weapMenu->addChild(bB, topZ(weapMenu) + 1);
            }
        }
        if (weapScroll) {
            weapScroll->scrollToTop();
            handleTouchPriority(this);
        }
    }

    void buildUpgSection(CCMenu* shopMenu, float panelW, float panelH) {
        if (upgNode) upgNode->removeAllChildren();
        float cx = panelW * 0.75f;
        float topY = panelH - 42.0f;
        CCLabelBMFont* hpInfo = CCLabelBMFont::create(fmt::format("HP: {:.0f} [Lv {}]", calcMaxHp(), g_hpLvl).c_str(), "chatFont.fnt");
        hpInfo->setScale(0.45f); hpInfo->setColor(ccc3(100, 230, 100)); hpInfo->setPosition(ccp(cx, topY - 16.0f));
        upgNode->addChild(hpInfo, topZ(upgNode) + 1);
        int64_t hC = hpCost(g_hpLvl);
        ButtonSprite* hpBtnSpr = ButtonSprite::create(hC >= 0 ? fmt::format("Upgrade HP ${}", hC).c_str() : "MAXED", "chatFont.fnt", hC >= 0 ? "GJ_button_01.png" : "GJ_button_05.png", 0.5f);
        CCMenuItemSpriteExtra* uBtn = CCMenuItemSpriteExtra::create(hpBtnSpr, this, menu_selector(ShopLayer::onUpgradeHp));
        uBtn->setPosition(ccp(cx, topY - 40.0f)); shopMenu->addChild(uBtn, topZ(shopMenu) + 1);
        CCLabelBMFont* dmgInfo = CCLabelBMFont::create(fmt::format("x{:.1f} DMG [Lv {}]", calcDmgMulti(), g_dmgLvl).c_str(), "chatFont.fnt");
        dmgInfo->setScale(0.45f); dmgInfo->setColor(ccc3(255, 140, 80)); dmgInfo->setPosition(ccp(cx, topY - 74.0f));
        upgNode->addChild(dmgInfo, topZ(upgNode) + 1);
        int64_t dC = dmgCost(g_dmgLvl);
        ButtonSprite* dmgBtnSpr = ButtonSprite::create(dC >= 0 ? fmt::format("Upgrade DMG ${}", dC).c_str() : "MAXED", "chatFont.fnt", dC >= 0 ? "GJ_button_01.png" : "GJ_button_05.png", 0.5f);
        CCMenuItemSpriteExtra* dBtn = CCMenuItemSpriteExtra::create(dmgBtnSpr, this, menu_selector(ShopLayer::onUpgradeDmg));
        dBtn->setPosition(ccp(cx, topY - 98.0f)); shopMenu->addChild(dBtn, topZ(shopMenu) + 1);
        CCLabelBMFont* coinInfo = CCLabelBMFont::create(fmt::format("x{:.1f} Cash [Lv {}]", calcCoinMult(), g_coinLvl).c_str(), "chatFont.fnt");
        coinInfo->setScale(0.45f); coinInfo->setColor(ccc3(255, 215, 0)); coinInfo->setPosition(ccp(cx, topY - 132.0f));
        upgNode->addChild(coinInfo, topZ(upgNode) + 1);
        int64_t cC = coinCost(g_coinLvl);
        ButtonSprite* coinBtnSpr = ButtonSprite::create(cC >= 0 ? fmt::format("Greed ${}", cC).c_str() : "MAXED", "chatFont.fnt", cC >= 0 ? "GJ_button_01.png" : "GJ_button_05.png", 0.5f);
        CCMenuItemSpriteExtra* cBtn = CCMenuItemSpriteExtra::create(coinBtnSpr, this, menu_selector(ShopLayer::onUpgradeCoin));
        cBtn->setPosition(ccp(cx, topY - 156.0f)); shopMenu->addChild(cBtn, topZ(shopMenu) + 1);
    }

    void refreshCoins() {
        if (coinText) coinText->setString(fmt::format("Coins: ${}", g_coins).c_str());
    }

    void onBuy(CCObject* sender) {
        int idx = sender->getTag();
        if (g_coins < g_weaps[idx].cost) { FLAlertLayer::create("Broke", "Not enough coins", "OK")->show(); return; }
        g_coins -= g_weaps[idx].cost; g_weapUnlock[idx] = true; saveWeaps(); rebuildDynamicContent();
        if (buyCB) buyCB();
    }
};

inline void ShopLayer::rebuildDynamicContent() {
    float panelW = m_mainLayer->getContentSize().width;
    float panelH = m_mainLayer->getContentSize().height;
    CCMenu* shopMenu = typeinfo_cast<CCMenu*>(m_mainLayer->getChildByID("purchase-menu"));
    if (!shopMenu) return;
    shopMenu->removeAllChildren();
    buildWeapList(shopMenu, panelW, panelH);
    buildUpgSection(shopMenu, panelW, panelH);
    refreshCoins();
}

class SpikeUpgPopup : public Popup {
public:
    std::function<void()> closeCB;
    std::function<void()> buyCB;
    CCLabelBMFont* coinLbl = nullptr;
    CCLabelBMFont* aimInfo = nullptr;
    CCLabelBMFont* moreInfo = nullptr;
    CCLabelBMFont* rainInfo = nullptr;
    CCMenu* btnMenu = nullptr;

    static SpikeUpgPopup* create(std::function<void()> cb, std::function<void()> bcb) {
        auto ret = new SpikeUpgPopup();
        if (ret && ret->init(380.f, 300.f)) {
            ret->closeCB = cb;
            ret->buyCB = bcb;
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool init(float w, float h) {
        if (!Popup::init(w, h)) return false;
        this->setTitle("Spike Upgrades");
        float pw = m_mainLayer->getContentSize().width;
        float ph = m_mainLayer->getContentSize().height;
        coinLbl = CCLabelBMFont::create("", "chatFont.fnt");
        coinLbl->setScale(0.55f);
        coinLbl->setColor(ccc3(255, 215, 0));
        coinLbl->setPosition(ccp(pw * 0.5f, ph - 35.0f));
        m_mainLayer->addChild(coinLbl, topZ(m_mainLayer) + 1);
        btnMenu = CCMenu::create();
        btnMenu->setPosition(ccp(0.0f, 0.0f));
        m_mainLayer->addChild(btnMenu, topZ(m_mainLayer) + 1);
        rebuild();
        return true;
    }

    void rebuild() {
        if (!btnMenu) return;
        btnMenu->removeAllChildren();
        float pw = m_mainLayer->getContentSize().width;
        float ph = m_mainLayer->getContentSize().height;
        coinLbl->setString(fmt::format("Coins: ${}", g_coins).c_str());

        if (aimInfo) { aimInfo->removeFromParent(); aimInfo = nullptr; }
        if (moreInfo) { moreInfo->removeFromParent(); moreInfo = nullptr; }
        if (rainInfo) { rainInfo->removeFromParent(); rainInfo = nullptr; }

        float topY = ph - 65.0f;
        aimInfo = CCLabelBMFont::create(
            fmt::format("AimAssist [Lv {}/{}]: {:.0f}%", g_spikeAimLvl, SPIKE_AIM_MAX, spikeAimPct(g_spikeAimLvl) * 100.0f).c_str(),
            "chatFont.fnt"
        );
        aimInfo->setScale(0.5f);
        aimInfo->setColor(ccc3(220, 140, 255));
        aimInfo->setPosition(ccp(pw * 0.5f, topY));
        m_mainLayer->addChild(aimInfo, topZ(m_mainLayer) + 1);
        int64_t aC = spikeAimCost(g_spikeAimLvl);
        ButtonSprite* aimSpr = ButtonSprite::create(
            aC >= 0 ? fmt::format("Aim ${}", aC).c_str() : "MAXED",
            "chatFont.fnt",
            aC >= 0 ? "GJ_button_01.png" : "GJ_button_05.png",
            0.5f
        );
        CCMenuItemSpriteExtra* aimBtn = CCMenuItemSpriteExtra::create(aimSpr, this, menu_selector(SpikeUpgPopup::onAim));
        aimBtn->setPosition(ccp(pw * 0.5f, topY - 25.0f));
        btnMenu->addChild(aimBtn, topZ(btnMenu) + 1);

        float midY = topY - 60.0f;
        moreInfo = CCLabelBMFont::create(
            fmt::format("More Spikes [Lv {}/{}]: +{:.0f}%", g_spikeMoreLvl, SPIKE_MORE_MAX, (spikeRateMult(g_spikeMoreLvl) - 1.0f) * 100.0f).c_str(),
            "chatFont.fnt"
        );
        moreInfo->setScale(0.5f);
        moreInfo->setColor(ccc3(255, 140, 220));
        moreInfo->setPosition(ccp(pw * 0.5f, midY));
        m_mainLayer->addChild(moreInfo, topZ(m_mainLayer) + 1);
        int64_t mC = spikeMoreCost(g_spikeMoreLvl);
        ButtonSprite* moreSpr = ButtonSprite::create(
            mC >= 0 ? fmt::format("More ${}", mC).c_str() : "MAXED",
            "chatFont.fnt",
            mC >= 0 ? "GJ_button_01.png" : "GJ_button_05.png",
            0.5f
        );
        CCMenuItemSpriteExtra* moreBtn = CCMenuItemSpriteExtra::create(moreSpr, this, menu_selector(SpikeUpgPopup::onMore));
        moreBtn->setPosition(ccp(pw * 0.5f, midY - 22.0f));
        btnMenu->addChild(moreBtn, topZ(btnMenu) + 1);

        float botY = midY - 60.0f;
        rainInfo = CCLabelBMFont::create(
            fmt::format("Rain Time [Lv {}/{}]: {:.0f}s (+{:.0f}s)", g_spikeRainLvl, SPIKE_RAIN_MAX, 30.0f + spikeRainBonus(g_spikeRainLvl), spikeRainBonus(g_spikeRainLvl)).c_str(),
            "chatFont.fnt"
        );
        rainInfo->setScale(0.5f);
        rainInfo->setColor(ccc3(140, 200, 255));
        rainInfo->setPosition(ccp(pw * 0.5f, botY));
        m_mainLayer->addChild(rainInfo, topZ(m_mainLayer) + 1);
        int64_t rC = spikeRainCost(g_spikeRainLvl);
        ButtonSprite* rainSpr = ButtonSprite::create(
            rC >= 0 ? fmt::format("Rain ${}", rC).c_str() : "MAXED",
            "chatFont.fnt",
            rC >= 0 ? "GJ_button_01.png" : "GJ_button_05.png",
            0.5f
        );
        CCMenuItemSpriteExtra* rainBtn = CCMenuItemSpriteExtra::create(rainSpr, this, menu_selector(SpikeUpgPopup::onRain));
        rainBtn->setPosition(ccp(pw * 0.5f, botY - 22.0f));
        btnMenu->addChild(rainBtn, topZ(btnMenu) + 1);
    }

    void onAim(CCObject*) {
        int64_t cost = spikeAimCost(g_spikeAimLvl);
        if (cost < 0) return;
        if (g_coins < cost) { FLAlertLayer::create("Broke", "Not enough coins", "OK")->show(); return; }
        g_coins -= cost; g_spikeAimLvl++; saveWeaps(); rebuild();
        if (buyCB) buyCB();
    }

    void onMore(CCObject*) {
        int64_t cost = spikeMoreCost(g_spikeMoreLvl);
        if (cost < 0) return;
        if (g_coins < cost) { FLAlertLayer::create("Broke", "Not enough coins", "OK")->show(); return; }
        g_coins -= cost; g_spikeMoreLvl++; saveWeaps(); rebuild();
        if (buyCB) buyCB();
    }

    void onRain(CCObject*) {
        int64_t cost = spikeRainCost(g_spikeRainLvl);
        if (cost < 0) return;
        if (g_coins < cost) { FLAlertLayer::create("Broke", "Not enough coins", "OK")->show(); return; }
        g_coins -= cost; g_spikeRainLvl++; saveWeaps(); rebuild();
        if (buyCB) buyCB();
    }

    void onClose(CCObject* o) override {
        saveWeaps();
        if (closeCB) closeCB();
        Popup::onClose(o);
    }
};
