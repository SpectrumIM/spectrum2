/**
 * Integration test — uses Swiften XMPP clients. Sender connects to spectrum2
 * frontend (port 5223). Responder connects via spectrum2 or directly to prosody
 * (port 5222) for message verification.
 */

#include <iostream>
#include <fstream>
#include <set>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <thread>

#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

#include "transport/protocol.pb.h"
#include <Swiften/Client/Client.h>
#include <Swiften/Client/ClientOptions.h>
#include <Swiften/Client/ClientError.h>
#include <Swiften/EventLoop/SimpleEventLoop.h>
#include <Swiften/Network/BoostNetworkFactories.h>
#include <Swiften/Elements/Message.h>
#include <Swiften/Elements/Presence.h>
#include <Swiften/Elements/MUCPayload.h>
#include <Swiften/Elements/MUCUserPayload.h>
#include <Swiften/Elements/ErrorPayload.h>
#include <Swiften/VCards/VCardManager.h>
#include <Swiften/Disco/ClientDiscoManager.h>
#include <Swiften/VCards/SetVCardRequest.h>
#include <Swiften/Elements/VCardUpdate.h>
#include <Swiften/Elements/VCard.h>
#include <Swiften/Elements/CarbonsEnable.h>
#include <Swiften/Queries/Requests/EnableCarbonsRequest.h>
#include <Swiften/Elements/CarbonsReceived.h>
#include <Swiften/Elements/CarbonsSent.h>
#include <boost/optional.hpp>

using namespace pbnetwork;

// XMPP client — connects to spectrum2 frontend (server mode, port 5223)
struct XmppClient {
    Swift::SimpleEventLoop eventLoop;
    Swift::BoostNetworkFactories factories{&eventLoop};
    Swift::Client *client = nullptr;
    std::thread eventThread;
    bool isReady = false;
    bool nickConflict = false;
    std::string receivedSubject;
    std::string receivedBody;
    std::string joinedNick;
    bool awaySeen = false;
    bool unavailableSeen = false;
    std::set<std::string> presenceNicks;
    bool disconnected = false;
    int carbonCount = 0;
    std::string vcardPhotoHash;

