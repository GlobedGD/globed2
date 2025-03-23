#include "connection_test_popup.hpp"

#include <data/packets/client/connection.hpp>
#include <data/packets/server/connection.hpp>
#include <managers/error_queues.hpp>
#include <managers/web.hpp>
#include <managers/game_server.hpp>
#include <net/manager.hpp>
#include <net/tcp_socket.hpp>
#include <net/game_socket.hpp>
#include <net/address.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;
using namespace asp::time;
using namespace util::rng;
using ConnectionState = NetworkManager::ConnectionState;
using StatusCell = ConnectionTestPopup::StatusCell;
using LogMessageCell = ConnectionTestPopup::LogMessageCell;
using Test = ConnectionTestPopup::Test;

bool ConnectionTestPopup::setup() {
    auto& nm = NetworkManager::get();
    if (nm.getConnectionState() != ConnectionState::Disconnected) {
        nm.disconnect();
    }

    this->setTitle("Connection test");
    this->scheduleUpdate();

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    // make the test list
    Build(GlobedListLayer<StatusCell>::createForComments(LIST_WIDTH, POPUP_HEIGHT * 0.7f))
        .parent(m_mainLayer)
        .pos(rlayout.fromCenter(0.f, -10.f))
        .store(list);

    // make the log list
    Build(GlobedListLayer<LogMessageCell>::createForComments(LIST_WIDTH, POPUP_HEIGHT * 0.7f))
        .visible(false)
        .parent(m_mainLayer)
        .pos(rlayout.fromCenter(0.f, -10.f))
        .store(logList);

    logList->setCellColor(globed::color::DarkBrown);

    Build(CCMenuItemExt::createTogglerWithFrameName("button-privacy-lists-on.png"_spr, "button-privacy-lists-off.png"_spr, 0.7f, [this](auto toggler) {
        bool on = !toggler->isOn();

        this->list->setVisible(!on);
        this->logList->setVisible(on);
    }))
        .pos(rlayout.fromTopRight(32.f, 20.f))
        .parent(m_buttonMenu);

    // add the tests

    this->addTest("Google TCP Test", [](Test* test) {
        test->logTrace("Creating TCP socket");

        TcpSocket sock;

        test->logInfo("Connecting to 8.8.8.8:53");

        auto result = sock.connect(NetworkAddress{"8.8.8.8", 53});

        if (result) {
            test->logInfo("Connected successfully!");
            sock.disconnect();
            test->finish();
        } else {
            test->fail(fmt::format("Connection to 8.8.8.8:53 failed: {}", result.unwrapErr()));
        }
    });

    this->addTest("Google HTTP Test", [](Test* test) {
        test->logInfo("Sending a HEAD request to https://google.com");

        auto task = WebRequestManager::get().testGoogle();

        while (task.isPending()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }

        auto val = task.getFinishedValue();
        if (!val) {
            test->fail("Task returned no value");
            return;
        }

        if (!val->ok()) {
            test->fail(fmt::format("Connection to google.com failed: {}", val->getError()));
            return;
        }

        test->logInfo("Request was successful!");
        test->finish();
    });

    // this string is compile-time, so this is ok
    std::string_view srvdomain = globed::string<"main-server-url">();
    srvdomain.remove_prefix(sizeof("https://") - 1);

    while (srvdomain.ends_with('/')) {
        srvdomain.remove_suffix(1);
    }

    this->addTest("DNS Test", [srvdomain](Test* test) {
        test->logInfo(fmt::format("Attempting to resolve host \"{}\"", srvdomain));

        NetworkAddress addr(srvdomain, 443);
        auto res = addr.resolveToString();

        if (res) {
            test->logInfo(fmt::format("Resolved to {}", res.unwrap()));
            test->finish();
        } else {
            test->fail(fmt::format("Failed to resolve {}: {}", srvdomain, res.unwrapErr()));
        }
    });

    auto centralTest = this->addTest("Central Test", [](Test* test) {
        std::string_view url = globed::string<"main-server-url">();

        test->logInfo(fmt::format("Attempting to fetch {}/versioncheck", url));

        auto task = WebRequestManager::get().testServer(url);

        while (task.isPending()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }

        auto val = task.getFinishedValue();
        if (!val) {
            test->fail("Task returned no value");
            return;
        }

        if (!val->ok()) {
            if (val->getCode() == 400) {
                test->logWarn("Received a 400, this could likely be an outdated client and not a network issue!");
            }

            test->fail(fmt::format("Request to {}/versioncheck failed: {}", url, val->getError()));
            return;
        }

        test->finish();
    });

    auto srvListTest = this->addTest("Server List Test", [](Test* test) {
        std::string_view url = globed::string<"main-server-url">();

        test->logInfo(fmt::format("Attempting to fetch {}/servers", url));

        auto task = WebRequestManager::get().fetchServers(url);

        while (task.isPending()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }

        auto val = task.getFinishedValue();
        if (!val) {
            test->fail("Task returned no value");
            return;
        }

        if (!val->ok()) {
            test->fail(fmt::format("Request to {}/servers failed: {}", url, val->getError()));
            return;
        }

        auto resp = val->text().unwrapOrDefault();
        std::string truncResp;
        if (resp.size() > 256) {
            truncResp = resp.substr(0, 256);
            truncResp += "... (truncated)";
        } else {
            truncResp = resp;
        }

        auto& gsm = GameServerManager::get();
        gsm.clear();
        gsm.clearActive();
        gsm.updateCache(resp);
        auto loadResult = gsm.loadFromCache();
        gsm.pendingChanges = true;

        if (!loadResult) {
            test->logWarn(fmt::format("Failed to parse server list: {}", loadResult.unwrapErr()));
            test->logWarn(fmt::format("Server response: \"{}\"", truncResp));
            test->fail("Invalid response was sent by the server");
            return;
        }

        Loader::get()->queueInMainThread([popup = test->ctPopup] {
            popup->queueGameServerTests();
        });

        test->finish();
    }, centralTest);

    workThread.setStartFunction([] {
        geode::utils::thread::setName("Connection Test Thread");
    });
    workThread.setExceptionFunction([](auto err) {
        ErrorQueues::get().error(fmt::format("Connection test thread threw an exception: {}", err.what()));
    });
    workThread.setTerminationFunction([this] {
        this->threadTerminated = true;
    });
    workThread.setLoopFunction([this](auto& stopToken) {
        threadTestQueue.waitForMessages(asp::time::Duration::fromMillis(100));

        auto msg = threadTestQueue.tryPop();
        if (!msg) return;

        auto& test = msg.value();

        if (auto& dep = test->dependsOn) {
            if (!dep->finished) {
                log::error("Internal error: attempting to run test \"{}\" which depends on \"{}\", which has not been done yet", test->name, dep->name);
                return;
            }

            // if it depends on another test and that one failed or was blocked, block this one from running
            if (dep->failed || dep->blocked) {
                test->block();
                return;
            }
        }

        test->running = true;
        test->reloadPopup(); // to show the Running label

        // run the test
        test->runFunc(test.get());

        test->running = false;
    });

    this->startedTestingAt = Instant::now();

    workThread.start();

    return true;
}

