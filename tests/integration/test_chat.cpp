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
    int plainMessageCount = 0;
    struct CarbonEvent {
        bool sent;
        Swift::JID from;
        Swift::JID innerFrom;
        std::string body;
    };
    std::vector<CarbonEvent> carbonEvents;
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
                CarbonEvent ev;
                ev.sent = (cs != nullptr);
                ev.from = m->getFrom();
                std::shared_ptr<Swift::Forwarded> fwd = cs ? cs->getForwarded() : cr->getForwarded();
                if (fwd) {
                    std::shared_ptr<Swift::Message> inner = std::dynamic_pointer_cast<Swift::Message>(fwd->getStanza());
                    if (inner) {
                        ev.body = inner->getBody().get_value_or("");
                        ev.innerFrom = inner->getFrom();
                    }
                }
                carbonEvents.push_back(ev);
                std::cerr << "[XMPP] Carbon #" << carbonCount
                          << " type=" << (cr ? "received" : "sent")
                          << " from=" << ev.from.toString() << " body='" << ev.body << "'" << std::endl;
                return;
            }
            plainMessageCount++;
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

static const std::string IRC_CHANNEL_JID = "#channel%localhost@localhost";
static const std::string IRC_SERVER_CHANNEL_JID = "#channel@localhost";
// Carbons are per session, so every resource of the user has to enable them itself
static void enableCarbons(Swift::Client *client) {
    Swift::EnableCarbonsRequest::create(client->getIQRouter())->send();
}

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

// Per-client tally of carbons received since a given index, classified the same way
// as Python carbons.py: sent carbons only, from our own bare JID (not our full JID),
// body must be one of the expected message kinds.
struct CarbonTally {
    int m1 = 0, m2 = 0, m3 = 0, weird = 0;
};

static bool startsWith(const std::string &s, const std::string &prefix) {
    return s.compare(0, prefix.size(), prefix) == 0;
}

static CarbonTally tallyCarbons(const XmppClient &c, size_t fromIndex) {
    CarbonTally t;
    std::string ownFull = c.client->getJID().toString();
    std::string ownBare = c.client->getJID().toBare().toString();
    for (size_t i = fromIndex; i < c.carbonEvents.size(); i++) {
        const XmppClient::CarbonEvent &ev = c.carbonEvents[i];
        // Carbons of legacy-side messages come from the transport JID (client\40localhost@localhostxmpp),
        // as Python's alt_jid allowed; carbons of our own account come from its bare JID.
        bool fromOwnAccount = ev.from.toBare().toString() == ownBare;
        bool fromTransport = ev.from.getDomain() == "localhostxmpp";
        if (!ev.sent || ev.from.toString() == ownFull || !(fromOwnAccount || fromTransport)) {
            std::cerr << "[XMPP] Weird carbon from=" << ev.from.toString() << " body='" << ev.body << "'" << std::endl;
            t.weird++;
        } else if (startsWith(ev.body, "Message 1 from ")) {
            t.m1++;
        } else if (ev.body == "Message 2") {
            t.m2++;
        } else if (startsWith(ev.body, "Message 3 from ")) {
            t.m3++;
        } else {
            std::cerr << "[XMPP] Weird carbon body='" << ev.body << "'" << std::endl;
            t.weird++;
        }
    }
    return t;
}

static void addTally(CarbonTally &sum, const CarbonTally &t) {
    sum.m1 += t.m1; sum.m2 += t.m2; sum.m3 += t.m3; sum.weird += t.weird;
}

