#include "bg.hpp"

static bool isBandwidthDead(int code, std::string const& body) {
    if (code == 509) return true;
    std::string low = body;
    for (char& c : low) c = (char)std::tolower((unsigned char)c);
    if (low.find("bandwidth") != std::string::npos) return true;
    if (low.find("egress") != std::string::npos) return true;
    if (low.find("quota") != std::string::npos) return true;
    return false;
}

static void showBwDeadAlert() {
    FLAlertLayer::create(
        "RIP",
        "too many ppl downloaded and my bandwith is gone, join my server and report this pls",
        "OK"
    )->show();
}

BgPicker* BgPicker::create(std::function<void()> cb) {
    BgPicker* ret = new BgPicker();
    if (ret && ret->init(360.f, 300.f)) {
        ret->m_doneCB = cb;
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool BgPicker::init(float w, float h) {
    if (!Popup::init(w, h)) return false;
    this->setTitle("Backgrounds");
    float pw = m_mainLayer->getContentSize().width;
    float ph = m_mainLayer->getContentSize().height;

    m_stat = CCLabelBMFont::create("Loading...", "chatFont.fnt");
    m_stat->setScale(0.55f);
    m_stat->setPosition(ccp(pw / 2, ph / 2));
    m_mainLayer->addChild(m_stat, bgTopZ(m_mainLayer) + 1);

    CCMenu* botMenu = CCMenu::create();
    botMenu->setPosition(ccp(0, 0));
    ButtonSprite* noneSpr = ButtonSprite::create("None", "chatFont.fnt", "GJ_button_06.png", 0.55f);
    CCMenuItemSpriteExtra* noneBtn = CCMenuItemSpriteExtra::create(noneSpr, this, menu_selector(BgPicker::onClear));
    noneBtn->setPosition(ccp(pw / 2 - 55.f, 25.f));
    botMenu->addChild(noneBtn, bgTopZ(botMenu) + 1);

    ButtonSprite* impSpr = ButtonSprite::create("Import", "chatFont.fnt", "GJ_button_01.png", 0.55f);
    CCMenuItemSpriteExtra* impBtn = CCMenuItemSpriteExtra::create(impSpr, this, menu_selector(BgPicker::onImport));
    impBtn->setPosition(ccp(pw / 2 + 55.f, 25.f));
    botMenu->addChild(impBtn, bgTopZ(botMenu) + 1);

    m_mainLayer->addChild(botMenu, bgTopZ(m_mainLayer) + 1);

    fetchList();
    return true;
}

void BgPicker::onClose(CCObject* o) {
    Popup::onClose(o);
}

void BgPicker::onClear(CCObject*) {
    g_bgFile = "";
    Mod::get()->setSavedValue<std::string>("bg_file", "");
    if (m_doneCB) m_doneCB();
    this->onClose(nullptr);
}

void BgPicker::fetchList() {
    web::WebRequest req = web::WebRequest();
    req.header("apikey", std::string(SUPA_KEY));
    req.header("Authorization", std::string("Bearer ") + SUPA_KEY);
    req.header("Content-Type", "application/json");
    req.bodyString("{\"prefix\":\"\",\"limit\":100,\"offset\":0,\"sortBy\":{\"column\":\"name\",\"order\":\"asc\"}}");

    m_listReq.spawn(
        req.post(SUPA_LIST),
        [this](web::WebResponse res) {
            std::string raw = res.string().unwrapOr("[]");
            log::info("bg list status={} body={}", res.code(), raw);
            if (isBandwidthDead(res.code(), raw)) {
                if (m_stat) m_stat->setString("Bandwidth dead");
                showBwDeadAlert();
                return;
            }
            if (!res.ok()) {
                if (m_stat) m_stat->setString(fmt::format("HTTP {} fail", res.code()).c_str());
                return;
            }
            geode::Result<matjson::Value, matjson::ParseError> parsed = matjson::Value::parse(raw);
            if (!parsed.isOk()) {
                if (m_stat) m_stat->setString("Bad json lol");
                return;
            }
            matjson::Value json = parsed.unwrap();
            if (!json.isArray()) {
                if (m_stat) m_stat->setString("Not an array??");
                return;
            }
            m_fnames.clear();
            for (size_t i = 0; i < json.size(); i++) {
                matjson::Value const& item = json[i];
                if (item.isObject() && item.contains("name")) {
                    geode::Result<std::string> nameRes = item["name"].asString();
                    if (nameRes.isOk()) {
                        std::string name = nameRes.unwrap();
                        std::string lower = name;
                        for (char& c : lower) c = (char)std::tolower((unsigned char)c);
                        bool isImg = false;
                        if (lower.size() >= 4) {
                            std::string ext4 = lower.substr(lower.size() - 4);
                            std::string ext5 = lower.size() >= 5 ? lower.substr(lower.size() - 5) : "";
                            if (ext4 == ".png" || ext4 == ".jpg" || ext5 == ".webp" || ext5 == ".jpeg") isImg = true;
                        }
                        if (!name.empty() && isImg) m_fnames.push_back(name);
                    }
                }
            }
            if (m_stat) m_stat->setVisible(false);
            buildList();
        }
    );
}

void BgPicker::buildList() {
    float pw = m_mainLayer->getContentSize().width;
    float ph = m_mainLayer->getContentSize().height;

    float scrollW = pw - 30.f;
    float scrollH = ph - 80.f;
    int cols = 2;
    float cellW = scrollW / (float)cols;
    float cellH = 80.f;
    m_thumbW = cellW - 24.f;
    m_thumbH = 50.f;
    int rows = ((int)m_fnames.size() + cols - 1) / cols;
    if (rows < 1) rows = 1;
    float totalH = cellH * (float)rows;
    if (totalH < scrollH) totalH = scrollH;

    m_scroll = ScrollLayer::create(CCSizeMake(scrollW, scrollH));
    m_scroll->setPosition(ccp(15.f, 55.f));
    m_mainLayer->addChild(m_scroll, bgTopZ(m_mainLayer) + 1);

    CCMenu* menu = CCMenu::create();
    menu->setPosition(ccp(0, 0));
    m_scroll->m_contentLayer->addChild(menu, bgTopZ(m_scroll->m_contentLayer) + 1);

    m_thumbHolders.clear();
    m_thumbHolders.resize(m_fnames.size(), nullptr);

    for (int i = 0; i < (int)m_fnames.size(); i++) {
        int col = i % cols;
        int row = i / cols;
        float cellCX = cellW * (float)col + cellW / 2.f;
        float cellCY = totalH - cellH * (float)row - cellH / 2.f;
        std::string displayName = m_fnames[i];
        if (displayName.size() > 16) displayName = displayName.substr(0, 13) + "...";

        CCScale9Sprite* rowBg = CCScale9Sprite::create("GJ_button_01.png");
        rowBg->setContentSize(CCSizeMake(cellW - 16.f, cellH - 8.f));
        CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(rowBg, this, menu_selector(BgPicker::onPick));
        btn->setPosition(ccp(cellCX, cellCY));
        btn->setTag(i);
        menu->addChild(btn, bgTopZ(menu) + 1);

        CCLabelBMFont* nameLbl = CCLabelBMFont::create(displayName.c_str(), "chatFont.fnt");
        nameLbl->setScale(0.45f);
        nameLbl->setPosition(ccp(cellCX, cellCY + (cellH / 2.f) - 14.f));
        menu->addChild(nameLbl, bgTopZ(menu) + 1);

        CCNode* thumbHolder = CCNode::create();
        thumbHolder->setPosition(ccp(cellCX, cellCY - 10.f));
        m_thumbHolders[i] = thumbHolder;
        menu->addChild(thumbHolder, bgTopZ(menu) + 1);

        CCLabelBMFont* placeholder = CCLabelBMFont::create("...", "chatFont.fnt");
        placeholder->setScale(0.4f);
        placeholder->setColor(ccc3(180, 180, 180));
        placeholder->setID("thumb-placeholder");
        thumbHolder->addChild(placeholder, bgTopZ(thumbHolder) + 1);
    }

    m_scroll->m_contentLayer->setContentSize(CCSizeMake(scrollW, totalH));
    m_scroll->scrollToTop();
    handleTouchPriority(this);

    m_thumbIdx = 0;
    loadNextThumb();
}

void BgPicker::onPick(CCObject* sender) {
    int idx = sender->getTag();
    if (idx < 0 || idx >= (int)m_fnames.size()) return;
    std::string fname = m_fnames[idx];

    std::filesystem::path cached = bgCacheDir() / fname;
    std::error_code ec;
    if (std::filesystem::exists(cached, ec)) {
        g_bgFile = fname;
        Mod::get()->setSavedValue<std::string>("bg_file", fname);
        if (m_doneCB) m_doneCB();
        this->onClose(nullptr);
        return;
    }

    m_pendingPick = fname;
    dlImage(fname);
}

void BgPicker::populateThumb(int idx) {
    if (idx < 0 || idx >= (int)m_thumbHolders.size()) return;
    CCNode* holder = m_thumbHolders[idx];
    if (!holder) return;
    holder->removeAllChildren();
    CCSprite* spr = makeBgSprFromDisk(m_fnames[idx]);
    if (!spr) {
        CCLabelBMFont* failLbl = CCLabelBMFont::create("x_x", "chatFont.fnt");
        failLbl->setScale(0.4f);
        failLbl->setColor(ccc3(255, 80, 80));
        holder->addChild(failLbl, bgTopZ(holder) + 1);
        return;
    }
    CCSize sz = spr->getContentSize();
    float scX = m_thumbW / sz.width;
    float scY = m_thumbH / sz.height;
    float sc = scX < scY ? scX : scY;
    spr->setScale(sc);
    spr->setPosition(ccp(0, 0));
    holder->addChild(spr, bgTopZ(holder) + 1);
}

void BgPicker::loadNextThumb() {
    while (m_thumbIdx < (int)m_fnames.size()) {
        std::filesystem::path cached = bgCacheDir() / m_fnames[m_thumbIdx];
        std::error_code ec;
        if (std::filesystem::exists(cached, ec)) {
            populateThumb(m_thumbIdx);
            m_thumbIdx++;
            continue;
        }
        break;
    }
    if (m_thumbIdx >= (int)m_fnames.size()) return;

    int idx = m_thumbIdx;
    std::string fname = m_fnames[idx];
    std::string url = std::string(SUPA_PUB) + fname;
    web::WebRequest req = web::WebRequest();
    m_thumbReq.spawn(
        req.get(url),
        [this, idx, fname](web::WebResponse res) {
            std::string rawBody = res.string().unwrapOr("");
            if (isBandwidthDead(res.code(), rawBody)) {
                showBwDeadAlert();
                return;
            }
            if (res.ok()) {
                geode::ByteVector bytes = res.data();
                std::filesystem::path dir = bgCacheDir();
                std::error_code ec;
                std::filesystem::create_directories(dir, ec);
                std::filesystem::path fpath = dir / fname;
                geode::Result<> wr = file::writeBinary(fpath, bytes);
                if (wr.isOk()) populateThumb(idx);
            }
            m_thumbIdx++;
            loadNextThumb();
        }
    );
}

void BgPicker::dlImage(std::string const& fname) {
    std::string url = std::string(SUPA_PUB) + fname;
    web::WebRequest req = web::WebRequest();

    m_dlReq.spawn(
        req.get(url),
        [this, fname](web::WebResponse res) {
            std::string rawBody = res.string().unwrapOr("");
            if (isBandwidthDead(res.code(), rawBody)) {
                if (m_stat) m_stat->setString("Bandwidth dead");
                showBwDeadAlert();
                return;
            }
            if (!res.ok()) {
                if (m_stat) m_stat->setString("Download failed");
                return;
            }
            geode::ByteVector bytes = res.data();
            std::filesystem::path dir = bgCacheDir();
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            std::filesystem::path fpath = dir / fname;
            geode::Result<> writeRes = file::writeBinary(fpath, bytes);
            if (!writeRes.isOk()) {
                if (m_stat) m_stat->setString("Save failed bruh");
                return;
            }
            g_bgFile = fname;
            Mod::get()->setSavedValue<std::string>("bg_file", fname);
            if (m_doneCB) m_doneCB();
            this->onClose(nullptr);
        }
    );
}

void BgPicker::onImport(CCObject*) {
    file::FilePickOptions::Filter filt;
    filt.description = "Images";
    filt.files = { "*.png", "*.jpg", "*.jpeg", "*.webp" };
    file::FilePickOptions opts;
    opts.filters = { filt };

    m_pickReq.spawn(
        file::pick(file::PickMode::OpenFile, opts),
        [this](file::PickResult res) {
            if (!res.isOk()) {
                FLAlertLayer::create("RIP", "Pick failed", "OK")->show();
                return;
            }
            std::optional<std::filesystem::path> maybe = res.unwrap();
            if (!maybe.has_value()) return;
            std::filesystem::path srcPath = maybe.value();
            geode::Result<geode::ByteVector> readRes = file::readBinary(srcPath);
            if (!readRes.isOk()) {
                FLAlertLayer::create("RIP", "Couldnt read that file", "OK")->show();
                return;
            }
            std::string fname = geode::utils::string::pathToString(srcPath.filename());
            if (fname.empty()) fname = "imported.png";
            std::filesystem::path dstDir = bgCacheDir();
            std::error_code ec;
            std::filesystem::create_directories(dstDir, ec);
            std::filesystem::path dst = dstDir / fname;
            geode::Result<> writeRes = file::writeBinary(dst, readRes.unwrap());
            if (!writeRes.isOk()) {
                FLAlertLayer::create("RIP", "Save failed lmao", "OK")->show();
                return;
            }
            CCSprite* test = makeBgSprFromDisk(fname);
            if (!test) {
                FLAlertLayer::create("RIP", "That image format aint supported", "OK")->show();
                std::filesystem::remove(dst, ec);
                return;
            }
            g_bgFile = fname;
            Mod::get()->setSavedValue<std::string>("bg_file", fname);
            if (m_doneCB) m_doneCB();
            this->onClose(nullptr);
        }
    );
}
