#pragma once

#include <globed/prelude.hpp>

#include <AdvancedLabel.hpp>
#include <Geode/Geode.hpp>

namespace globed {

class PingOverlay : public CCNode {
public:
    static PingOverlay *create();

    void updatePing(uint32_t ms);
    void updateWithDisconnected();
    void updateWithEditor();

    void addToLayer(CCNode *parent);
    void reloadFromSettings();

private:
    Label *m_pingLabel = nullptr;
    Label *m_versionLabel = nullptr;
    bool m_enabled, m_conditional;

    bool init() override;
    void updateOpacity();
};

} // namespace globed