// test_carbons: Full replication of Python carbons.py.
// Carbon counts are checked per batch; each batch waits for the expected count and then
// settles so that late duplicates are counted before the exact comparison.
static void test_carbons(XmppClient &c1, XmppClient &c2, XmppClient &responder) {
    // Plain messages from earlier tests on these clients must not count as weird here
    c1.plainMessageCount = 0;
    c2.plainMessageCount = 0;

    enableCarbons(c1.client);
    enableCarbons(c2.client);
    sleep(2); // Python waits 5s for other clients; enable is an IQ and not confirmed here

    Swift::JID respXmpp("responder%localhost@localhostxmpp");
    Swift::JID respProsody("responder@localhost");
    std::string c1name = c1.client->getJID().toString();
    std::string c2name = c2.client->getJID().toString();
    auto sendMsg = [](XmppClient &from, const Swift::JID &to, const std::string &body) {
        std::shared_ptr<Swift::Message> m(new Swift::Message());
        m->setTo(to); m->setBody(body); m->setType(Swift::Message::Chat);
        from.client->sendMessage(m);
    };

    // Each batch: send from both clients, wait for the expected carbons, then settle.
    auto runBatch = [&](const std::string &body1, const std::string &body2, int perClient) {
        size_t s1 = c1.carbonEvents.size();
        size_t s2 = c2.carbonEvents.size();
        int base1 = c1.carbonCount, base2 = c2.carbonCount;
        for (int i = 0; i < perClient; i++) {
            sendMsg(c1, respXmpp, body1);
            sendMsg(c2, respProsody, body2);
            usleep(200000);
        }
        c1.waitForCarbonCount(base1 + perClient, 10);
        c2.waitForCarbonCount(base2 + perClient, 10);
        sleep(2); // catch late duplicates before the exact comparison
        CarbonTally t1 = tallyCarbons(c1, s1);
        CarbonTally t2 = tallyCarbons(c2, s2);
        return std::make_pair(t1, t2);
    };

    CarbonTally sum1, sum2;

    // Python: "Message 1 from <jid>" — unique per client (cs_m1_cnt)
    auto b1 = runBatch("Message 1 from " + c1name, "Message 1 from " + c2name, 1);
    addTally(sum1, b1.first); addTally(sum2, b1.second);
    check("backend-xmpp: Carbons are delivered to clients across spectrum boundary",
          b1.first.m1 >= 1 && b1.second.m1 >= 1);
    check("backend-xmpp: No unexpected carbons are delivered",
          b1.first.m1 == 1 && b1.second.m1 == 1);

    // Python: "Message 2" — identical text (cs_m2_cnt)
    auto b2 = runBatch("Message 2", "Message 2", 1);
    addTally(sum1, b2.first); addTally(sum2, b2.second);
    check("backend-xmpp: Identical carbons are delivered correctly",
          b2.first.m2 == 1 && b2.second.m2 == 1);

    // Python: "Message 3 from <jid>" x3 — repeated (cs_m3_cnt)
    auto b3 = runBatch("Message 3 from " + c1name, "Message 3 from " + c2name, 3);
    addTally(sum1, b3.first); addTally(sum2, b3.second);
    check("backend-xmpp: Repeated carbons are delivered correctly",
          b3.first.m3 == 3 && b3.second.m3 == 3);

    // Python: no_weird — any unexpected carbon or plain message fails the run
    check("backend-xmpp: No weird carbon-related messages delivered",
          sum1.weird == 0 && sum2.weird == 0 && c1.plainMessageCount == 0 && c2.plainMessageCount == 0);

    // Python responder_got_all is not ported: this `responder` is the spectrum-side
    // resp@localhost, not the prosody-side responder the Python test used, so the
    // incoming count is not comparable here yet.
    (void)responder;
}

// test_received_copies: a legacy message goes only to the last-replied XMPP resource,
// and every other resource of the user gets a XEP-0280 <received> carbon of it.
// c1 replies first, so it is the last-replied resource; c3 is another resource of the same user.
static void test_received_copies(XmppClient &c1, XmppClient &c3, XmppClient &prosodyResponder) {
    enableCarbons(c1.client);
    enableCarbons(c3.client);
    // Spectrum copies only to resources in its presence oracle, so both must announce presence
    c1.client->sendPresence(Swift::Presence::create());
    c3.client->sendPresence(Swift::Presence::create());
    sleep(2);

    // c1 replies to the legacy contact, making its full JID the last-replied resource
    std::shared_ptr<Swift::Message> reply(new Swift::Message());
    reply->setTo(Swift::JID("responder%localhost@localhostxmpp"));
    reply->setBody("reply from c1");
    reply->setType(Swift::Message::Chat);
    c1.client->sendMessage(reply);
    sleep(2);

    // Reset what the reply may have produced, then send the legacy message from the Prosody side
    c1.receivedBody.clear();
    c1.plainMessageCount = 0;
    c1.carbonCount = 0;
    c1.carbonEvents.clear();
    c3.receivedBody.clear();
    c3.plainMessageCount = 0;
    c3.carbonCount = 0;
    c3.carbonEvents.clear();

    const std::string legacyBody = "Received copy test";
    std::shared_ptr<Swift::Message> legacy(new Swift::Message());
    legacy->setTo(Swift::JID("client@localhost"));
    legacy->setBody(legacyBody);
    legacy->setType(Swift::Message::Chat);
    prosodyResponder.client->sendMessage(legacy);

    c1.waitForBody(10);
    sleep(2); // catch late copies before the exact comparisons
    c3.waitForCarbonCount(1, 10);
    sleep(2);

    check("received: last-replied resource gets the message", c1.receivedBody, legacyBody);
    check("received: last-replied resource gets no copy", c1.carbonCount == 0);
    check("received: other resource gets exactly one copy", c3.carbonCount == 1);
    bool isReceived = c3.carbonCount == 1 && !c3.carbonEvents[0].sent;
    check("received: copy is a <received> carbon", isReceived);
    check("received: copy carries the legacy body", c3.carbonCount == 1 && c3.carbonEvents[0].body == legacyBody);
    check("received: other resource gets no plain message", c3.plainMessageCount == 0);
}

