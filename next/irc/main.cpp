// next IRC backend: connects to the next frontend as its backend and bridges
// each logged-in user to an IRC server (one IRC connection per user).
//
//   spectrum_next_irc [frontend_host=127.0.0.1] [frontend_port=5231] [irc_host=127.0.0.1] [irc_port=6667]
//
// Login.legacyName is "nick@server[:port]" (as in the libpurple IRC backend); a bare nick
// uses the default IRC server given on the command line. Password is ignored (first test).
// ConversationMessage.buddyName is the IRC target: a nick or a #channel.
// Channels are joined on the first message sent to them.

#include <cstdlib>
#include <csignal>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "wire.h"

using namespace next;
using pbnetwork::WrapperMessage;

namespace {

std::string frontendHost = "127.0.0.1";
int frontendPort = 5231;
std::string ircHost = "127.0.0.1";
int ircPort = 6667;

// The one connection to the frontend.
struct Link {
	Socket fd;
	std::mutex writeLock;
	bool send(const WrapperMessage &msg) {
		std::lock_guard<std::mutex> lock(writeLock);
		return sendFrame(fd, msg);
	}
};
std::shared_ptr<Link> link;

void toFrontend(const WrapperMessage &msg) {
	if (link) link->send(msg);
}

// One IRC connection for one user.
struct Session {
	std::string user;
	std::string nick;
	Socket fd = -1;
	std::mutex writeLock;
	// Channels the server has confirmed we joined (JOIN echoed back by the server).
	std::set<std::string> joined;
	// Messages waiting for a channel join to be confirmed, keyed by channel.
	std::mutex stateLock;
	std::map<std::string, std::vector<std::string>> pending;

