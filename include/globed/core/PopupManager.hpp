#pragma once

#include <globed/util/singleton.hpp>
#include <globed/util/CStr.hpp>
#include <Geode/Geode.hpp>

#include <queue>

namespace asp::time {
    class Duration; // to prevent leaking asp headers into the public interface
}

namespace globed {

class [[nodiscard]] PopupRef {
    friend class PopupManager;
    struct Data;

public:
    PopupRef(FLAlertLayer* layer);
    PopupRef();

    PopupRef(const PopupRef& other) = default;
    PopupRef& operator=(const PopupRef& other) = default;
    PopupRef(PopupRef&& other) noexcept = default;
    PopupRef& operator=(PopupRef&& other) noexcept = default;

    operator FLAlertLayer*() const;
    FLAlertLayer* operator->() const;
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

    bool shouldPreventClosing();

private:
    geode::Ref<FLAlertLayer> inner;

    Data& getFields();
    bool hasFields() const;
    void doShow(bool reshowing = false);
};

class PopupManager : public SingletonNodeBase<PopupManager, true> {
    friend class SingletonNodeBase;
    friend class PopupRef;
    PopupManager();

public:
    // Creates a popup with the given title and content (optionally button 1, 2 and width). Does not show the popup to the user.
    PopupRef alert(
        CStr title,
        const std::string& content,
        CStr btn1 = "Ok",
        CStr btn2 = nullptr,
        float width = 360.f
    );

    // Creates a popup with the given title and content (optionally button 1, 2 and width). Does not show the popup to the user.
    // The callback is involved when the user presses either of the buttons in the popup.
    PopupRef quickPopup(
        CStr title,
        const std::string& content,
        CStr btn1 = "Ok",
        CStr btn2 = nullptr,
        std::function<void (FLAlertLayer*, bool)> callback = {},
        float width = 360.f
    );

    // Creates a popup with the given title and content as a formatted string. Does not show the popup to the user
    template <class... Args>
    PopupRef alertFormat(
        CStr title,
        fmt::format_string<Args...> fmt,
        Args&&... args
    ) {
        return alert(title, fmt::format(fmt, std::forward<Args>(args)...));
    }

    // Create a PopupRef for this custom popup that can be used to manage it.
    // Don't call this if the popup has already been shown.
    PopupRef manage(FLAlertLayer* alert);

    bool isManaged(FLAlertLayer* alert);
private:
    cocos2d::CCScene* m_prevScene = nullptr;
    bool m_isTransitioning = false;
    std::array<geode::Ref<FLAlertLayer>, 16> m_savedAlerts;
    size_t m_frameCounter = 0;
    size_t m_savedAlertCount;
    std::queue<PopupRef> m_queuedPopups;

    void changedScene(cocos2d::CCScene* newScene);
    void queuePopup(const PopupRef& popup);
    void update(float dt);
};

/// Creates a popup with the given title and content (optionally button 1, 2 and width) and shows it to the user.
/// Shorthand for PopupManager::get().alert(args).showInstant()
inline void alert(
    CStr title,
    const std::string& content,
    CStr btn1 = "Ok",
    CStr btn2 = nullptr,
    float width = 360.f
) {
    PopupManager::get().alert(title, content, btn1, btn2, width).showInstant();
}

/// Creates a popup with the given title and content (optionally button 1, 2 and width) and shows it to the user.
/// The callback is involved when the user presses either of the buttons in the popup.
/// Shorthand for PopupManager::get().quickPopup(args).showInstant()
inline void quickPopup(
    CStr title,
    const std::string& content,
    CStr btn1 = "Ok",
    CStr btn2 = nullptr,
    std::function<void (FLAlertLayer*, bool)> callback = {},
    float width = 360.f
) {
    PopupManager::get().quickPopup(
        title, content, btn1, btn2, std::move(callback), width
    ).showInstant();
}

/// Creates a popup with the given title and content as a formatted string and shows it to the user.
/// Shorthand for PopupManager::get().alertFormat(args).showInstant()
template <class... Args>
void alertFormat(
    CStr title,
    fmt::format_string<Args...> fmt,
    Args&&... args
) {
    PopupManager::get().alertFormat(title, fmt, std::forward<Args>(args)...).showInstant();
}

}