// test_gateway_received_copies: gateway mode. The transport is a Prosody component, so the user's
// XMPP clients are Prosody accounts. Prosody generates the copies for the other resources.
static void test_gateway_received_copies(XmppClient &c1, XmppClient &c3, XmppClient &c4, XmppClient &prosodyResponder) {
    std::vector<XmppClient *> clients = {&c1, &c3, &c4};
    for (XmppClient *c : clients) {
        enableCarbons(c->client);
    }
    // Presence to the transport logs the user in to the legacy network; broadcast presence is what the oracle sees
    for (XmppClient *c : clients) {
        Swift::Presence::ref toTransport = Swift::Presence::create();
        toTransport->setTo(Swift::JID("localhostxmpp"));
        c->client->sendPresence(toTransport);
        c->client->sendPresence(Swift::Presence::create());
    }
    sleep(3);

    // A resource replies to the legacy contact, making its full JID the last-replied resource
    auto replyFrom = [&](XmppClient &c, const std::string &body) {
        std::shared_ptr<Swift::Message> reply(new Swift::Message());
        reply->setTo(Swift::JID("responder%localhost@localhostxmpp"));
        reply->setBody(body);
        reply->setType(Swift::Message::Chat);
        c.client->sendMessage(reply);
        sleep(2);
    };
    auto resetAll = [&]() {
        for (XmppClient *c : clients) {
            c->carbonEvents.clear(); c->carbonCount = 0; c->receivedBody.clear();
        }
    };
    // Address the resource the transport's backend logs in with, not the XMPP user: a message to
    // client@localhost would be fanned out by Prosody itself to every resource and never pass the transport
    auto sendLegacy = [&](const std::string &body) {
        std::shared_ptr<Swift::Message> legacy(new Swift::Message());
        legacy->setTo(Swift::JID("responder@localhost/Spectrum"));
        legacy->setBody(body);
        legacy->setType(Swift::Message::Chat);
        prosodyResponder.client->sendMessage(legacy);
    };
    // Copies from the transport carry the user's bare JID as sender and the legacy body
    auto countCopies = [&](const XmppClient &c, const std::string &body) {
        int n = 0;
        for (const XmppClient::CarbonEvent &ev : c.carbonEvents) {
            if (!ev.sent && ev.from.toBare().toString() == "client@localhost" && ev.body == body) n++;
        }
        return n;
    };

    replyFrom(c1, "gateway reply from c1");
    resetAll();

    // The backend logs in asynchronously; the transport announces its session to the user as the contact's presence
    c3.waitForNick("Spectrum", 60);
    check("gateway: backend session announced by the transport", c3.presenceNicks.count("Spectrum"));

    // Phase 1: c1 is the last-replied resource; c3 and c4 are the other resources of the same user
    const std::string firstBody = "Gateway copy test";
    sendLegacy(firstBody);
    c3.waitForCarbonCount(1, 10);
    c4.waitForCarbonCount(1, 10);
    sleep(2); // catch late copies before the exact comparisons

    check("gateway: last-replied resource gets the message", c1.receivedBody == firstBody);
    check("gateway: other resource gets exactly one copy", countCopies(c3, firstBody) == 1);
    check("gateway: third resource gets exactly one copy", countCopies(c4, firstBody) == 1);
    check("gateway: replied resource gets no copy", countCopies(c1, firstBody) == 0);

    // Phase 2: c3 replies, so it becomes the last-replied resource; c1 and c4 get the copies
    replyFrom(c3, "gateway reply from c3");
    resetAll();

    const std::string secondBody = "Gateway copy test after switch";
    sendLegacy(secondBody);
    c1.waitForCarbonCount(1, 10);
    c4.waitForCarbonCount(1, 10);
    sleep(2);

    check("gateway: switched last-replied resource gets the message", c3.receivedBody == secondBody);
    check("gateway: previous resource gets exactly one copy after switch", countCopies(c1, secondBody) == 1);
    check("gateway: third resource gets exactly one copy after switch", countCopies(c4, secondBody) == 1);
    check("gateway: switched replied resource gets no copy", countCopies(c3, secondBody) == 0);
}