	void sendLine(const std::string &line) {
		std::lock_guard<std::mutex> lock(writeLock);
		std::cerr << "> " << user << ": " << line << "\n";
		sendRaw(fd, line + "\r\n");
	}
};

std::mutex sessionsLock;
std::map<std::string, std::shared_ptr<Session>> sessions;

std::shared_ptr<Session> sessionFor(const std::string &user) {
	std::lock_guard<std::mutex> lock(sessionsLock);
	auto it = sessions.find(user);
	return it == sessions.end() ? nullptr : it->second;
}

// Splits "a b c :rest" style IRC line into tokens; trailing text after " :" is one token.
std::vector<std::string> splitIrc(const std::string &line) {
	std::vector<std::string> out;
	size_t pos = 0;
	while (pos < line.size()) {
		if (line[pos] == ':' && !out.empty()) {
			out.push_back(line.substr(pos + 1));
			break;
		}
		size_t sp = line.find(' ', pos);
		if (sp == std::string::npos) sp = line.size();
		if (sp > pos) out.push_back(line.substr(pos, sp - pos));
		pos = sp + 1;
	}
	return out;
}

std::string nickOf(const std::string &prefix) {
	return prefix.substr(0, prefix.find('!'));
}

void sendConnected(const std::string &user) {
	pbnetwork::Connected c;
	c.set_user(user);
	toFrontend(makeWrapper(WrapperMessage::TYPE_CONNECTED, c));
}

void sendDisconnected(const std::string &user, int error, const std::string &message) {
	pbnetwork::Disconnected d;
	d.set_user(user);
	d.set_error(error);
	d.set_message(message);
	toFrontend(makeWrapper(WrapperMessage::TYPE_DISCONNECTED, d));
}

void sendIncoming(const std::string &user, const std::string &buddy, const std::string &sender, const std::string &text) {
	pbnetwork::ConversationMessage m;
	m.set_username(user);
	m.set_buddyname(buddy);
	m.set_nickname(sender);
	m.set_message(text);
	toFrontend(makeWrapper(WrapperMessage::TYPE_CONV_MESSAGE, m));
}

// Reads IRC lines from the session's socket until it closes.
void readIrc(std::shared_ptr<Session> s) {
	std::string buf;
	char chunk[1024];
	bool registered = false;
	for (;;) {
		int n = recvSome(s->fd, chunk, sizeof(chunk));
		if (n <= 0) break;
		buf.append(chunk, n);
		size_t eol;
		while ((eol = buf.find('\n')) != std::string::npos) {
			std::string line = buf.substr(0, eol);
			buf.erase(0, eol + 1);
			if (!line.empty() && line.back() == '\r') line.pop_back();
			std::cerr << "< " << s->user << ": " << line << "\n";
			if (line.empty()) continue;

			std::string prefix;
			if (line[0] == ':') {
				size_t sp = line.find(' ');
				prefix = line.substr(1, sp - 1);
				line = sp == std::string::npos ? "" : line.substr(sp + 1);
			}
			auto t = splitIrc(line);
			if (t.empty()) continue;
			const std::string &cmd = t[0];

			if (cmd == "JOIN" && t.size() >= 2 && nickOf(prefix) == s->nick) {
				// Server confirmed our join: send what was queued for this channel.
				std::vector<std::string> queued;
				{
					std::lock_guard<std::mutex> lock(s->stateLock);
					s->joined.insert(t[1]);
					queued = s->pending[t[1]];
					s->pending.erase(t[1]);
				}
				for (const auto &text : queued) {
					s->sendLine("PRIVMSG " + t[1] + " :" + text);
				}
			} else if (cmd == "PING") {
				s->sendLine("PONG :" + (t.size() > 1 ? t[1] : ""));
			} else if (cmd == "001" && !registered) {
				registered = true;
				sendConnected(s->user);
			} else if (cmd == "PRIVMSG" && t.size() >= 3) {
				std::string sender = nickOf(prefix);
				// A message to our nick comes from the sender; to a channel it is filed under the channel.
				std::string buddy = (t[1] == s->nick) ? sender : t[1];
				sendIncoming(s->user, buddy, sender, t[2]);
			} else if (cmd == "433") {  // nick in use
				sendDisconnected(s->user, 2, "nickname in use");
				break;
			} else if (cmd == "ERROR") {
				break;
			}
		}
	}
	sendDisconnected(s->user, 0, "IRC connection closed");
	std::lock_guard<std::mutex> lock(sessionsLock);
	auto it = sessions.find(s->user);
	if (it != sessions.end() && it->second == s) sessions.erase(it);
	closeSocket(s->fd);
}

// Splits Login.legacyName "nick@host[:port]" into its parts; without "@host" the
// default IRC server from the command line is used.
void parseLegacyName(const std::string &legacyName, std::string &nick, std::string &host, int &port) {
	nick = legacyName;
	host = ircHost;
	port = ircPort;
	size_t at = legacyName.find('@');
	if (at == std::string::npos) return;
	nick = legacyName.substr(0, at);
	host = legacyName.substr(at + 1);
	size_t colon = host.rfind(':');
	if (colon != std::string::npos) {
		port = std::atoi(host.c_str() + colon + 1);
		host.erase(colon);
	}
}

void login(const pbnetwork::Login &login) {
	std::string nick, host;
	int port;
	parseLegacyName(login.legacyname(), nick, host, port);
	if (nick.empty() || host.empty() || port <= 0) {
		sendDisconnected(login.user(), 1, "bad IRC account " + login.legacyname() + ", expected nick@server[:port]");
		return;
	}
	std::cerr << login.user() << ": connecting to " << host << ":" << port << " as " << nick << "\n";
	Socket fd = connectTo(host, port);
	if (fd < 0) {
		sendDisconnected(login.user(), 1, "cannot reach IRC server " + host + ":" + std::to_string(port));
		return;
	}
	auto s = std::make_shared<Session>();
	s->user = login.user();
	s->nick = nick;
	// Libera (and most IRC networks) limit nicks to 16 characters.
	constexpr size_t kMaxNick = 16;
	if (s->nick.size() > kMaxNick) s->nick.resize(kMaxNick);
	s->fd = fd;
	{
		std::lock_guard<std::mutex> lock(sessionsLock);
		auto old = sessions.find(s->user);
		if (old != sessions.end()) {
			old->second->sendLine("QUIT :replaced");
			closeSocket(old->second->fd);
		}
		sessions[s->user] = s;
	}
	s->sendLine("NICK " + s->nick);
	s->sendLine("USER " + s->nick + " 0 * :" + s->nick);
	std::thread(readIrc, s).detach();
}

void logout(const std::string &user) {
	auto s = sessionFor(user);
	if (!s) return;
	s->sendLine("QUIT :bye");
	closeSocket(s->fd);
}

void sendOut(const pbnetwork::ConversationMessage &m) {
	auto s = sessionFor(m.username());
	if (!s) {
		sendDisconnected(m.username(), 1, "not logged in");
		return;
	}
	const std::string &target = m.buddyname();
	bool isChannel = !target.empty() && target[0] == '#';
	std::lock_guard<std::mutex> lock(s->stateLock);
	if (isChannel && !s->joined.count(target)) {
		// Not joined yet: queue the message, and send JOIN once per channel.
		bool firstForChannel = !s->pending.count(target);
		s->pending[target].push_back(m.message());
		if (firstForChannel) {
			s->sendLine("JOIN " + target);
		}
		return;
	}
	s->sendLine("PRIVMSG " + target + " :" + m.message());
}

// Sends QUIT on every IRC connection and closes it. Called on shutdown.
void quitAll() {
	std::lock_guard<std::mutex> lock(sessionsLock);
	for (auto &entry : sessions) {
		entry.second->sendLine("QUIT :shutdown");
		closeSocket(entry.second->fd);
	}
	sessions.clear();
}

}  // namespace

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
BOOL WINAPI onConsoleEvent(DWORD event) {
	if (event == CTRL_C_EVENT || event == CTRL_CLOSE_EVENT || event == CTRL_BREAK_EVENT) {
		quitAll();
		ExitProcess(0);
	}
	return FALSE;
}
#endif