std::shared_ptr<ConnectionTestPopup::Test> ConnectionTestPopup::addTest(
    std::string name,
    std::function<void(Test*)> runFunc,
    std::shared_ptr<Test> dependsOn
) {
    auto cell = StatusCell::create(name.c_str(), LIST_WIDTH);
    this->list->addCell(cell);

    auto sptr = tests.emplace_back(std::make_shared<Test>(
        this, cell, std::move(name), std::move(dependsOn), std::move(runFunc)
    ));

    this->threadTestQueue.push(sptr);

    return sptr;
}

void ConnectionTestPopup::queueGameServerTests() {
    auto& gsm = GameServerManager::get();
    auto servers = gsm.getAllServers();

    for (const auto& [serverId, server] : servers) {
        this->addTest(server.name, [server](Test* test) {
            GameSocket sock;

            NetworkAddress addr(server.address);
            auto resolvRes = addr.resolveToString();

            if (!resolvRes) {
                test->fail(fmt::format("Failed to resolve host ({}): {}", server.address, resolvRes.unwrapErr()));
                return;
            }

            test->logInfo(fmt::format("Connecting to {} (resolved to {})", server.address, resolvRes.unwrapOrDefault()));

            auto res = sock.connect(addr, false);

            if (!res) {
                test->fail(fmt::format("Socket connect failed: {}", res.unwrapErr()));
                return;
            }

            auto pingId = Random::get().generate<uint32_t>();

            test->logTrace(fmt::format("Connected successfully, sending a ping packet, id: {}", pingId));

            // send a TCP ping packet
            res = sock.sendPacketTCP(PingPacket::create(pingId));
            if (!res) {
                test->fail(fmt::format("Failed to send (TCP) ping packet: {}", res.unwrapErr()));
                return;
            }

            // wait up to 5s for response
            while (true) {
                auto pollRes = sock.poll(5000);
                if (!pollRes) {
                    test->fail(fmt::format("Failed to poll socket: {}", res.unwrapErr()));
                    return;
                }

                auto r = pollRes.unwrap();
                if (r == GameSocket::PollResult::Udp) {
                    (void) sock.recvPacketUDP(); // drop it
                    continue;
                } else if (r == GameSocket::PollResult::None) {
                    test->fail("No ping response received after 5 seconds");
                    return;
                } else {
                    // tcp packet was received!
                    auto pktres = sock.recvPacketTCP();
                    if (!pktres) {
                        test->fail(fmt::format("Failed to decode ping response: {}", pktres.unwrapErr()));
                        return;
                    }

                    auto pkt = pktres.unwrap();
                    if (!pkt) {
                        test->fail(fmt::format("Failed to decode ping response (null packet)"));
                        return;
                    }

                    auto pongpkt = pkt->tryDowncast<PingResponsePacket>();
                    if (!pongpkt) {
                        test->fail(fmt::format("Unexpected packet, expected PingResponsePacket, got packet ID {}", pkt->getPacketId()));
                        return;
                    }

                    test->logTrace(fmt::format("Received ping response, id: {}, player count: {}", pongpkt->id, pongpkt->playerCount));

                    if (pongpkt->id != pingId) {
                        test->fail("Server responded unexpected ping ID");
                        return;
                    }

                    break;
                }
            }

            // send a UDP ping packet
            pingId += 1;
            res = sock.sendPacketUDP(PingPacket::create(pingId));

            test->logTrace(fmt::format("Sending a UDP ping, id: {}", pingId));

            if (!res) {
                test->fail(fmt::format("Failed to send (UDP) ping packet: {}", res.unwrapErr()));
                return;
            }

            // wait up to 5s for response
            while (true) {
                auto pollRes = sock.poll(5000);
                if (!pollRes) {
                    test->fail(fmt::format("Failed to poll socket: {}", res.unwrapErr()));
                    return;
                }

                auto r = pollRes.unwrap();
                if (r == GameSocket::PollResult::Tcp) {
                    (void) sock.recvPacketTCP(); // drop it
                    continue;
                } else if (r == GameSocket::PollResult::None) {
                    test->fail("No UDP ping response received after 5 seconds");
                    return;
                } else {
                    // udp packet was received!
                    auto pktres = sock.recvPacketUDP(true);
                    if (!pktres) {
                        test->fail(fmt::format("Failed to decode ping response: {}", pktres.unwrapErr()));
                        return;
                    }

                    auto pkt = pktres.unwrap()->packet;
                    if (!pkt) {
                        test->fail(fmt::format("Failed to decode ping response (null packet)"));
                        return;
                    }

                    auto pongpkt = pkt->tryDowncast<PingResponsePacket>();
                    if (!pongpkt) {
                        test->fail(fmt::format("Unexpected packet, expected PingResponsePacket, got packet ID {}", pkt->getPacketId()));
                        return;
                    }

                    test->logTrace(fmt::format("Received UDP ping response, id: {}, player count: {}", pongpkt->id, pongpkt->playerCount));

                    if (pongpkt->id != pingId) {
                        test->fail("Server responded unexpected ping ID");
                        return;
                    }

                    break;
                }
            }

            test->logTrace("Cleaning up");

            // disconnect
            sock.disconnect();

            test->finish();
        });
    }
}

