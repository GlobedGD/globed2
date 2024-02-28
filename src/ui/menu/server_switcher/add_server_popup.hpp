#pragma once
#include <defs/all.hpp>

class ServerSwitcherPopup;

// Note that this class is used for both adding new servers, and modifying existing.
// the actual behavior depends on whether `modifyingIndex` is set to -1 or an actual server index.
class AddServerPopup : public geode::Popup<int, ServerSwitcherPopup*> {
public:
    constexpr static float POPUP_WIDTH = 420.f;
    constexpr static float POPUP_HEIGHT = 280.f;

    static AddServerPopup* create(int modifyingIndex, ServerSwitcherPopup* parent);

    void onTestSuccess();
    void onTestFailure(const std::string_view message);

protected:
    geode::InputNode *nameNode, *urlNode;
    ServerSwitcherPopup* parent;
    int modifyingIndex = -1;

    std::string testedName, testedUrl;

    bool setup(int modifyingIndex, ServerSwitcherPopup* parent) override;
};