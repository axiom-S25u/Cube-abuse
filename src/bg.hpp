#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/utils/web.hpp>
#include <filesystem>

using namespace geode::prelude;

inline const char* GH_API_LIST = "https://api.github.com/repos/axiom-S25u/Cube-Abuse-Bg/contents";
inline const char* GH_RAW = "https://raw.githubusercontent.com/axiom-S25u/Cube-Abuse-Bg/main/";
inline std::string g_bgFile = "";

static int bgTopZ(CCNode* p) {
    int m = 0;
    if (p) {
        for (CCNode* node : p->getChildrenExt()) {
            if (node->getZOrder() > m) m = node->getZOrder();
        }
    }
    return m;
}

static std::filesystem::path bgCacheDir() {
    return Mod::get()->getSaveDir() / "bg_cache";
}

static CCSprite* makeBgSprFromDisk(std::string const& fname) {
    std::filesystem::path p = bgCacheDir() / fname;
    geode::Result<geode::ByteVector> res = file::readBinary(p);
    if (!res.isOk()) return nullptr;
    geode::ByteVector bytes = res.unwrap();
    CCImage* img = new CCImage();
    if (!img->initWithImageData((void*)bytes.data(), (int)bytes.size())) {
        img->release();
        return nullptr;
    }
    CCTexture2D* tex = new CCTexture2D();
    tex->initWithImage(img);
    img->release();
    CCSprite* spr = CCSprite::createWithTexture(tex);
    tex->release(); 
    return spr;
}

static void loadBgToLayer(CCLayer* layer, int z) {
    if (g_bgFile.empty()) return;
    CCNode* old = layer->getChildByID("custom-bg-spr");
    if (old) old->removeFromParent();
    CCSprite* spr = makeBgSprFromDisk(g_bgFile);
    if (!spr) return;
    CCSize ws = CCDirector::sharedDirector()->getWinSize();
    spr->setPosition(ccp(ws.width / 2, ws.height / 2));
    float scX = ws.width / spr->getContentSize().width;
    float scY = ws.height / spr->getContentSize().height;
    float sc = (scX > scY) ? scX : scY;
    spr->setScale(sc);
    spr->setID("custom-bg-spr");
    layer->addChild(spr, z);
}

static void removeBgFromLayer(CCLayer* layer) {
    CCNode* old = layer->getChildByID("custom-bg-spr");
    if (old) old->removeFromParent();
}

class BgPicker : public Popup {
public:
    std::function<void()> m_doneCB;
    CCLabelBMFont* m_stat = nullptr;
    ScrollLayer* m_scroll = nullptr;
    std::vector<std::string> m_fnames;
    std::vector<CCNode*> m_thumbHolders;
    int m_thumbIdx = 0;
    bool m_pickerBusy = false;
    std::string m_pendingPick = "";
    TaskHolder<web::WebResponse> m_listReq;
    TaskHolder<web::WebResponse> m_dlReq;
    TaskHolder<web::WebResponse> m_thumbReq;
    TaskHolder<file::PickResult> m_pickReq;
    float m_thumbW = 0.f;
    float m_thumbH = 0.f;

    static BgPicker* create(std::function<void()> cb);
    bool init(float w, float h);
    void onClose(CCObject*) override;
    void fetchList();
    void buildList();
    void onPick(CCObject*);
    void onClear(CCObject*);
    void onImport(CCObject*);
    void dlImage(std::string const& fname);
    void populateThumb(int idx);
    void loadNextThumb();
};
