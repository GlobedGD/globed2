#include "changelog.hpp"

#include <util/format.hpp>

using namespace geode::prelude;

static const std::string LAST_CHANGELOG_KEY = "_last-changelog-seen";
static constexpr std::string LAST_CHANGELOG_KEY2 = "_last-changelog";

namespace globed {

bool shouldShowChangelogPopup() {
    return true;
    auto currentVersion = Mod::get()->getVersion();
    auto lastVerResult = VersionInfo::parse(Mod::get()->getSavedValue<std::string>(LAST_CHANGELOG_KEY));
    VersionInfo lastVer;
    if (lastVerResult) {
        lastVer = lastVerResult.unwrap();
    } else {
        return true;
    }

    if (lastVer < currentVersion) {
        return true;
    }

    return false;
}

namespace {
class BetterMDPopup : public MDPopup {
public:
    static BetterMDPopup* create(
        std::string const& title, std::string const& content, char const* btn1, char const* btn2,
        utils::MiniFunction<void(bool)> onClick
    ) {
        auto ret = new BetterMDPopup();
        if (ret->initAnchored(
                420.f, MDPopup::estimateHeight(content), title, content, btn1, btn2, onClick,
                "square01_001.png", { 0, 0, 94, 94 }
            )) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
}

FLAlertLayer* showChangelogPopup() {
    auto currentVersion = Mod::get()->getVersion();
    // parse changelog.md lol
    auto clog_ = Mod::get()->getMetadata().getChangelog();
    if (!clog_) {
        log::warn("Changelog is null");
        return nullptr;
    }

    auto clog = std::move(clog_.value());
    auto lines = util::format::splitlines(clog);

    std::string currentChangelog;

    bool insideCorrectVersion = false;

    for (auto line : lines) {
        if (line.starts_with("## ")) {
            auto linever = util::format::trim(line.substr(sizeof("## ") - 1));
            auto parsedr = VersionInfo::parse(linever);
            if (!parsedr) {
                continue;
            }


            if (parsedr.unwrap() == currentVersion) {
                insideCorrectVersion = true;
            } else if (insideCorrectVersion) {
                break;
            }
        } else if (insideCorrectVersion) {
            currentChangelog += line;
            currentChangelog += "\n";
        }
    }

    log::debug("Changelog text: {}, size: {}", currentChangelog, currentChangelog.size());

    if (!currentChangelog.empty()) {
        Mod::get()->setSavedValue(LAST_CHANGELOG_KEY, Mod::get()->getVersion().toVString());
        return BetterMDPopup::create(
            "Globed changelog",
            fmt::format("\n## {}\n\n{}", currentVersion.toVString(), currentChangelog),
            "Ok",
            nullptr, [](bool){}
        );
    }

    return nullptr;
}

}