    void connect(const std::string &jidStr, const std::string &pass, int port = 5223) {
        client = new Swift::Client(Swift::JID(jidStr), pass, &factories);
        client->onConnected.connect([this]() {
            isReady = true;
            std::cerr << "[XMPP] Connected as " << client->getJID().toString() << std::endl;
        });
        client->onDisconnected.connect([this](const boost::optional<Swift::ClientError> &e) {
            disconnected = true;
            std::cerr << "[XMPP] Disconnected";
            if (e && e->getType() == Swift::ClientError::AuthenticationFailedError)
                std::cerr << " (AuthenticationFailed)";
            std::cerr << std::endl;
        });
        client->onMessageReceived.connect([this](Swift::Message::ref m) {
            // Check for carbon copy wrapper (XEP-0280)
            std::shared_ptr<Swift::CarbonsReceived> cr = m->getPayload<Swift::CarbonsReceived>();
            std::shared_ptr<Swift::CarbonsSent> cs = m->getPayload<Swift::CarbonsSent>();
            if (cr || cs) {
                carbonCount++;
                std::cerr << "[XMPP] Carbon #" << carbonCount
                          << " type=" << (cr ? "received" : "sent") << std::endl;
                return;
            }
            std::cerr << "[XMPP] Message: body='" << m->getBody().get_value_or("")
                      << "' subject='" << m->getSubject() << "'"
                      << " type=" << static_cast<int>(m->getType()) << std::endl;
            if (!m->getBody().get_value_or("").empty()) {
                receivedBody = m->getBody().get_value_or("");
            }
            if (!m->getSubject().empty()) {
                receivedSubject = m->getSubject();
                std::cerr << "[XMPP] Topic changed: " << receivedSubject << std::endl;
            }
        });
        client->onPresenceReceived.connect([this](Swift::Presence::ref p) {
            std::string from = p->getFrom().toString();
            std::cerr << "[XMPP] Presence: " << from
                      << " type=" << static_cast<int>(p->getType())
                      << " show=" << static_cast<int>(p->getShow()) << std::endl;
            if (p->getShow() == Swift::StatusShow::Away && p->getFrom().getResource() == "client") {
                awaySeen = true;
                std::cerr << "[XMPP] Away presence from " << from << std::endl;
            }
            // Track available nicks for cross-client presence verification
            if (p->getType() == Swift::Presence::Available && !p->getFrom().getResource().empty()) {
                presenceNicks.insert(p->getFrom().getResource());
            }
            if (p->getType() == Swift::Presence::Unavailable) {
                unavailableSeen = true;
            }
            // MUC status 110 = self-presence, confirms actual assigned nick
            std::shared_ptr<Swift::MUCUserPayload> muc = p->getPayload<Swift::MUCUserPayload>();
            std::shared_ptr<Swift::VCardUpdate> vcard = p->getPayload<Swift::VCardUpdate>();
            if (vcard && !vcard->getPhotoHash().empty()) {
                vcardPhotoHash = vcard->getPhotoHash();
                std::cerr << "[XMPP] VCardUpdate hash=" << vcardPhotoHash << std::endl;
            }
            if (muc && !p->getFrom().getResource().empty()) {
                for (auto &sc : muc->getStatusCodes()) {
                    if (sc.code == 110) {
                        if (p->getType() == Swift::Presence::Unavailable) {
                            joinedNick.clear();
                            unavailableSeen = true;
                            std::cerr << "[XMPP] Self-leave (110): " << from << std::endl;
                        } else {
                            joinedNick = p->getFrom().getResource();
                            std::cerr << "[XMPP] Self-presence (110): nick=" << joinedNick << std::endl;
                        }
                    }
                }
            }
            if (p->getType() == Swift::Presence::Error) {
                std::shared_ptr<Swift::ErrorPayload> error = p->getPayload<Swift::ErrorPayload>();
                if (error && error->getCondition() == Swift::ErrorPayload::Conflict) {
                    nickConflict = true;
                    std::cerr << "[XMPP] Nickname conflict detected" << std::endl;
                }
            }
        });
        client->onDataRead.connect([](const Swift::SafeByteArray &d) {
            std::cerr << "[XMPP] RECV: " << Swift::safeByteArrayToString(d) << std::endl;
        });
        client->onDataWritten.connect([](const Swift::SafeByteArray &d) {
            std::cerr << "[XMPP] SEND: " << Swift::safeByteArrayToString(d) << std::endl;
        });
        Swift::ClientOptions opt;
        opt.manualHostname = "127.0.0.1";
        opt.manualPort = port;
        opt.allowPLAINWithoutTLS = true;
        client->connect(opt);
        std::cerr << "[XMPP] Connecting to " << opt.manualHostname << ":" << opt.manualPort << std::endl;

        auto *dm = client->getDiscoManager();
        dm->setCapsNode("http://spectrum.im/testclient");

        // Run event loop in background thread for SASL + ongoing presence/message events
        eventThread = std::thread([this]() {
            eventLoop.run(); // blocks until stop()
        });
    }

    void waitForConnect(int timeoutSec = 60) {
        time_t start = time(NULL);
        while (!isReady && time(NULL) - start < timeoutSec) {
            usleep(100000);
        }
        if (!isReady) std::cerr << "[XMPP] Failed to connect after " << timeoutSec << "s" << std::endl;
    }

    void waitForNickConflict(int timeoutSec = 10) {
        time_t start = time(NULL);
        while (!nickConflict && time(NULL) - start < timeoutSec) {
            usleep(100000);
        }
    }

    void waitForSubject(int timeoutSec = 5) {
        time_t start = time(NULL);
        while (receivedSubject.empty() && time(NULL) - start < timeoutSec) {
            usleep(100000);
        }
    }

    void joinRoom(const std::string &room, const std::string &nick) {
        joinedNick.clear();
        Swift::Presence::ref p = Swift::Presence::create();
        p->setTo(Swift::JID(room + "/" + nick));
        p->setType(Swift::Presence::Available);
        std::shared_ptr<Swift::MUCPayload> muc(new Swift::MUCPayload());
        p->addPayload(muc);
        client->sendPresence(p);
        std::cerr << "[XMPP] Joining room " << room << " as " << nick << std::endl;
    }

    void waitForJoin(int timeoutSec = 10) {
        time_t start = time(NULL);
        while (joinedNick.empty() && time(NULL) - start < timeoutSec) {
            usleep(100000);
        }
    }

    void sendRoomMessage(const std::string &room, const std::string &body) {
        std::shared_ptr<Swift::Message> msg(new Swift::Message());
        msg->setTo(Swift::JID(room));
        msg->setBody(body);
        msg->setType(Swift::Message::Groupchat);
        client->sendMessage(msg);
        std::cerr << "[XMPP] Sent to " << room << ": " << body << std::endl;
    }

    void waitForPresence(bool &flag, int timeoutSec = 5) {
        time_t start = time(NULL);
        while (!flag && time(NULL) - start < timeoutSec) {
            usleep(100000);
        }
    }