void ConnectionTestPopup::softReloadTests() {
    bool anyUnfinished = false;

    for (auto test : this->tests) {
        test->cell->softReload(*test);

        if (!test->finished) {
            anyUnfinished = true;
        }
    }

    // if all tests finished, stop thread and let the user exit
    if (!anyUnfinished && !reallyClose) {
        workThread.stop();
        reallyClose = true; // so the user does not have to look at the popup
        this->appendLog("All tests finished.", ccColor3B{ 47, 255, 64 });
    }
}

void ConnectionTestPopup::appendLog(std::string msg, ccColor3B color) {
    Duration dur = startedTestingAt.elapsed();
    msg = fmt::format("{}.{:03} {}", dur.seconds(), dur.subsecMillis(), msg);
    double ts = static_cast<double>(dur.micros()) / (1000.0 * 1000.0);

    logList->addCell(msg, color, ts, LIST_WIDTH);
}

void ConnectionTestPopup::appendLogAsync(std::string msg, ccColor3B color) {
    Duration dur = startedTestingAt.elapsed();
    msg = fmt::format("{}.{:03} {}", dur.seconds(), dur.subsecMillis(), msg);
    double ts = static_cast<double>(dur.micros()) / (1000.0 * 1000.0);

    Loader::get()->queueInMainThread([this, ts, msg = std::move(msg), color] {
        logList->addCell(msg, color, ts, LIST_WIDTH);
        logList->sort([](LogMessageCell* a, LogMessageCell* b) {
            return a->timestamp < b->timestamp;
        });
    });
}