int main(int argc, char **argv) {
#ifdef _WIN32
	SetConsoleCtrlHandler(onConsoleEvent, TRUE);
#else
	// Not async-signal-safe in general; acceptable for this test backend.
	std::signal(SIGINT, [](int) { quitAll(); std::_Exit(0); });
	std::signal(SIGTERM, [](int) { quitAll(); std::_Exit(0); });
#endif
	if (argc > 1) frontendHost = argv[1];
	if (argc > 2) frontendPort = std::atoi(argv[2]);
	if (argc > 3) ircHost = argv[3];
	if (argc > 4) ircPort = std::atoi(argv[4]);

	if (!initSockets()) {
		std::cerr << "socket init failed\n";
		return 1;
	}
	Socket fd = connectTo(frontendHost, frontendPort);
	if (fd < 0) {
		std::cerr << "cannot connect to frontend " << frontendHost << ":" << frontendPort << "\n";
		return 1;
	}
	link = std::make_shared<Link>();
	link->fd = fd;
	std::cerr << "connected to frontend, IRC server " << ircHost << ":" << ircPort << "\n";

	WrapperMessage msg;
	while (recvFrame(fd, msg)) {
		switch (msg.type()) {
			case WrapperMessage::TYPE_LOGIN: {
				pbnetwork::Login l;
				if (l.ParseFromString(msg.payload())) login(l);
				break;
			}
			case WrapperMessage::TYPE_LOGOUT: {
				pbnetwork::Logout l;
				if (l.ParseFromString(msg.payload())) logout(l.user());
				break;
			}
			case WrapperMessage::TYPE_CONV_MESSAGE: {
				pbnetwork::ConversationMessage m;
				if (m.ParseFromString(msg.payload())) sendOut(m);
				break;
			}
			case WrapperMessage::TYPE_PING:
				toFrontend(makeWrapper(WrapperMessage::TYPE_PONG, pbnetwork::Logout()));
				break;
			default:
				break;
		}
	}
	std::cerr << "frontend closed\n";
	quitAll();
	return 0;
}
