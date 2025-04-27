#include "connection_test_popup.hpp"

#include <Geode/utils/web.hpp>
#include <asp/net/IpAddress.hpp>
#include <matjson/stl_serialize.hpp>
#include <matjson/reflect.hpp>

#include <data/packets/client/connection.hpp>
#include <data/packets/server/connection.hpp>
#include <managers/error_queues.hpp>
#include <managers/web.hpp>
#include <managers/central_server.hpp>
#include <managers/game_server.hpp>
#include <managers/settings.hpp>
#include <managers/popup.hpp>
#include <net/manager.hpp>
#include <net/tcp_socket.hpp>
#include <net/game_socket.hpp>
#include <net/address.hpp>
#include <util/misc.hpp>
#include <util/format.hpp>
#include <util/ui.hpp>
#include <ui/general/ask_input_popup.hpp>

using namespace geode::prelude;
using namespace asp::time;
using namespace util::rng;
using ConnectionState = NetworkManager::ConnectionState;
using StatusCell = ConnectionTestPopup::StatusCell;
using LogMessageCell = ConnectionTestPopup::LogMessageCell;
using Test = ConnectionTestPopup::Test;
using Protocol = GameSocket::Protocol;

constexpr std::string_view URL_OVERRIDE_KEY = "connection-test-url-override";

template <typename T>
static Result<std::shared_ptr<T>> waitForPacketOn(
    GameSocket& sock,
    Protocol proto,
    int timeoutMs = 5000,
    bool skipUdpMarker = true,
    bool mustFromConnected = false
) {
    auto endAt = SystemTime::now() + Duration::fromMillis(timeoutMs);

    // wait up to 5s for response
    while (endAt.isFuture()) {
        auto toPoll = endAt.until().millis();
        if (toPoll == 0) {
            break; // safeguard
        }

        Result<bool> pollRes = Err("");

        pollRes = sock.poll(proto, toPoll);
        if (!pollRes) {
            return Err("Failed to poll socket: {}", pollRes.unwrapErr());
        }

        auto r = pollRes.unwrap();
        if (!r) {
            break;
        }

        std::shared_ptr<Packet> respPkt;
        bool fromConnected;

        switch (proto) {
            case Protocol::Unspecified: {
                auto res = sock.recvPacket();
                if (!res) {
                    return Err("Failed to receive packet: {}", res.unwrapErr());
                }

                auto& rp = res.unwrap();

                respPkt = std::move(rp.packet);
                fromConnected = rp.fromConnected;
            } break;

            case Protocol::Tcp: {
                auto res = sock.recvPacketTCP();
                if (!res) {
                    return Err("Failed to receive packet: {}", res.unwrapErr());
                }

                respPkt = std::move(res).unwrap();
                fromConnected = true;
            } break;

            case Protocol::Udp: {
                while (!respPkt) {
                    auto res = sock.recvPacketUDP(skipUdpMarker);
                    if (!res) {
                        return Err("Failed to receive packet: {}", res.unwrapErr());
                    }

                    auto& maybePacket = res.unwrap();

                    if (maybePacket) {
                        respPkt = std::move(maybePacket.value().packet);
                        fromConnected = maybePacket.value().fromConnected;
                    } else {
                        // if it returned nullopt and no error, this means we received a udp frame.
                        // keep looping until we have the entire packet.
                        auto pollRes = sock.poll(Protocol::Udp, 50);
                        if (!pollRes) {
                            return Err("Failed to poll socket (for UDP frame): {}", pollRes.unwrapErr());
                        }
                    }
                }
            } break;
        }

        if (!respPkt) {
            return Err("Failed to decode response? (null packet)");
        }

        if (mustFromConnected && !fromConnected) {
            continue;
        }

        auto neededPkt = respPkt->tryDowncast<T>();
        if (!neededPkt) {
            return Err("Unexpected packet, expected {}, got packet ID {}", util::misc::getTypeName<T>(), respPkt->getPacketId());
        }

        return Ok(std::static_pointer_cast<T>(std::move(respPkt)));
    }

    return Err("Timed out; no response was received from the server.");
}