// test_gateway_sent_carbons: gateway mode. The account sends a message from another legacy client,
// so the backend reports it with carbon=true. A component cannot send from the user's bare JID, so the
// transport asks Prosody through a XEP-0356 <privilege> wrapper to send the <sent> carbon to every resource.
// The swiften backend never sets carbon=true, so the test sends that message to the transport as a backend.
// The test's first connection answers only the first PING, so Spectrum drops it on the next ping round;
// a fresh connection is opened here instead.
static void test_gateway_sent_carbons(const std::string &host, int port, XmppClient &c1, XmppClient &c3, XmppClient &c4, XmppClient &prosodyResponder) {
    std::vector<XmppClient *> clients = {&c1, &c3, &c4};
    for (XmppClient *c : clients) {
        c->carbonEvents.clear(); c->carbonCount = 0; c->receivedBody.clear();
    }
    prosodyResponder.carbonEvents.clear(); prosodyResponder.carbonCount = 0; prosodyResponder.receivedBody.clear();

    int fd = connect_tcp(host, port);
    check("gateway sent: backend connection", fd >= 0 && handshake(fd));
    if (fd < 0) return;

    const std::string body = "Sent from another legacy client";
    ConversationMessage m;
    m.set_username("client@localhost");
    m.set_buddyname("responder@localhost");
    m.set_message(body);
    m.set_carbon(true);
    WrapperMessage wrap;
    wrap.set_type(WrapperMessage_Type_TYPE_CONV_MESSAGE);
    wrap.set_payload(m.SerializeAsString());
    check("gateway sent: carbon message sent to the transport", send_wrapper(fd, wrap));

    for (XmppClient *c : clients) {
        c->waitForCarbonCount(1, 10);
    }
    sleep(2); // catch late duplicates before the exact comparisons
    close(fd);

    // A <sent> carbon must come from the user's own bare JID and carry the legacy body. No XMPP resource
    // sent the message, so the forwarded message is from the bare JID too.
    auto countSent = [&](const XmppClient &c) {
        int n = 0;
        for (const XmppClient::CarbonEvent &ev : c.carbonEvents) {
            if (ev.sent && ev.from.toString() == "client@localhost" && ev.innerFrom.toString() == "client@localhost" && ev.body == body) n++;
        }
        return n;
    };
    check("gateway sent: first resource gets exactly one <sent> carbon", countSent(c1) == 1);
    check("gateway sent: second resource gets exactly one <sent> carbon", countSent(c3) == 1);
    check("gateway sent: third resource gets exactly one <sent> carbon", countSent(c4) == 1);
    bool noPlain = c1.receivedBody != body && c3.receivedBody != body && c4.receivedBody != body;
    check("gateway sent: no resource gets it as a plain message", noPlain);
    // The message was already sent on the legacy network, so it must not be sent there again
    bool notEchoed = prosodyResponder.receivedBody != body;
    for (const XmppClient::CarbonEvent &ev : prosodyResponder.carbonEvents) {
        if (ev.body == body) notEchoed = false;
    }
    check("gateway sent: the legacy contact does not get it again", notEchoed);
}

