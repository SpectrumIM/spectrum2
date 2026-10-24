// next frontend: accepts raw protobuf clients and one backend, routes by user.
//
//   spectrum_next_frontend [client_port=5230] [backend_port=5231]
//
// Clients send WrapperMessage frames (Login, Logout, ConversationMessage, JoinRoom, LeaveRoom, Ping).
// The backend sends Connected, Disconnected, ConversationMessage, ParticipantChanged, Ping.
// Login accepts any password (first test). Messages are routed by userName.

#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "wire.h"
#include "client_json.h"

using namespace next;
using pbnetwork::WrapperMessage;

namespace {

struct Client {
	Socket fd;
	std::mutex writeLock;
	// The backend link is binary frames (length-prefixed WrapperMessage), not JSON.
	bool sendBinary(const WrapperMessage &msg) {
		std::lock_guard<std::mutex> lock(writeLock);
		return sendFrame(fd, msg);
	}
	// Clients get one JSON object per line (see client_json.h).
	bool send(const WrapperMessage &msg) {
		std::string line;
		if (!encodeClientLine(msg, line)) return false;
		std::lock_guard<std::mutex> lock(writeLock);
		return sendRaw(fd, line + "\n");
	}
};

// PONG has no payload.
WrapperMessage pong() {
	WrapperMessage w;
	w.set_type(WrapperMessage::TYPE_PONG);
	return w;
}

std::mutex stateLock;
std::shared_ptr<Client> backend;                    // the one connected backend, or null
// A logged-in user: the client that logged it in, and its legacy name (IRC nick),
// which the backend needs again on logout.
struct Route {
	std::shared_ptr<Client> client;
	std::string legacyName;
};
std::map<std::string, Route> routes;  // user -> route

// Forward to the backend. Returns false if no backend is connected.
bool toBackend(const WrapperMessage &msg) {
	std::shared_ptr<Client> b;
	{
		std::lock_guard<std::mutex> lock(stateLock);
		b = backend;
	}
	return b && b->sendBinary(msg);
}

std::shared_ptr<Client> routeFor(const std::string &user) {
	std::lock_guard<std::mutex> lock(stateLock);
	auto it = routes.find(user);
	return it == routes.end() ? nullptr : it->second.client;
}

void sendToUser(const std::string &user, const WrapperMessage &msg) {
	if (auto c = routeFor(user)) {
		c->send(msg);
	}
}

// Reads one '\n'-terminated line. buf keeps bytes received after the line.
bool readLine(Socket fd, std::string &buf, std::string &line) {
	for (;;) {
		size_t eol = buf.find('\n');
		if (eol != std::string::npos) {
			line = buf.substr(0, eol);
			buf.erase(0, eol + 1);
			return true;
		}
		char chunk[1024];
		int n = recvSome(fd, chunk, sizeof(chunk));
		if (n <= 0) return false;
		buf.append(chunk, n);
	}
}

void handleClient(std::shared_ptr<Client> client) {
	std::cerr << "client connected\n";
	WrapperMessage msg;
	std::string buf, line, error;
	while (readLine(client->fd, buf, line)) {
		if (line.empty()) continue;
		if (!decodeClientLine(line, msg, error)) {
			pbnetwork::Disconnected d;
			d.set_user("");
			d.set_error(2);
			d.set_message(error);
			client->send(makeWrapper(WrapperMessage::TYPE_DISCONNECTED, d));
			continue;
		}
		switch (msg.type()) {
			case WrapperMessage::TYPE_LOGIN: {
				pbnetwork::Login login;
				if (!login.ParseFromString(msg.payload())) break;
				{
					std::lock_guard<std::mutex> lock(stateLock);
					routes[login.user()] = Route{client, login.legacyname()};
				}
				if (!toBackend(msg)) {
					pbnetwork::Disconnected d;
					d.set_user(login.user());
					d.set_error(1);
					d.set_message("no backend connected");
					client->send(makeWrapper(WrapperMessage::TYPE_DISCONNECTED, d));
				}
				break;
			}
			case WrapperMessage::TYPE_LOGOUT: {
				pbnetwork::Logout logout;
				if (logout.ParseFromString(msg.payload())) {
					std::lock_guard<std::mutex> lock(stateLock);
					routes.erase(logout.user());
				}
				toBackend(msg);
				break;
			}
			case WrapperMessage::TYPE_PING:
				client->send(pong());
				break;
			default:
				toBackend(msg);
				break;
		}
	}

	// Client gone: log out all of its users at the backend.
	std::vector<std::string> users;
	std::vector<Route> gone;
	{
		std::lock_guard<std::mutex> lock(stateLock);
		for (auto it = routes.begin(); it != routes.end();) {
			if (it->second.client == client) {
				users.push_back(it->first);
				gone.push_back(it->second);
				it = routes.erase(it);
			} else {
				++it;
			}
		}
	}
	for (size_t i = 0; i < users.size(); ++i) {
		const std::string &user = users[i];
		pbnetwork::Logout logout;
		logout.set_user(user);
		logout.set_legacyname(gone[i].legacyName);
		toBackend(makeWrapper(WrapperMessage::TYPE_LOGOUT, logout));
	}
	std::cerr << "client disconnected\n";
	closeSocket(client->fd);
}

void handleBackend(std::shared_ptr<Client> b) {
	std::cerr << "backend connected\n";
	WrapperMessage msg;
	while (recvFrame(b->fd, msg)) {
		switch (msg.type()) {
			case WrapperMessage::TYPE_CONNECTED: {
				pbnetwork::Connected c;
				if (c.ParseFromString(msg.payload())) sendToUser(c.user(), msg);
				break;
			}
			case WrapperMessage::TYPE_DISCONNECTED: {
				pbnetwork::Disconnected d;
				if (d.ParseFromString(msg.payload())) sendToUser(d.user(), msg);
				break;
			}
			case WrapperMessage::TYPE_CONV_MESSAGE: {
				pbnetwork::ConversationMessage m;
				if (m.ParseFromString(msg.payload())) sendToUser(m.username(), msg);
				break;
			}
			case WrapperMessage::TYPE_PARTICIPANT_CHANGED: {
				pbnetwork::Participant p;
				if (p.ParseFromString(msg.payload())) sendToUser(p.username(), msg);
				break;
			}
			case WrapperMessage::TYPE_PING:
				b->sendBinary(pong());
				break;
			default:
				std::cerr << "backend: ignoring message type " << msg.type() << "\n";
				break;
		}
	}
	std::cerr << "backend disconnected\n";
	{
		std::lock_guard<std::mutex> lock(stateLock);
		if (backend == b) backend.reset();
	}
	closeSocket(b->fd);
}

void acceptLoop(Socket listener, bool isBackend) {
	for (;;) {
		Socket fd = acceptOn(listener);
		if (fd < 0) continue;
		auto c = std::make_shared<Client>();
		c->fd = fd;
		if (isBackend) {
			{
				std::lock_guard<std::mutex> lock(stateLock);
				if (backend) {
					std::cerr << "second backend rejected (one backend supported)\n";
					closeSocket(fd);
					continue;
				}
				backend = c;
			}
			std::thread(handleBackend, c).detach();
		} else {
			std::thread(handleClient, c).detach();
		}
	}
}

}  // namespace

int main(int argc, char **argv) {
	int clientPort = argc > 1 ? std::atoi(argv[1]) : 5230;
	int backendPort = argc > 2 ? std::atoi(argv[2]) : 5231;

	if (!initSockets()) {
		std::cerr << "socket init failed\n";
		return 1;
	}
	Socket clients = listenOn(clientPort);
	Socket backends = listenOn(backendPort);
	if (clients < 0 || backends < 0) {
		std::cerr << "cannot listen on " << clientPort << " / " << backendPort << "\n";
		return 1;
	}
	std::cerr << "clients on " << clientPort << ", backend on " << backendPort << "\n";

	std::thread(acceptLoop, backends, true).detach();
	acceptLoop(clients, false);
	return 0;
}