bool ConnectionTestPopup::setup() {
    auto& nm = NetworkManager::get();
    if (nm.getConnectionState() != ConnectionState::Disconnected) {
        nm.disconnect();
    }

    this->setTitle("Connection test");
    this->usedCentralUrl = globed::string<"main-server-url">();

    // Backup game servers
    auto& gsm = GameServerManager::get();
    gsm.backupInternalData();

    auto& gs = GlobedSettings::get();

    auto override = Mod::get()->getSavedValue<std::string>(URL_OVERRIDE_KEY);

    if (gs.launchArgs().devStuff && !override.empty() && override.starts_with("http")) {
        this->usedCentralUrl = override;
        this->isCentralUrlOverriden = true;
    }

    while (this->usedCentralUrl.ends_with('/')) {
        this->usedCentralUrl.pop_back();
    }

    this->scheduleUpdate();

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    // make the test list
    Build(GlobedListLayer<StatusCell>::createForComments(LIST_WIDTH, POPUP_HEIGHT * 0.7f))
        .visible(false)
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
        .pos(rlayout.fromTopRight(24.f, 20.f))
        .visible(false)
        .store(logToggler)
        .parent(m_buttonMenu);

    // button to start testing
    Build(ButtonSprite::create("Start", "bigFont.fnt", "GJ_button_01.png", 0.8f))
        .intoMenuItem([this](auto dabutton) {
            dabutton->setVisible(false);
            this->startTesting();
        })
        .pos(rlayout.center)
        .parent(m_buttonMenu);

    if (gs.launchArgs().devStuff) {
        CCSprite* pencil;
        Build(CircleButtonSprite::create(
            Build<CCSprite>::createSpriteName("pencil.png"_spr)
                .store(pencil)
                .collect(),
            CircleBaseColor::Green,
            CircleBaseSize::Small
        ))
            .scale(0.7f)
            .intoMenuItem([] {
                AskInputPopup::create("Change testing URL", [](auto newUrl) {
                    geode::createQuickPopup(
                        "Note",
                        "Changing the URL is <cr>not recommended</c> unless you know what you are doing. Only do it if you are having troubles connecting to a <cy>specific</c> server. Are you sure you want to proceed?",
                        "Cancel",
                        "Yes",
                        [newUrl = std::string(newUrl)](auto, bool yes) {
                            if (!yes) return;

                            Mod::get()->setSavedValue(URL_OVERRIDE_KEY, newUrl);

                            PopupManager::get().alert("Note", "Reopen the connection test popup for the change to apply.").showInstant();
                        }
                    );
                }, 80, "Server URL", util::misc::STRING_URL)->show();
            })
            .pos(rlayout.fromTopRight(24.f, 20.f))
            .store(pencilBtn)
            .parent(m_buttonMenu);

        pencil->setScale(pencil->getScale() * 0.9f); // needed!
    }

    // add the tests

    gtcpTest = this->addTest("Google TCP Test", [](Test* test) {
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

    ghttpTest = this->addTest("Google HTTP Test", [](Test* test) {
        test->logInfo("Sending a HEAD request to Google");

        auto task = WebRequestManager::get().testGoogle();

        while (task.isPending()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
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

    std::string_view srvdomain = usedCentralUrl;

    if (srvdomain.starts_with("https://")) {
        srvdomain.remove_prefix(sizeof("https://") - 1);
    } else if (srvdomain.starts_with("http://")) {
        srvdomain.remove_prefix(sizeof("http://") - 1);
    }

    while (srvdomain.ends_with('/')) {
        srvdomain.remove_prefix(1);
    }

    // let's not try to dns query an ip address
    if (!util::format::hasIpAddress(srvdomain)) {
        dnsTest = this->addTest("DNS Test", [srvdomain](Test* test) {
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

        traceTest = this->addTest("CF Trace Test", [srvdomain](Test* test) {
            test->logInfo(fmt::format("Retrieving cloudflare trace for domain {}", srvdomain));

            auto task = WebRequestManager::get().testCloudflareDomainTrace(srvdomain);

            while (task.isPending()) {
                std::this_thread::sleep_for(std::chrono::milliseconds{50});
            }

            auto val = task.getFinishedValue();
            if (!val) {
                test->fail("Task returned no value");
                return;
            }

            if (!val->ok()) {
                test->fail(fmt::format("Connection to cloudflare failed: {}", val->getError()));
                return;
            }

            auto text = val->text().unwrapOrDefault();
            auto lines = util::format::split(text, "\n");

            std::string_view datacenter, loc, tls;
            std::string ip;
            for (auto line : lines) {
                if (line.starts_with("colo=")) {
                    line.remove_prefix(5);
                    datacenter = line;
                } else if (line.starts_with("loc=")) {
                    line.remove_prefix(4);
                    loc = line;
                } else if (line.starts_with("tls=")) {
                    line.remove_prefix(4);
                    tls = line;
                } else if (line.starts_with("ip=")) {
                    line.remove_prefix(3);
                    ip = line;
                }
            }

            // some users might not want their ip in logs /shrug
            if (!ip.empty()) {
                auto res = asp::net::Ipv4Address::tryFromString(ip);
                if (res) {
                    auto octets = res.unwrap().octets();
                    ip = fmt::format("{}.{}.x.x", octets[0], octets[1]);
                } else {
                    log::warn("Failed to parse IP: error {}", (int) res.unwrapErr());
                    ip = "<invalid>";
                }
            }

            test->logTrace(fmt::format("CF datacenter={}, loc={}, tls={}, ip={}", datacenter, loc, tls, ip));

            test->logInfo("Request was successful!");
            test->finish();
        });
    }

    centralTest = this->addTest("Central Test", [this](Test* test) {
        std::string_view url = this->usedCentralUrl;

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

        // validate response
        auto respText = val->text().unwrapOrDefault();
        auto parseResult = matjson::parse(respText);
        if (!parseResult) {
            log::warn("versioncheck invalid response: \n{}", respText);

            // check if
            if (respText.find("<html>") != std::string::npos || respText.find("<body>") != std::string::npos) {
                test->fail("Server erroneously returned an HTML response instead of a JSON structure");
            } else {
                test->fail(fmt::format("Failed to parse JSON sent by the server: {}", parseResult.unwrapErr()));
            }

            return;
        }

        // if it's a real json we just assume it works fine xd

        test->finish();
    });

    srvListTest = this->addTest("Server List Test", [this](Test* test) {
        std::string_view url = this->usedCentralUrl;

        test->logInfo(fmt::format("Attempting to fetch {}/v3/meta", url));

        auto task = WebRequestManager::get().fetchServerMeta(url);

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

        auto& csm = CentralServerManager::get();
        auto cres = matjson::parseAs<MetaResponse>(resp);
        if (!cres) {
            test->logWarn(fmt::format("Failed to parse server list: {}", cres.unwrapErr()));
            test->logWarn(fmt::format("Server response: \"{}\"", truncResp));
            test->fail("Invalid response was sent by the server");
            return;
        }

        csm.initFromMeta(cres.unwrap());

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
        this->addTest(server.name, [server = server](Test* test) {
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

            auto r = waitForPacketOn<PingResponsePacket>(sock, Protocol::Tcp);
            if (!r) {
                test->fail(r.unwrapErr());
                return;
            }

            auto pongpkt = std::move(r).unwrap();

            test->logTrace(fmt::format("Received TCP ping response, id: {}, player count: {}", pongpkt->id, pongpkt->playerCount));

            if (pongpkt->id != pingId) {
                test->fail("Server responded unexpected ping ID");
                return;
            }

            // send a UDP ping packet
            pingId += 1;
            res = sock.sendPacketUDP(PingPacket::create(pingId));

            test->logTrace(fmt::format("Sending a UDP ping, id: {}", pingId));

            if (!res) {
                test->fail(fmt::format("Failed to send (UDP) ping packet: {}", res.unwrapErr()));
                return;
            }

            r = waitForPacketOn<PingResponsePacket>(sock, Protocol::Udp);
            if (!r) {
                test->fail(r.unwrapErr());
                return;
            }

            pongpkt = std::move(r).unwrap();

            test->logTrace(fmt::format("Received UDP ping response, id: {}, player count: {}", pongpkt->id, pongpkt->playerCount));

            if (pongpkt->id != pingId) {
                test->fail("Server responded unexpected ping ID");
                return;
            }

            test->logTrace("Cleaning up");

            // disconnect
            sock.disconnect();

            test->ctPopup->dontFinish = true;

            // add packet test if needed
            Loader::get()->queueInMainThread([addr, popup = test->ctPopup] {
                popup->queuePacketLimitTest(addr);
                popup->dontFinish = false;
            });

            test->finish();
        });
    }
}

void ConnectionTestPopup::queuePacketLimitTest(const NetworkAddress& addr) {
    // don't add it twice
    if (this->packetTest) {
        return;
    }

    packetTest = this->addTest("Packet Limit Test", [addr = addr](Test* test) {
        GameSocket sock;

        auto res = sock.connect(addr, false);
        if (!res) {
            test->fail(fmt::format("Failed to connect to server: {}", res.unwrapErr()));
            return;
        }

        test->logInfo("Connected successfully, starting packet limit test");

        constexpr static std::array testSizes = std::to_array<size_t>({
            1300,
            1400,
            2000,
            4000,
            7000,
            9500,
            14800,
            19800,
            29800,
            39800,
            49800,
            60000,
        });

        struct TestResult {
            float reliability = 0.f;
            float minTimeTaken = 1000000.f;
            float maxTimeTaken = 0.f;
            float avgTimeTaken = 0.f;
        };

        std::array<TestResult, testSizes.size()> results;
        auto& baseline = results[0];

        std::vector<uint8_t> data;
        data.reserve(testSizes.back());

        for (size_t i = 0; i < testSizes.size(); i++) {
            size_t tsize = testSizes[i];
            auto& testResult = results[i];

            // Send a test packet
            data.resize(tsize);
            Random::get().fill(data.data(), tsize);

            auto pkt = ConnectionTestPacket::create(0, data);
            auto clientpkt = static_cast<ConnectionTestPacket*>(pkt.get());

            size_t attempts;
            if (tsize < 10000) {
                attempts = 10;
            } else if (tsize <= 30000) {
                attempts = 7;
            } else {
                attempts = 5;
            }

            test->logTrace(fmt::format("Testing with size {}, doing {} attempts", tsize, attempts));

            auto totalStartTime = Instant::now();

            size_t successAttempts = 0;
            float minTaken = 10000000.f;
            float maxTaken = 0.f;

            for (size_t att = 0; att < attempts; att++) {
                if (att >= 3 && successAttempts == 0) {
                    test->logWarn("All attempts failed so far, skipping to the next size");
                    attempts = att;
                    break;
                }

                if (test->ctPopup->waitingForThreadTerm) {
                    test->logWarn("Interrupting test, cancellation was requested");
                    test->finish();
                    return;
                }

                auto startTime = Instant::now();

                clientpkt->uid = Random::get().generate<uint32_t>();
                auto res = sock.sendPacketUDP(pkt);
                if (!res) {
                    test->fail(fmt::format("Attempt {} failed: udp send failed: {}", att, res.unwrapErr()));
                    continue;
                }

                auto res2 = waitForPacketOn<ConnectionTestResponsePacket>(sock, Protocol::Udp);
                if (!res2) {
                    test->logWarn(fmt::format("Attempt {} failed: {}", att, res2.unwrapErr()));
                    continue;
                }

                auto resppkt = std::move(res2).unwrap();

                // verify data
                if (clientpkt->uid != resppkt->uid) {
                    test->logWarn(fmt::format("Wrong uid in response, expected {}, got {}", clientpkt->uid, resppkt->uid));
                    continue;
                }

                if (clientpkt->data != resppkt->data) {
                    test->logWarn("Wrong data in response, binary data differs");
                    continue;
                }

                // success!
                successAttempts++;

                auto elapsed = startTime.elapsed();
                float timeTaken = elapsed.micros() / 1'000'000.f;
                minTaken = std::min(minTaken, timeTaken);
                maxTaken = std::max(maxTaken, timeTaken);

                // we will wait at least 150ms between attempts to not get rate limited
                if (elapsed.millis() < 150) {
                    std::this_thread::sleep_for(std::chrono::milliseconds{150 - elapsed.millis()});
                }
            }

            testResult.avgTimeTaken = totalStartTime.elapsed().micros() / 1'000'000.f / static_cast<float>(attempts);
            testResult.reliability = static_cast<float>(successAttempts) / attempts;
            testResult.minTimeTaken = minTaken;
            testResult.maxTimeTaken = maxTaken;

            test->logTrace(fmt::format(
                "Finished, results: reliability = {}%, avg time = {:.3f}s, min time = {:.3f}s, max time = {:.3f}s",
                static_cast<int>(testResult.reliability * 100),
                testResult.avgTimeTaken,
                testResult.minTimeTaken,
                testResult.maxTimeTaken
            ));

            if (std::abs(testResult.reliability) < 0.01f) {
                test->logTrace("Breaking early, this limit is already extremely unreliable!");
                break;
            }
        }

        size_t bestChosenSize = testSizes[0];

        for (size_t i = 0; i < testSizes.size(); i++) {
            size_t size = testSizes[i];
            auto& result = results.at(i);

            // if unstable, break!
            if (result.reliability < 0.95f) {
                break;
            }

            // i dont really do other tests here :P
            bestChosenSize = size;
        }

        if (bestChosenSize == testSizes.back()) {
            bestChosenSize = 0;
        }

        auto& gs = GlobedSettings::get();
        test->logInfo(fmt::format("Test finished, setting packet limit to {} (most stable), previous was {}", bestChosenSize, gs.globed.fragmentationLimit.get()));
        gs.globed.fragmentationLimit = bestChosenSize;

        test->finish();
    });
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

        this->showVerdictPopup();
    }
}

void ConnectionTestPopup::showVerdictPopup() {
    // a bit funky

    bool anyFailed = false;
    for (auto& test : this->tests) {
        if (test->didNotSucceed()) {
            anyFailed = true;
        }
    }

    std::string content;
    bool openLink = false;

    if (gtcpTest->didNotSucceed() || ghttpTest->didNotSucceed()) {
        content = "Basic network tests are <cr>failing</c>, this could likely be a problem with your <cy>internet connection</c> or <co>firewall</c>. Try disabling an antivirus if you have one.";
    } else if (dnsTest && dnsTest->didNotSucceed()) {
        // i dont really know how you would fix this
        content = "DNS resolution for the server failed, this may be due to a block by your ISP or a delay in DNS propagation. Try waiting for up to 24-48 hours to see if the issue is resolved.";
    } else if (centralTest->didNotSucceed()) {
        // this is silly
        if (centralTest->failReason.find("returned an HTML") != std::string::npos) {
            content = fmt::format(
                "The Globed server sent an <cr>erroneous response</c>. This could be a <cr>block</c> by your ISP or antivirus. Try visiting <cy>{}</c> in your browser and see whether it is reachable there.",
                this->usedCentralUrl
            );
        } else {
            content = fmt::format(
                "The Globed server could <cr>not be reached</c> or sent an <cr>erroneous response</c>. Try visiting <cy>{}</c> in your browser and see whether it is reachable there.",
                this->usedCentralUrl
            );
        }

        openLink = true;
    } else if (srvListTest->didNotSucceed()) {
        content = "The Globed server failed to properly respond with the server list. This should never happen.";
    } else if (packetTest && packetTest->didNotSucceed()) {
        content = "An error happened during the <cy>packet limit test</c>, meaning a potential issue with your <co>firewall</c> or <cg>router</c>. Try manually setting the packet limit to <cy>1400</c> in settings.";
    } else if (anyFailed) {
        // game server test failed?
        content = "An error happened during testing one or multiple <cj>game servers</c>. This isn't critical, assuming other servers work, but this should <cr>not</c> happen. See the <cg>logs</c> for more details.";
    } else {
        content = "All tests passed <cg>successfully</c> :)";
    }

    if (!openLink) {
        PopupManager::get().alert("Test Results", content, "Ok", nullptr, 380.f).showInstant();
    } else {
        geode::createQuickPopup("Test Results", content, "Cancel", "Open", 380.f, [url = this->usedCentralUrl](auto alert, bool yes) {
            if (yes) {
                geode::utils::web::openLinkInBrowser(url);
            }
        });
    }
}

void ConnectionTestPopup::startTesting() {
    if (!workThread.isStopped()) {
        ErrorQueues::get().warn("Attempting to call startTesting more than once");
        return;
    }

    this->startedTestingAt = Instant::now();
    workThread.start();
    hasBeenStarted = true;
    list->setVisible(true);
    logToggler->setVisible(true);

    if (pencilBtn) {
        pencilBtn->setVisible(false);
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
    if (reallyClose || !hasBeenStarted) {
        if (actuallyReallyClose || threadTerminated || !hasBeenStarted) {
            auto& gsm = GameServerManager::get();
            gsm.restoreInternalData();
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

bool Test::didNotSucceed() {
    return this->failed || this->blocked;
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
        .limitLabelWidth(150.f, 0.4f, 0.05f)
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
            PopupManager::get().alertFormat(
                "Test failure",
                "This test failed with the following error:\n\n<cy>{}</c>",
                this->failReason,
                380.f
            ).showInstant();
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