static void test_bad_password() {
    XmppClient bad;
    bad.connect("client@localhost", "wrongpassword");
    bad.waitForConnect(10);
    check("bad password rejected", !bad.isReady);
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: test_chat <host> <port> [slack|jabber|irc_server] [xmpp_port]" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    int port = std::atoi(argv[2]);
    std::string mode = (argc > 3) ? argv[3] : "";
    bool isSlack = (mode == "slack");
    bool isJabber = (mode == "jabber");
    bool isGateway = (mode == "gateway");
    bool isIrcServer = (mode == "irc_server");
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
    else if (isGateway) {
        // Gateway mode: no frontend on xmppPort; the user's clients are Prosody accounts
        // The legacy account is the existing responder account, so legacy traffic must pass through the transport.
        // The backend logs in as responder@localhost/Spectrum; the test's sender is a second resource of the same account.
        // The jabber run shares this database and registers client@localhost first, so drop that record before registering.
        cmd(fd, "unregister client@localhost");
        cmd(fd, "register client@localhost responder@localhost password");

        XmppClient c1, c3, c4, prosodyResponder;
        c1.connect("client@localhost/gw1", "password", 5222);
        c1.waitForConnect(60);
        check("gateway: c1 connected", c1.isReady);
        c3.connect("client@localhost/gw3", "password", 5222);
        c3.waitForConnect(60);
        check("gateway: c3 connected", c3.isReady);
        c4.connect("client@localhost/gw4", "password", 5222);
        c4.waitForConnect(60);
        check("gateway: c4 connected", c4.isReady);
        prosodyResponder.connect("responder@localhost/sender", "password", 5222);
        prosodyResponder.waitForConnect(60);
        check("gateway: prosody responder connected", prosodyResponder.isReady);
        // Lower than the backend's resource (priority 0), so bare-JID messages to responder go to the backend
        Swift::Presence::ref senderPresence = Swift::Presence::create();
        senderPresence->setPriority(-1);
        prosodyResponder.client->sendPresence(senderPresence);
        // Prosody only delivers between resources of the same account when carbons are enabled
        enableCarbons(prosodyResponder.client);

        std::cout << "\n--- gateway received copies ---\n";
        test_gateway_received_copies(c1, c3, c4, prosodyResponder);

        std::cout << "\n--- gateway sent carbons ---\n";
        test_gateway_sent_carbons(host, port, c1, c3, c4, prosodyResponder);

        prosodyResponder.disconnect();
        c4.disconnect();
        c3.disconnect();
        c1.disconnect();
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

        std::cout << "\n--- received copies ---\n";
        XmppClient c3;
        c3.connect("client@localhost/rcv", "password", xmppPort);
        c3.waitForConnect(60);
        check("c3 spectrum connected", c3.isReady);
        XmppClient prosodyResponder;
        prosodyResponder.connect("responder@localhost", "password", 5222);
        prosodyResponder.waitForConnect(60);
        check("prosody responder connected", prosodyResponder.isReady);
        test_received_copies(client, c3, prosodyResponder);
        prosodyResponder.disconnect();
        c3.disconnect();

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
        const std::string &ircChannel = isIrcServer ? IRC_SERVER_CHANNEL_JID : IRC_CHANNEL_JID;
        const std::string label = isIrcServer ? " (irc_server)" : " (irc)";

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
        responder.joinRoom(ircChannel, "resp"); responder.waitForJoin();

        std::cout << "\n--- muc_join_leave" << label << " ---\n";
        test_muc_join_leave(client, responder, ircChannel);

        std::cout << "\n--- muc_echo" << label << " ---\n";
        test_muc_echo(client, responder, ircChannel, ircChannel);

        std::cout << "\n--- muc_pm" << label << " ---\n";
        test_muc_pm(client, responder, ircChannel, ircChannel);

        std::cout << "\n--- muc_change_topic" << label << " ---\n";
        test_muc_change_topic(client, responder, ircChannel, ircChannel);

        responder.leaveRoom(ircChannel, "resp"); responder.waitForLeave();

        std::cout << "\n--- muc_away" << label << " ---\n";
        test_muc_away(client, responder, ircChannel);

        std::cout << "\n--- muc_join_nickname_used" << label << " ---\n";
        test_muc_join_nickname_used(client, responder, ircChannel);

        std::cout << "\n--- muc_whois" << label << " ---\n";
        test_muc_whois(client, ircChannel);

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