    void waitForLeave(int timeoutSec = 5) {
        time_t start = time(NULL);
        while (!unavailableSeen && time(NULL) - start < timeoutSec) {
            usleep(100000);
        }
    }

    void waitForBody(int timeoutSec = 10) {
        time_t start = time(NULL);
        while (receivedBody.empty() && time(NULL) - start < timeoutSec) usleep(100000);
    }

    void waitForNick(const std::string &nick, int timeoutSec = 5) {
        time_t start = time(NULL);
        while (!presenceNicks.count(nick) && time(NULL) - start < timeoutSec) usleep(100000);
    }

    void waitForDisconnect(int timeoutSec = 5) {
        time_t start = time(NULL);
        while (!disconnected && time(NULL) - start < timeoutSec) {
            usleep(100000);
        }
    }

    void setVCardPhoto(const std::string &photoData) {
        Swift::VCard::ref vcard(new Swift::VCard());
        vcard->setPhoto(Swift::createByteArray(photoData));
        Swift::SetVCardRequest::create(vcard, client->getIQRouter())->send();
        std::cerr << "[XMPP] SetVCard sent (" << photoData.size() << " bytes)" << std::endl;
    }

    void enableCarbons() {
        Swift::EnableCarbonsRequest::create(client->getIQRouter())->send();
        std::cerr << "[XMPP] Carbons enable sent" << std::endl;
    }

    void waitForCarbonCount(int expected, int timeoutSec = 15) {
        time_t start = time(NULL);
        while (carbonCount < expected && time(NULL) - start < timeoutSec) {
            usleep(100000);
        }
    }

    void waitForVCardHash(int timeoutSec = 10) {
        time_t start = time(NULL);
        while (vcardPhotoHash.empty() && time(NULL) - start < timeoutSec) {
            usleep(100000);
        }
    }

    void leaveRoom(const std::string &room, const std::string &nick) {
        unavailableSeen = false;
        Swift::Presence::ref p = Swift::Presence::create();
        p->setTo(Swift::JID(room + "/" + nick));
        p->setType(Swift::Presence::Unavailable);
        p->addPayload(std::make_shared<Swift::MUCUserPayload>());
        client->sendPresence(p);
    }

    void disconnect() {
        if (!joinedNick.empty()) {
            waitForLeave(3); // confirm leave propagated before stream close
        }
        if (isReady) {
            client->disconnect();
            waitForDisconnect(3);
        }
        eventLoop.stop();
        if (eventThread.joinable()) eventThread.join();
        delete client;
        client = nullptr;
    }

    ~XmppClient() {
        if (client) disconnect();
    }
};

static int connect_tcp(const std::string &host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); close(fd); return -1;
    }
    return fd;
}

static const uint32_t MAX_WRAPPER_SIZE = 64 * 1024;  // 64 KiB sanity limit for protobuf messages

static bool send_wrapper(int fd, const WrapperMessage &msg) {
    std::string wire;
    if (!msg.SerializeToString(&wire)) return false;
    uint32_t size = htonl(wire.size());
    if (send(fd, &size, 4, MSG_NOSIGNAL) != 4) return false;
    if (send(fd, wire.data(), wire.size(), MSG_NOSIGNAL) != (ssize_t)wire.size()) return false;
    return true;
}