void ConnectionTestPopup::update(float dt) {
    if (waitingForThreadTerm && threadTerminated) {
        if (this->loadingPopup) {
            this->loadingPopup->forceClose();
            this->loadingPopup = nullptr;
        }

        actuallyReallyClose = true;
        this->onClose(this);

        return;
    }
}

void ConnectionTestPopup::onClose(cocos2d::CCObject* o) {
    if (reallyClose) {
        if (actuallyReallyClose || threadTerminated) {
            Popup::onClose(o);
            return;
        }

        // stop the thread
        workThread.stop();
        waitingForThreadTerm = true;

        this->loadingPopup = IntermediaryLoadingPopup::create([](auto self) {
            self->disableClosing();
        }, [](auto self) {});
        this->loadingPopup->show();

        return;
    }

    geode::createQuickPopup(
        "Note",
        "Are you sure you want to interrupt the network test?",
        "Cancel",
        "Yes",
        [this](auto, bool yeah) {
            if (!yeah) return;

            this->reallyClose = true;
            this->onClose(this);
        }
    );
}

ConnectionTestPopup* ConnectionTestPopup::create() {
    auto ret = new ConnectionTestPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

/* Test */

void Test::finish() {
    this->finished = true;
    this->reloadPopup();
}

void Test::fail(std::string message) {
    this->finished = true;
    this->failed = true;
    this->logError(message);
    this->failReason = std::move(message);
    this->reloadPopup();
}

void Test::block() {
    this->finished = true;
    this->blocked = true;
    this->reloadPopup();
}

void Test::reloadPopup() {
    Loader::get()->queueInMainThread([popup = this->ctPopup] {
        popup->softReloadTests();
    });
}

void Test::logTrace(std::string_view message) {
    log::debug("[{}] {}", this->name, message);
    this->ctPopup->appendLogAsync(fmt::format("[{}] {}", this->name, message), ccColor3B{ 173, 173, 173 });
}

void Test::logInfo(std::string_view message) {
    log::info("[{}] {}", this->name, message);
    this->ctPopup->appendLogAsync(fmt::format("[{}] {}", this->name, message), ccColor3B{ 57, 191, 253 });
}

void Test::logWarn(std::string_view message) {
    log::warn("[{}] {}", this->name, message);
    this->ctPopup->appendLogAsync(fmt::format("[{}] {}", this->name, message), ccColor3B{ 235, 232, 62 });
}

void Test::logError(std::string_view message) {
    log::error("[{}] {}", this->name, message);
    this->ctPopup->appendLogAsync(fmt::format("[{}] {}", this->name, message), ccColor3B{ 255, 80, 80 });
}

/* StatusCell */

StatusCell* StatusCell::create(const char* name, float width) {
    auto ret = new StatusCell;
    if (ret->init(name, width)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool StatusCell::init(const char* name, float width) {
    if (!CCMenu::init()) return false;

    this->setContentSize({width, 18.f});

    Build<CCLabelBMFont>::create("?", "bigFont.fnt")
        .store(qmark);

    Build<CCSprite>::createSpriteName("GJ_deleteIcon_001.png")
        .store(xIcon);

    Build<CCSprite>::createSpriteName("GJ_completesIcon_001.png")
        .store(checkIcon);

    Build<BetterLoadingCircle>::create()
        .store(loadingCircle);

    loadingCircle->setOpacity(255);

    CCSize statusIconSize{12.f, 12.f};
    float myVertCenter = this->getContentHeight() / 2.f;

    auto setStatusIconData = [&](CCNode* thing) {
        thing->setPosition({10.f, myVertCenter});
        util::ui::rescaleToMatch(thing, statusIconSize);
        this->addChild(thing);
    };

    setStatusIconData(qmark);
    setStatusIconData(xIcon);
    setStatusIconData(checkIcon);
    setStatusIconData(loadingCircle);

    Build<CCLabelBMFont>::create(name, "bigFont.fnt")
        .limitLabelWidth(150.f, 0.4f, 0.1f)
        .scale(0.4f)
        .anchorPoint(0.f, 0.5f)
        .pos(22.f, myVertCenter)
        .parent(this)
        .store(nameLabel);

    Build<CCLabelBMFont>::create("", "bigFont.fnt")
        .scale(0.35f)
        .anchorPoint(0.f, 0.5f)
        .pos(180.f, myVertCenter)
        .parent(this)
        .store(stateLabel);

    Build<CCSprite>::createSpriteName("GJ_infoBtn_001.png")
        .with([&](auto spr) {
            util::ui::rescaleToMatch(spr, statusIconSize);
        })
        .intoMenuItem([this] {
            FLAlertLayer::create(
                nullptr,
                "Test failure",
                fmt::format("This test failed with the following error:\n\n<cy>{}</c>", this->failReason),
                "Ok",
                nullptr,
                380.f
            )->show();
        })
        .pos(width - 4.f - statusIconSize.width / 2.f, myVertCenter)
        .parent(this)
        .store(showFailReasonBtn);

    this->setUnknown();

    return true;
}

void StatusCell::softReload(const Test& test) {
    if (test.running) {
        this->setRunning();
    } else if (!test.finished) {
        this->setUnknown();
    } else if (test.failed) {
        this->setFailed(test.failReason);
    } else if (test.blocked) {
        this->setBlocked(test.dependsOn ? test.dependsOn->name : "Unknown");
    } else {
        this->setSuccessful();
    }
}

void StatusCell::setUnknown() {
    this->switchIcon(Icon::Unknown);
    this->showFailReasonBtn->setVisible(false);

    this->stateLabel->setString("Unknown");
}

void StatusCell::setRunning() {
    this->switchIcon(Icon::Running);
    this->showFailReasonBtn->setVisible(false);

    this->stateLabel->setString("Running");
}

void StatusCell::setFailed(std::string failReason) {
    this->switchIcon(Icon::Failed);
    this->showFailReasonBtn->setVisible(true);

    this->stateLabel->setString("Failed");
    this->failReason = std::move(failReason);
}

void StatusCell::setBlocked(std::string_view depTest) {
    this->switchIcon(Icon::Blocked);
    this->showFailReasonBtn->setVisible(true);

    this->stateLabel->setString("Blocked");
    this->failReason = fmt::format("This test was not ran because it depends on the test \"{}\", which did not run successfully.", depTest);
}

void StatusCell::setSuccessful() {
    this->switchIcon(Icon::Successful);
    this->showFailReasonBtn->setVisible(false);

    this->stateLabel->setString("Successful");
}

void StatusCell::switchIcon(Icon icon) {
    this->qmark->setVisible(icon == Icon::Unknown);
    this->xIcon->setVisible(icon == Icon::Failed || icon == Icon::Blocked);
    this->checkIcon->setVisible(icon == Icon::Successful);
    this->loadingCircle->setVisible(icon == Icon::Running);
}

/* LogMessageCell */

bool LogMessageCell::init(const std::string& message, cocos2d::ccColor3B color, double ts, float width) {
    constexpr float PAD = 5.f;

    this->timestamp = ts;

    auto ta = SimpleTextArea::create(message, "chatFont.fnt", 0.5f, width - PAD * 2.f);
    ta->setColor(globed::into<ccColor4B>(color));
    ta->setAnchorPoint({0.f, 0.f});
    ta->setPosition(PAD, 2.f);
    this->addChild(ta);

    this->setContentSize({width, ta->getScaledContentHeight() + 4.f});

    return true;
}

LogMessageCell* LogMessageCell::create(const std::string& message, cocos2d::ccColor3B color, double ts, float width) {
    auto ret = new LogMessageCell;
    if (ret->init(message, color, ts, width)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

