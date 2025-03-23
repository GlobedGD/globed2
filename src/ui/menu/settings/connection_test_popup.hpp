#pragma once

#include <defs/geode.hpp>
#include <ui/general/progressbar.hpp>
#include <ui/general/list/list.hpp>
#include <ui/general/loading_circle.hpp>
#include <ui/general/intermediary_loading_popup.hpp>

#include <functional>
#include <asp/thread/Thread.hpp>
#include <asp/sync/Channel.hpp>
#include <asp/time/Instant.hpp>
#include <asp/time/Duration.hpp>

class ConnectionTestPopup : public geode::Popup<> {
public:
    struct Test;

    static constexpr float POPUP_WIDTH = 340.f;
    static constexpr float POPUP_HEIGHT = 240.f;
    static constexpr float LIST_WIDTH= 300.f;

    static ConnectionTestPopup* create();

    bool setup() override;
    void onClose(CCObject*) override;
    void setProgress(double p);
    void queueGameServerTests();
    void softReloadTests();
    void update(float dt) override;

    class StatusCell : public cocos2d::CCMenu {
    public:
        static StatusCell* create(const char* name, float width);
        void softReload(const Test& test);

    protected:
        cocos2d::CCLabelBMFont* qmark;
        cocos2d::CCSprite* xIcon;
        cocos2d::CCSprite* checkIcon;
        BetterLoadingCircle* loadingCircle;

        cocos2d::CCLabelBMFont* nameLabel;
        cocos2d::CCLabelBMFont* stateLabel;
        CCMenuItemSpriteExtra* showFailReasonBtn;
        std::string failReason;

        bool init(const char* name, float width);
        void setUnknown();
        void setRunning();
        void setFailed(std::string failReason);
        void setBlocked(std::string_view depTest);
        void setSuccessful();

        enum class Icon {
            Unknown, Running, Failed, Blocked, Successful
        };

        void switchIcon(Icon icon);
    };

protected:
    ProgressBar* progressBar;
    asp::Thread<> workThread;
    bool reallyClose = false;
    bool actuallyReallyClose = false;
    bool waitingForThreadTerm = false;
    asp::AtomicBool threadTerminated = false;
    IntermediaryLoadingPopup* loadingPopup = nullptr;

    // Range of tests to perform:
    // 1. Google TCP Test: Open TCP socket to 8.8.8.8
    // 2. Google HTTP Test: Fetch https://google.com (HEAD request)
    // 3. DNS Test: Perform DNS lookup for the main server url
    // 4. Central Test: Fetch the index page of the main server
    // 5. Server List Test: Fetch the servers page of the main server
    // Then for each server:
    // * Perform DNS resolution of the address, if applicable
    // * Try connecting to the server with TCP
    // * Try sending some dummy packet and see if the server responds properly
    // * Send a UDP ping and see if there is a response

public:
    struct Test {
        ConnectionTestPopup* ctPopup;
        StatusCell* cell;
        std::string name;
        std::shared_ptr<Test> dependsOn;
        std::function<void(Test*)> runFunc;
        asp::AtomicBool finished = false;
        asp::AtomicBool failed = false;
        asp::AtomicBool blocked = false;
        asp::AtomicBool running = false;
        std::string failReason;

        Test(ConnectionTestPopup* ctPopup,
            StatusCell* cell,
            std::string name,
            std::shared_ptr<Test> dependsOn,
            std::function<void(Test*)> runFunc
        ) : ctPopup(ctPopup), cell(cell), name(name), dependsOn(dependsOn), runFunc(runFunc) {}

        void finish();
        void fail(std::string message);
        void block();
        void reloadPopup();

        void logTrace(std::string_view message);
        void logInfo(std::string_view message);
        void logWarn(std::string_view message);
        void logError(std::string_view message);
    };

    class LogMessageCell : public CCNode {
    public:
        double timestamp;

        static LogMessageCell* create(const std::string& message, cocos2d::ccColor3B color, double timestamp, float width);

    protected:
        bool init(const std::string& message, cocos2d::ccColor3B color, double timestamp, float width);
    };

protected:
    std::vector<std::shared_ptr<Test>> tests;
    asp::Channel<std::shared_ptr<Test>> threadTestQueue;
    GlobedListLayer<StatusCell>* list;
    GlobedListLayer<LogMessageCell>* logList;
    asp::time::Instant startedTestingAt;

    std::shared_ptr<Test> addTest(
        std::string name,
        std::function<void(Test*)> runFunc,
        std::shared_ptr<Test> dependsOn = nullptr
    );

    void appendLog(std::string msg, cocos2d::ccColor3B color);
    void appendLogAsync(std::string msg, cocos2d::ccColor3B color);
};