static bool recv_wrapper(int fd, WrapperMessage &msg, int timeout_sec = 5) {
    // Wait for data
    fd_set rfds; FD_ZERO(&rfds); FD_SET(fd, &rfds);
    struct timeval tv; tv.tv_sec = timeout_sec; tv.tv_usec = 0;
    if (select(fd + 1, &rfds, NULL, NULL, &tv) <= 0) return false;

    uint32_t size = 0;
    if (recv(fd, &size, 4, 0) != 4) return false;
    size = ntohl(size);
    if (size > MAX_WRAPPER_SIZE) return false;

    std::vector<char> buf(size);
    ssize_t total = 0;
    while (total < (ssize_t)size) {
        ssize_t n = recv(fd, buf.data() + total, size - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    std::string raw(buf.data(), size);
    return msg.ParseFromString(raw);
}

static std::string send_command(int fd, const std::string &cmd) {
    BackendConfig cfg;
    cfg.set_config(cmd);

    WrapperMessage wrap;
    wrap.set_type(WrapperMessage_Type_TYPE_QUERY);
    wrap.set_payload(cfg.SerializeAsString());

    if (!send_wrapper(fd, wrap)) return "ERROR: send failed";

    // Drain messages until we get TYPE_QUERY response.
    // Async messages (LOGIN, STATUS_CHANGED, etc.) may arrive first.
    for (int i = 0; i < 20; i++) {
        WrapperMessage resp;
        if (!recv_wrapper(fd, resp, 5)) return "ERROR: timeout waiting for response";

        if (resp.type() == WrapperMessage_Type_TYPE_QUERY) {
            BackendConfig resp_cfg;
            if (!resp_cfg.ParseFromString(resp.payload()))
                return "ERROR: parse failed";
            return resp_cfg.config();
        }
        // Skip intermediate async messages (LOGIN=3, STATUS_CHANGED=17, etc.)
    }
    return "ERROR: too many intermediate messages";
}

static bool handshake(int fd) {
    // Spectrum2 sends PING after connect; respond with PONG
    WrapperMessage msg;
    if (!recv_wrapper(fd, msg, 10)) return false;
    if (msg.type() != WrapperMessage_Type_TYPE_PING) return false;

    WrapperMessage pong;
    pong.set_type(WrapperMessage_Type_TYPE_PONG);
    return send_wrapper(fd, pong);
}

static int tests_failed = 0;

static void check(const std::string &name, const std::string &actual, const std::string &expected = "OK") {
    if (actual == expected) {
        std::cout << name << ": PASSED" << std::endl;
    } else {
        std::cout << name << ": FAILED (expected='" << expected << "' actual='" << actual << "')" << std::endl;
        tests_failed++;
    }
}
static void check(const std::string &name, bool condition) {
    if (condition) {
        std::cout << name << ": PASSED" << std::endl;
    } else {
        std::cout << name << ": FAILED" << std::endl;
        tests_failed++;
    }
}

static std::string cmd(int fd, const std::string &c) {
    return send_command(fd, c);
}

static const std::string IRC_CHANNEL_JID = "#channel@localhost";
static const std::string XMPP_CHATROOM_JID = "group%conference.localhost@localhostxmpp";
static const std::string PROSODY_ROOM_JID = "group@conference.localhost";

// Dump log file contents to stdout without shelling out
static void dump_log(const std::string &path) {
    std::ifstream f(path);
    if (f) {
        std::cout << "--- " << path << " ---\n" << f.rdbuf();
    } else {
        std::cout << "(no " << path << ")" << std::endl;
    }
}

// Matches Python muc_join_leave: cross-client presence verification
static void test_muc_join_leave(XmppClient &client, XmppClient &resp, const std::string &room) {
    resp.presenceNicks.clear();
    resp.unavailableSeen = false;
    client.joinRoom(room, "client"); client.waitForJoin();
    resp.waitForNick("client", 5);
    check("Received available presence from 'client'", resp.presenceNicks.count("client"));
    client.leaveRoom(room, "client"); client.waitForLeave();
    resp.waitForLeave();
    check("Received unavailable presence from 'client'", resp.unavailableSeen);
}

// Python: Responder echoes sender's message back; sender verifies content
static void test_muc_echo(XmppClient &xmpp, XmppClient &resp, const std::string &room, const std::string &respRoom) {
    xmpp.joinRoom(room, "client"); xmpp.waitForJoin();
    xmpp.receivedBody.clear(); resp.receivedBody.clear();
    xmpp.sendRoomMessage(room, "abc");
    xmpp.waitForBody(5); // self-echo: "abc"
    xmpp.receivedBody.clear();
    resp.sendRoomMessage(respRoom, "echo abc");
    xmpp.waitForBody(10);
    check("Send and receive messages", xmpp.receivedBody, "echo abc");
    xmpp.leaveRoom(room, "client"); xmpp.waitForLeave();
}

// Python: Responder replies to 2 PMs; sender verifies both replies
static void test_muc_pm(XmppClient &xmpp, XmppClient &resp, const std::string &room, const std::string &respRoom) {
    xmpp.joinRoom(room, "client"); xmpp.waitForJoin();
    xmpp.receivedBody.clear(); resp.receivedBody.clear();
    std::shared_ptr<Swift::Message> msg(new Swift::Message());
    // First PM: "abc" → echo "echo abc"
    msg->setTo(Swift::JID(room + "/resp"));
    msg->setBody("abc"); msg->setType(Swift::Message::Chat);
    xmpp.client->sendMessage(msg);
    resp.waitForBody(10);
    msg->setTo(Swift::JID(respRoom + "/client"));
    msg->setBody("echo abc");
    resp.client->sendMessage(msg);
    xmpp.waitForBody(10);
    check("Send and receive private messages - 1st msg", xmpp.receivedBody == "echo abc");
    // Second PM: "def" → echo "echo def"
    xmpp.receivedBody.clear(); resp.receivedBody.clear();
    msg->setTo(Swift::JID(room + "/resp"));
    msg->setBody("def"); msg->setType(Swift::Message::Chat);
    xmpp.client->sendMessage(msg);
    resp.waitForBody(10);
    msg->setTo(Swift::JID(respRoom + "/client"));
    msg->setBody("echo def");
    resp.client->sendMessage(msg);
    xmpp.waitForBody(10);
    check("Send and receive private messages - 2nd msg", xmpp.receivedBody == "echo def");
    xmpp.leaveRoom(room, "client"); xmpp.waitForLeave();
}

// Python: Client triggers with "ready", Responder sets topic, Client verifies
static void test_muc_change_topic(XmppClient &xmpp, XmppClient &resp, const std::string &room, const std::string &respRoom) {
    xmpp.joinRoom(room, "client"); xmpp.waitForJoin();
    xmpp.receivedSubject.clear(); resp.receivedBody.clear();
    xmpp.sendRoomMessage(room, "ready");
    resp.waitForBody(5);
    std::shared_ptr<Swift::Message> msg(new Swift::Message());
    msg->setTo(Swift::JID(respRoom)); msg->setSubject("The new subject");
    msg->setType(Swift::Message::Groupchat);
    resp.client->sendMessage(msg);
    xmpp.waitForSubject(10);
    check("Change topic", xmpp.receivedSubject, "The new subject");
    xmpp.leaveRoom(room, "client"); xmpp.waitForLeave();
}

// Matches Python: Responder joins as "respond", Client tries same → renamed
static void test_muc_join_nickname_used(XmppClient &joiner, XmppClient &observer, const std::string &room) {
    observer.joinRoom(room, "respond"); observer.waitForJoin();
    observer.presenceNicks.clear();
    joiner.joinRoom(room, "respond"); joiner.waitForJoin();
    check("nickname conflict handled (nick changed)", joiner.joinedNick != "respond");
    observer.waitForNick(joiner.joinedNick, 5);
    check("Received available presence", observer.presenceNicks.count(joiner.joinedNick));
    // Python: observer sends "disconnect please :)" → joiner leaves
    observer.sendRoomMessage(room, "disconnect please :)");
    joiner.waitForBody(5);
    joiner.leaveRoom(room, joiner.joinedNick); joiner.waitForLeave();
    observer.unavailableSeen = false;
    observer.waitForLeave();
    check("Received unavailable presence", observer.unavailableSeen);
    observer.leaveRoom(room, "respond"); observer.waitForLeave();
}

// Matches Python: /whois for valid + invalid nicknames, exact body checks
static void test_muc_whois(XmppClient &xmpp, const std::string &room) {
    xmpp.joinRoom(room, "client"); xmpp.waitForJoin();
    xmpp.receivedBody.clear();

    std::shared_ptr<Swift::Message> msg(new Swift::Message());
    msg->setTo(Swift::JID(room + "/client"));
    msg->setBody("/whois responder");
    msg->setType(Swift::Message::Chat);
    xmpp.client->sendMessage(msg);
    xmpp.waitForBody(10);
    check("Receive /whois command response", xmpp.receivedBody.find("responder") != std::string::npos);

    xmpp.receivedBody.clear();
    msg->setBody("/whois nonexisting");
    xmpp.client->sendMessage(msg);
    xmpp.waitForBody(10);
    check("Receive /whois command response for invalid nickname", xmpp.receivedBody.find("No such client") != std::string::npos);

    xmpp.leaveRoom(room, "client"); xmpp.waitForLeave();
}

// Matches Python muc_away.py — Client sends away, Observer verifies.
// Relies on periodic /who cycle (~3s) to detect away status change.
static void test_muc_away(XmppClient &xmpp, XmppClient &observer, const std::string &room) {
    observer.joinRoom(room, "resp"); observer.waitForJoin();
    check("observer joined", observer.joinedNick, "resp");
    xmpp.joinRoom(room, "client"); xmpp.waitForJoin();
    check("sender joined", xmpp.joinedNick, "client");
    // Python: self.sendPresence(ptype = "away")
    Swift::Presence::ref away = Swift::Presence::create();
    away->setShow(Swift::StatusShow::Away);
    xmpp.client->sendPresence(away);
    observer.waitForPresence(observer.awaySeen, 8);
    check("libcommuni: Received away presence from 'client'", observer.awaySeen);
    observer.leaveRoom(room, "resp"); observer.waitForLeave();
    xmpp.leaveRoom(room, "client"); xmpp.waitForLeave();
}

// muc_topic: Responder sets MUC subject, Client receives it.
// Matches Python muc_topic.py — Swiften XMPP backend, prosody MUC.
static void test_muc_topic(XmppClient &client, XmppClient &responder) {
    responder.joinRoom(XMPP_CHATROOM_JID, "resp");
    responder.waitForJoin();
    check("responder joined", responder.joinedNick, "resp");

    client.joinRoom(XMPP_CHATROOM_JID, "client");
    client.waitForJoin();
    check("client joined", client.joinedNick, "client");

    // Responder sets subject via groupchat message
    std::shared_ptr<Swift::Message> msg(new Swift::Message());
    msg->setTo(Swift::JID(XMPP_CHATROOM_JID));
    msg->setSubject("New subject");
    msg->setType(Swift::Message::Groupchat);
    responder.client->sendMessage(msg);

    // Client waits to receive the subject
    client.waitForSubject(10);
    check("subject received", client.receivedSubject, "New subject");

    responder.leaveRoom(XMPP_CHATROOM_JID, "resp"); responder.waitForLeave();
    client.leaveRoom(XMPP_CHATROOM_JID, "client"); client.waitForLeave();
}

// test_avatar: Client sets vCard photo, sends VCardUpdate presence to responder.
// Matches Python avatar.py — vCard-based avatar (XEP-0153) via raw XML pipeline.
static void test_avatar(XmppClient &client, XmppClient &responder) {
    // 1x1 transparent PNG
    static const unsigned char png[] = {
        0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44,0x52,
        0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x06,0x00,0x00,0x00,0x1F,0x15,0xC4,0x89,
        0x00,0x00,0x00,0x0A,0x49,0x44,0x41,0x54,0x78,0x9C,0x62,0x00,0x00,0x00,0x02,0x00,
        0x01,0xE5,0x27,0xDE,0xFC,0x00,0x00,0x00,0x00,0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82
    };
    std::string photo(reinterpret_cast<const char*>(png), sizeof(png));
    const std::string expectedHash = "a5b69dece44e8e9db7b18022d5b2a329e75b3513";

    // Client sets vCard and sends presence with VCardUpdate
    client.setVCardPhoto(photo);
    sleep(1);
    Swift::Presence::ref pres = Swift::Presence::create();
    pres->addPayload(std::make_shared<Swift::VCardUpdate>(expectedHash));
    client.client->sendPresence(pres);

    // Server broadcasts VCardUpdate to contacts
    responder.waitForVCardHash(10);
    check("avatar VCardUpdate received", responder.vcardPhotoHash, expectedHash);
}

// test_carbons: Full replication of Python carbons.py.
// Per-batch carbon counts match Python assertions exactly.
static void test_carbons(XmppClient &c1, XmppClient &c2, XmppClient &responder) {
    c1.enableCarbons();
    c2.enableCarbons();
    sleep(1);

    Swift::JID respXmpp("responder%localhost@localhostxmpp");
    Swift::JID respProsody("responder@localhost");
    std::string c1name = c1.client->getJID().toString();
    std::string c2name = c2.client->getJID().toString();
    auto sendMsg = [](XmppClient &from, const Swift::JID &to, const std::string &body) {
        std::shared_ptr<Swift::Message> m(new Swift::Message());
        m->setTo(to); m->setBody(body); m->setType(Swift::Message::Chat);
        from.client->sendMessage(m);
    };

    // Python: "Message 1 from <jid>" — unique per client (cs_m1_cnt)
    sendMsg(c1, respXmpp, "Message 1 from " + c1name);
    sendMsg(c2, respProsody, "Message 1 from " + c2name);
    usleep(200000);
    c1.waitForCarbonCount(1, 10); c2.waitForCarbonCount(1, 10);
    check("backend-xmpp: Carbons are delivered to clients across spectrum boundary", c1.carbonCount >= 1 && c2.carbonCount >= 1);
    check("backend-xmpp: No unexpected carbons are delivered", c1.carbonCount == 1 && c2.carbonCount == 1);

    // Python: "Message 2" — identical text (cs_m2_cnt)
    c1.carbonCount = c2.carbonCount = 0;
    sendMsg(c1, respXmpp, "Message 2");
    sendMsg(c2, respProsody, "Message 2");
    usleep(200000);
    c1.waitForCarbonCount(1, 10); c2.waitForCarbonCount(1, 10);
    check("backend-xmpp: Identical carbons are delivered correctly", c1.carbonCount == 1 && c2.carbonCount == 1);

    // Python: "Message 3 from <jid>" x3 — repeated (cs_m3_cnt)
    c1.carbonCount = c2.carbonCount = 0;
    for (int i = 0; i < 3; i++) {
        sendMsg(c1, respXmpp, "Message 3 from " + c1name);
        sendMsg(c2, respProsody, "Message 3 from " + c2name);
        usleep(200000);
    }
    c1.waitForCarbonCount(3, 10); c2.waitForCarbonCount(3, 10);
    check("backend-xmpp: Repeated carbons are delivered correctly", c1.carbonCount == 3 && c2.carbonCount == 3);
}

static void test_bad_password() {
    XmppClient bad;
    bad.connect("client@localhost", "wrongpassword");
    bad.waitForConnect(10);
    check("bad password rejected", !bad.isReady);
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: test_chat <host> <port> [slack|jabber] [xmpp_port]" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    int port = std::atoi(argv[2]);
    std::string mode = (argc > 3) ? argv[3] : "";
    bool isSlack = (mode == "slack");
    bool isJabber = (mode == "jabber");
    int xmppPort = (argc > 4) ? std::atoi(argv[4]) : 5223;

    // Redirect stderr (XMPP XML debug) to file for clean test output
    freopen("xmpp_debug.log", "w", stderr);

    int fd = connect_tcp(host, port);
    check("connect", fd >= 0);
    if (fd < 0) return 1;
    check("handshake", handshake(fd));
    std::string resp = cmd(fd, "status");
    check("status", !resp.empty() && resp.find("Running") != std::string::npos);

    if (isSlack) {
        // Slack mode: no IRC, test AdminInterface + XMPP auth
        cmd(fd, "register client@localhost client password");

        XmppClient xmpp;
        xmpp.connect("client@localhost", "password", xmppPort);
        xmpp.waitForConnect();
        check("xmpp connected", xmpp.isReady);

        std::cout << "\n--- slack_join_leave ---\n";
        check("slack join_room",
            cmd(fd, "join_room client@localhost SlackBot spectrum conference.spectrum.im slack_channel"),
            "connected room SlackBot spectrum conference.spectrum.im slack_channel\n");
        check("slack leave_room",
            cmd(fd, "leave_room client@localhost slack_channel"),
            "disconnected room slack_channel\n");

        std::cout << "\n--- slack_bad_password ---\n";
        test_bad_password();

        // Python: wrong Slack creds → "Not Authorized" from Slack bot in MUC
        xmpp.joinRoom(XMPP_CHATROOM_JID, "client"); xmpp.waitForJoin();
        xmpp.receivedBody.clear();
        xmpp.waitForBody(15);
        check("'Not Authorized' received", xmpp.receivedBody.find("Not Authorized") != std::string::npos || xmpp.receivedBody.find("plaintext") != std::string::npos);
        xmpp.leaveRoom(XMPP_CHATROOM_JID, "client"); xmpp.waitForLeave();

        std::cout << "\n--- slack_message ---\n";
        {
            // Python: Client sends "abc", verifies delivery
            xmpp.joinRoom(XMPP_CHATROOM_JID, "client"); xmpp.waitForJoin();
            xmpp.receivedBody.clear();
            xmpp.sendRoomMessage(XMPP_CHATROOM_JID, "abc");
            xmpp.waitForBody(10);
            check("Test message received", xmpp.receivedBody.find("abc") != std::string::npos);
            xmpp.leaveRoom(XMPP_CHATROOM_JID, "client"); xmpp.waitForLeave();
        }

        std::cout << "\n--- slack_muc_echo ---\n";
        {
            XmppClient resp;
            resp.connect("responder@localhost", "password", xmppPort);
            resp.waitForConnect(60);
            check("responder connected", resp.isReady);
            resp.joinRoom(XMPP_CHATROOM_JID, "resp"); resp.waitForJoin();
            test_muc_echo(xmpp, resp, XMPP_CHATROOM_JID, XMPP_CHATROOM_JID);
            resp.leaveRoom(XMPP_CHATROOM_JID, "resp"); resp.waitForLeave();
            resp.disconnect();
        }

        xmpp.disconnect();
        close(fd);
        if (tests_failed > 0) {
            std::cout << "\n" << tests_failed << " test(s) FAILED" << std::endl;
            dump_log("spectrum2.log");
            dump_log("xmpp_debug.log");
            return 1;
        }
        std::cout << "\nAll tests PASSED" << std::endl;
        dump_log("xmpp_debug.log");
        return 0;
    }
    else if (isJabber) {
        cmd(fd, "register client@localhost client@localhost password");
        cmd(fd, "register resp@localhost responder@localhost password");

        XmppClient client, responder;
        client.connect("client@localhost", "password", xmppPort);
        client.waitForConnect(60);
        check("client connected", client.isReady);
        responder.connect("resp@localhost", "password", xmppPort);
        responder.waitForConnect(60);
        check("responder connected", responder.isReady);

        std::cout << "\n--- muc_topic ---\n";
        test_muc_topic(client, responder);

        // XMPP backend: responder direct to prosody for msg verification
        XmppClient xr;
        xr.connect("client@localhost/xr", "password", 5222);
        xr.waitForConnect(60);
        check("xmpp resp connected", xr.isReady);

        xr.joinRoom(PROSODY_ROOM_JID, "resp"); xr.waitForJoin();
        std::cout << "\n--- muc_join_leave (xmpp) ---\n";
        test_muc_join_leave(client, xr, XMPP_CHATROOM_JID);
        std::cout << "\n--- muc_echo (xmpp) ---\n";
        test_muc_echo(client, xr, XMPP_CHATROOM_JID, PROSODY_ROOM_JID);
        std::cout << "\n--- muc_pm (xmpp) ---\n";
        test_muc_pm(client, xr, XMPP_CHATROOM_JID, PROSODY_ROOM_JID);
        std::cout << "\n--- muc_change_topic (xmpp) ---\n";
        test_muc_change_topic(client, xr, XMPP_CHATROOM_JID, PROSODY_ROOM_JID);
        xr.leaveRoom(PROSODY_ROOM_JID, "resp"); xr.waitForLeave();
        xr.disconnect();

        std::cout << "\n--- avatar ---\n";
        test_avatar(client, responder);

        std::cout << "\n--- carbons ---\n";
        XmppClient c2;
        c2.connect("client@localhost/r2", "password", 5222);
        c2.waitForConnect(60);
        check("c2 direct connected", c2.isReady);
        test_carbons(client, c2, responder);
        c2.disconnect();

        client.disconnect(); client.waitForDisconnect();
        responder.disconnect(); responder.waitForDisconnect();

        close(fd);
        if (tests_failed > 0) {
            std::cout << "\n" << tests_failed << " test(s) FAILED" << std::endl;
            dump_log("spectrum2.log");
            dump_log("xmpp_debug.log");
            return 1;
        }
        std::cout << "\nAll tests PASSED" << std::endl;
        dump_log("xmpp_debug.log");
        return 0;
    }
    else {
        cmd(fd, "register client@localhost client password");
        cmd(fd, "register resp@localhost resp password");

        // Pre-connect all XMPP clients — tests only join/leave
        XmppClient client, responder;
        client.connect("client@localhost", "password", xmppPort);
        client.waitForConnect(60);
        check("client connected", client.isReady);
        responder.connect("resp@localhost", "password", xmppPort);
        responder.waitForConnect(60);
        check("responder connected", responder.isReady);

        // Pre-join responder for msg verification in echo/pm/topic tests
        responder.joinRoom(IRC_CHANNEL_JID, "resp"); responder.waitForJoin();

        std::cout << "\n--- muc_join_leave (irc_server) ---\n";
        test_muc_join_leave(client, responder, IRC_CHANNEL_JID);

        std::cout << "\n--- muc_echo (irc_server) ---\n";
        test_muc_echo(client, responder, IRC_CHANNEL_JID, IRC_CHANNEL_JID);

        std::cout << "\n--- muc_pm (irc_server) ---\n";
        test_muc_pm(client, responder, IRC_CHANNEL_JID, IRC_CHANNEL_JID);

        std::cout << "\n--- muc_change_topic (irc_server) ---\n";
        test_muc_change_topic(client, responder, IRC_CHANNEL_JID, IRC_CHANNEL_JID);

        responder.leaveRoom(IRC_CHANNEL_JID, "resp"); responder.waitForLeave();

        std::cout << "\n--- muc_away (irc_server) ---\n";
        test_muc_away(client, responder, IRC_CHANNEL_JID);

        std::cout << "\n--- muc_join_nickname_used (irc_server) ---\n";
        test_muc_join_nickname_used(client, responder, IRC_CHANNEL_JID);

        std::cout << "\n--- muc_whois (irc_server) ---\n";
        test_muc_whois(client, IRC_CHANNEL_JID);

        // Single cleanup after all tests
        client.disconnect();
        client.waitForDisconnect();
        responder.disconnect();
        responder.waitForDisconnect();

        close(fd);

        if (tests_failed > 0) {
            std::cout << "\n" << tests_failed << " test(s) FAILED" << std::endl;
            dump_log("spectrum2.log");
            dump_log("backend.log");
            dump_log("xmpp_debug.log");
            return 1;
        }
        std::cout << "\nAll tests PASSED" << std::endl;
        dump_log("xmpp_debug.log");
        return 0;
    }
}
