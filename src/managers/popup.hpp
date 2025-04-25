#pragma once

#include <util/singleton.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
#include <queue>

#include <asp/data/Cow.hpp>
#include <asp/time/Duration.hpp>
#include <fmt/core.h>

class PopupManager : public SingletonNodeBase<PopupManager, true> {
    friend class SingletonNodeBase;
    friend class FLAlertLayerHook;

    PopupManager();

public:
    class PopupRef {
        friend class FLAlertLayerHook;
        friend class PopupManager;
        struct Data;

    public:
        PopupRef(FLAlertLayer* layer);
        PopupRef();

        operator FLAlertLayer*() const;
        FLAlertLayer* getInner() const;

        // Set whether the popup should follow the user if they transition to another scene,
        // instead of disappearing. By default is disabled.
        void setPersistent(bool state = true);

        // Makes it impossible to accidentally close the popup (via esc or back button)
        // for a given period of time after it is shown.
        void blockClosingFor(const asp::time::Duration& dur);

        // Makes it impossible to accidentally close the popup (via esc or back button)
        // for a given period of time (in ms) after it is shown.
        void blockClosingFor(int durMillis);

        // Shows the popup to the user immediately, does nothing if already shown.
        void showInstant();

        // Queues the popup to be shown to the user when it's appropriate,
        // e.g. not while they're in a level and unpaused or in the middle of a transition
        void showQueue();

        bool isShown();

    private:
        geode::Ref<FLAlertLayer> inner;

        Data& getFields();
        bool hasFields();
        void doShow(bool reshowing = false);
    };

    // Creates a popup with the given title and content (optionally button 1, 2 and width). Does not show the popup to the user.
    [[nodiscard]] PopupRef alert(
        const char* title,
        const std::string& content,
        const char* btn1 = "Ok",
        const char* btn2 = nullptr,
        float width = 360.f
    );

    // Creates a popup with the given title and content (optionally button 1, 2 and width). Does not show the popup to the user.
    [[nodiscard]] PopupRef alert(
        const std::string& title,
        const std::string& content,
        const char* btn1 = "Ok",
        const char* btn2 = nullptr,
        float width = 360.f
    );

    // Creates a popup with the given title and content (optionally button 1, 2 and width). Does not show the popup to the user.
    // The callback is involved when the user presses either of the buttons in the popup.
    [[nodiscard]] PopupRef quickPopup(
        const std::string& title,
        const std::string& content,
        const char* btn1 = "Ok",
        const char* btn2 = nullptr,
        std::function<void (FLAlertLayer*, bool)> callback = {},
        float width = 360.f
    );

    // Creates a popup with the given title and content (optionally button 1, 2 and width). Does not show the popup to the user.
    // The callback is involved when the user presses either of the buttons in the popup.
    [[nodiscard]] PopupRef quickPopup(
        const char* title,
        const std::string& content,
        const char* btn1 = "Ok",
        const char* btn2 = nullptr,
        std::function<void (FLAlertLayer*, bool)> callback = {},
        float width = 360.f
    );

    // Creates a popup with the given title and content as a formatted string. Does not show the popup to the user
    template <typename TitleT, class... Args>
    [[nodiscard]] PopupRef alertFormat(
        TitleT title,
        fmt::format_string<Args...> fmt,
        Args&&... args
    ) {
        return alert(title, fmt::format(fmt, std::forward<Args>(args)...));
    }

    // Create a PopupRef for this custom popup that can be used to manage it.
    // Don't call this if the popup has already been shown.
    [[nodiscard]] PopupRef manage(FLAlertLayer* alert);

    bool isManaged(FLAlertLayer* alert);

private:
    cocos2d::CCScene* m_prevScene = nullptr;
    bool m_isTransitioning = false;
    std::array<geode::Ref<FLAlertLayer>, 8> m_savedAlerts;
    size_t m_frameCounter = 0;
    size_t m_savedAlertCount;
    std::queue<PopupRef> m_queuedPopups;

    void changedScene(cocos2d::CCScene* newScene);
    void queuePopup(const PopupRef& popup);
    void update(float dt);
};