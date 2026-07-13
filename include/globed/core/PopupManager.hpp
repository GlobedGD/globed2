#pragma once

#include "../prelude.hpp"
#include "../util/singleton.hpp"
#include "../util/CStr.hpp"
#include <Geode/binding/FLAlertLayer.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/ui/PopupManager.hpp>

#include <deque>

class Label;


namespace globed {

using PopupRef = geode::ManagedPopup;

class GLOBED_DLL PopupManager : public SingletonNodeBase<PopupManager, true> {
    friend class SingletonNodeBase;
    PopupManager() = default;

public:
    constexpr static float DEFAULT_WIDTH = 370.f;

    // Creates a popup with the given title and content (optionally button 1, 2 and width). Does not show the popup to the user.
    PopupRef alert(
        CStr title,
        const std::string& content,
        CStr btn1 = "Ok",
        CStr btn2 = nullptr,
        float width = DEFAULT_WIDTH
    );

    // Creates a popup with the given title and content (optionally button 1, 2 and width). Does not show the popup to the user.
    // The callback is involved when the user presses either of the buttons in the popup.
    PopupRef quickPopup(
        CStr title,
        const std::string& content,
        CStr btn1 = "Ok",
        CStr btn2 = nullptr,
        geode::Function<void (FLAlertLayer*, bool)> callback = {},
        float width = DEFAULT_WIDTH
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

    // Returns whether there are currently any queued popups that can't be shown,
    // either because the player is transitioning or in a level and unpaused.
    bool hasPendingPopups() const;
};

/// Creates a popup with the given title and content (optionally button 1, 2 and width) and shows it to the user.
/// Shorthand for PopupManager::get().alert(args).showInstant()
inline void alert(
    CStr title,
    const std::string& content,
    CStr btn1 = "Ok",
    CStr btn2 = nullptr,
    float width = PopupManager::DEFAULT_WIDTH
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
    geode::Function<void (FLAlertLayer*, bool)> callback = {},
    float width = PopupManager::DEFAULT_WIDTH
) {
    PopupManager::get().quickPopup(
        title, content, btn1, btn2, std::move(callback), width
    ).showInstant();
}

/// Creates a confirmation popup and shows it to the user.
/// Shorthand for quickPopup(...), but with the callback having 1 argument and only being invoked when the user presses the second button.
inline void confirmPopup(
    CStr title,
    const std::string& content,
    CStr btn1 = "Cancel",
    CStr btn2 = "Ok",
    geode::Function<void (FLAlertLayer*)> callback = {},
    float width = PopupManager::DEFAULT_WIDTH
) {
    auto cb = [cb = std::move(callback)](FLAlertLayer* alert, bool btn2) mutable {
        if (btn2) cb(alert);
    };

    quickPopup(title, content, btn1, btn2, std::move(cb), width);
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

/// Shows a geode::Notification with a message.
GLOBED_DLL void toast(geode::NotificationIcon icon, float duration, const std::string& message);

/// Shows a geode::Notification with a message.
GLOBED_DLL void toast(cocos2d::CCSprite* icon, float duration, const std::string& message);

/// Shows a geode::Notification with a formatted message and a specified duration.
template <class... Args>
void toast(geode::NotificationIcon icon, float duration, fmt::format_string<Args...> fmt, Args&&... args) {
    return toast(icon, duration, fmt::format(fmt, std::forward<Args>(args)...));
}

/// Shows a geode::Notification with a formatted message.
template <class... Args>
void toast(geode::NotificationIcon icon, fmt::format_string<Args...> fmt, Args&&... args) {
    auto msg = fmt::format(fmt, std::forward<Args>(args)...);
    return toast(icon, 0.35f + msg.size() * 0.025f, msg);
}

/// Shows a geode::Notification with a formatted message and an error icon.
template <class... Args>
void toastError(fmt::format_string<Args...> fmt, Args&&... args) {
    return toast(geode::NotificationIcon::Error, fmt, std::forward<Args>(args)...);
}

/// Shows a geode::Notification with a formatted message and a success icon.
template <class... Args>
void toastSuccess(fmt::format_string<Args...> fmt, Args&&... args) {
    return toast(geode::NotificationIcon::Success, fmt, std::forward<Args>(args)...);
}

void colorizeLabel(Label* label, std::string_view text);

}
