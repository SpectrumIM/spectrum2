// next frontend: serves clients over HTTP (behind nginx for HTTPS) and talks to one backend.
//
//   spectrum_next_frontend [http_port=5230] [backend_port=5231] [http_host=127.0.0.1]
//
// HTTP API, JSON bodies (see client_json.h for the {"type", "payload"} envelope):
//   POST /api/login   {"legacyName": "nick@irc.libera.chat", "password": ""}
//                     -> {"session": "<token>"}; later requests send "Authorization: Bearer <token>"
//   POST /api/send    {"type": "TYPE_CONV_MESSAGE" | "TYPE_JOIN_ROOM" | "TYPE_LEAVE_ROOM", "payload": {...}}
//   GET  /api/events  server-sent events, one envelope per event with "id: N"; reconnecting with
//                     Last-Event-ID: N (or ?since=N+1) resends what came after N. The stream
//                     closes when the session ends.
//   POST /api/logout
// The user is the legacy name; payload user fields are set from the session. A session with no
// open event stream for 90 s is logged out at the backend. nginx needs proxy_buffering off
// (the stream sends X-Accel-Buffering: no) and a proxy_read_timeout above the 15 s keepalive.
//
// The backend link is length-prefixed WrapperMessage frames: Login, Logout, ConversationMessage,
// JoinRoom, LeaveRoom go out; Connected, Disconnected, ConversationMessage, ParticipantChanged,
// Ping come in.

#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <json/json.h>

#include "wire.h"
#include "client_json.h"

using namespace next;
using pbnetwork::WrapperMessage;
using Clock = std::chrono::steady_clock;

namespace {

constexpr auto kKeepAlive = std::chrono::seconds(15);
constexpr auto kSessionTimeout = std::chrono::seconds(90);
constexpr size_t kMaxQueued = 2000;
constexpr size_t kMaxBody = 1 << 20;

// One logged-in user and the events waiting for its client.
struct Session {
	std::string id;
	std::string user;
	std::string legacyName;
	std::mutex lock;
	std::condition_variable changed;
	std::deque<std::pair<uint64_t, std::string>> events;  // (sequence, envelope JSON)
	uint64_t nextSeq = 1;
	Clock::time_point lastSeen = Clock::now();
	int polling = 0;
	bool ended = false;  // logged out, replaced or disconnected: no more events

	void push(const WrapperMessage &msg) {
		std::string json;
		if (!encodeClientLine(msg, json)) return;
		std::lock_guard<std::mutex> l(lock);
		if (ended) return;
		events.emplace_back(nextSeq++, json);
		if (events.size() > kMaxQueued) events.pop_front();
		changed.notify_all();
	}

	void end() {
		std::lock_guard<std::mutex> l(lock);
		ended = true;
		changed.notify_all();
	}
};

struct Backend {
	Socket fd;
	std::mutex writeLock;
	bool send(const WrapperMessage &msg) {
		std::lock_guard<std::mutex> lock(writeLock);
		return sendFrame(fd, msg);
	}
};

// PONG has no payload.
WrapperMessage pong() {
	WrapperMessage w;
	w.set_type(WrapperMessage::TYPE_PONG);
	return w;
}

std::mutex stateLock;
std::shared_ptr<Backend> backend;                           // the one connected backend, or null
std::map<std::string, std::shared_ptr<Session>> sessions;   // session id -> session
std::map<std::string, std::shared_ptr<Session>> users;      // user -> its current session

bool toBackend(const WrapperMessage &msg) {
	std::shared_ptr<Backend> b;
	{
		std::lock_guard<std::mutex> lock(stateLock);
		b = backend;
	}
	return b && b->send(msg);
}

void sendToUser(const std::string &user, const WrapperMessage &msg) {
	std::shared_ptr<Session> s;
	{
		std::lock_guard<std::mutex> lock(stateLock);
		auto it = users.find(user);
		if (it != users.end()) s = it->second;
	}
	if (s) s->push(msg);
}

WrapperMessage disconnected(const std::string &user, int error, const std::string &message) {
	pbnetwork::Disconnected d;
	d.set_user(user);
	d.set_error(error);
	d.set_message(message);
	return makeWrapper(WrapperMessage::TYPE_DISCONNECTED, d);
}

std::string newSessionId() {
	static std::mutex lock;
	static std::mt19937_64 rng(std::random_device{}() ^ static_cast<uint64_t>(Clock::now().time_since_epoch().count()));
	std::lock_guard<std::mutex> l(lock);
	std::ostringstream id;
	id << std::hex;
	for (int i = 0; i < 4; ++i) id << rng();
	return id.str();
}

// Removes a session and logs its user out at the backend.
void dropSession(const std::shared_ptr<Session> &s, const char *why) {
	{
		std::lock_guard<std::mutex> lock(stateLock);
		sessions.erase(s->id);
		auto it = users.find(s->user);
		if (it != users.end() && it->second == s) users.erase(it);
	}
	s->end();
	pbnetwork::Logout logout;
	logout.set_user(s->user);
	logout.set_legacyname(s->legacyName);
	toBackend(makeWrapper(WrapperMessage::TYPE_LOGOUT, logout));
	std::cerr << "session of " << s->user << " closed (" << why << ")\n";
}

// ---- HTTP ----

struct Request {
	std::string method;
	std::string path;
	std::map<std::string, std::string> query;
	std::map<std::string, std::string> headers;  // lower-case names
	std::string body;
};

struct Response {
	int status = 200;
	std::string body = "{}";
};

std::string lower(std::string s) {
	for (auto &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return s;
}

std::string trim(const std::string &s) {
	size_t b = s.find_first_not_of(" \t");
	size_t e = s.find_last_not_of(" \t\r");
	return b == std::string::npos ? "" : s.substr(b, e - b + 1);
}

// Reads one request; false when the connection closed or the request is malformed.
bool readRequest(Socket fd, std::string &buf, Request &req) {
	size_t end;
	while ((end = buf.find("\r\n\r\n")) == std::string::npos) {
		if (buf.size() > 64 * 1024) return false;
		char chunk[4096];
		int n = recvSome(fd, chunk, sizeof(chunk));
		if (n <= 0) return false;
		buf.append(chunk, n);
	}
	std::istringstream head(buf.substr(0, end));
	buf.erase(0, end + 4);

	std::string line, target, version;
	if (!std::getline(head, line)) return false;
	std::istringstream first(line);
	if (!(first >> req.method >> target >> version)) return false;

	size_t q = target.find('?');
	req.path = target.substr(0, q);
	if (q != std::string::npos) {
		std::istringstream params(target.substr(q + 1));
		std::string pair;
		while (std::getline(params, pair, '&')) {
			size_t eq = pair.find('=');
			req.query[pair.substr(0, eq)] = eq == std::string::npos ? "" : pair.substr(eq + 1);
		}
	}
	while (std::getline(head, line)) {
		size_t colon = line.find(':');
		if (colon == std::string::npos) continue;
		req.headers[lower(trim(line.substr(0, colon)))] = trim(line.substr(colon + 1));
	}

	size_t length = 0;
	auto cl = req.headers.find("content-length");
	if (cl != req.headers.end()) length = std::strtoul(cl->second.c_str(), nullptr, 10);
	if (length > kMaxBody) return false;
	while (buf.size() < length) {
		char chunk[4096];
		int n = recvSome(fd, chunk, sizeof(chunk));
		if (n <= 0) return false;
		buf.append(chunk, n);
	}
	req.body = buf.substr(0, length);
	buf.erase(0, length);
	return true;
}

const char *reason(int status) {
	switch (status) {
		case 200: return "OK";
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 503: return "Service Unavailable";
		default: return "Error";
	}
}

bool writeResponse(Socket fd, const Response &res, bool keepAlive) {
	std::ostringstream out;
	out << "HTTP/1.1 " << res.status << " " << reason(res.status) << "\r\n"
		<< "Content-Type: application/json; charset=utf-8\r\n"
		<< "Content-Length: " << res.body.size() << "\r\n"
		<< "Cache-Control: no-store\r\n"
		<< "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n\r\n"
		<< res.body;
	return sendRaw(fd, out.str());
}

std::string jsonString(const Json::Value &v) {
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "";
	return Json::writeString(writer, v);
}

Response error(int status, const std::string &message) {
	Json::Value v;
	v["error"] = message;
	return Response{status, jsonString(v)};
}

bool parseJson(const std::string &text, Json::Value &out) {
	Json::CharReaderBuilder builder;
	std::string errs;
	std::istringstream in(text);
	return Json::parseFromStream(builder, in, &out, &errs) && out.isObject();
}

std::shared_ptr<Session> authorize(const Request &req) {
	auto it = req.headers.find("authorization");
	if (it == req.headers.end() || it->second.compare(0, 7, "Bearer ") != 0) return nullptr;
	std::lock_guard<std::mutex> lock(stateLock);
	auto s = sessions.find(it->second.substr(7));
	return s == sessions.end() ? nullptr : s->second;
}

Response handleLogin(const Request &req) {
	Json::Value body;
	if (!parseJson(req.body, body) || !body["legacyName"].isString() || body["legacyName"].asString().empty()) {
		return error(400, "expected {\"legacyName\": ..., \"password\": ...}");
	}
	auto s = std::make_shared<Session>();
	s->id = newSessionId();
	s->legacyName = body["legacyName"].asString();
	s->user = s->legacyName;

	pbnetwork::Login login;
	login.set_user(s->user);
	login.set_legacyname(s->legacyName);
	login.set_password(body.get("password", "").asString());

	std::shared_ptr<Session> replaced;
	{
		std::lock_guard<std::mutex> lock(stateLock);
		if (!backend) return error(503, "no backend connected");
		auto it = users.find(s->user);
		if (it != users.end()) {
			replaced = it->second;
			sessions.erase(replaced->id);
		}
		sessions[s->id] = s;
		users[s->user] = s;
	}
	if (replaced) {
		replaced->push(disconnected(replaced->user, 0, "logged in from another client"));
		replaced->end();
	}
	if (!toBackend(makeWrapper(WrapperMessage::TYPE_LOGIN, login))) {
		dropSession(s, "backend unreachable");
		return error(503, "no backend connected");
	}
	std::cerr << "session of " << s->user << " opened\n";
	Json::Value out;
	out["session"] = s->id;
	return Response{200, jsonString(out)};
}

Response handleSend(const Request &req, const std::shared_ptr<Session> &s) {
	Json::Value body;
	if (!parseJson(req.body, body) || !body["type"].isString()) {
		return error(400, "expected {\"type\": ..., \"payload\": ...}");
	}
	const std::string type = body["type"].asString();
	if (type != "TYPE_CONV_MESSAGE" && type != "TYPE_JOIN_ROOM" && type != "TYPE_LEAVE_ROOM") {
		return error(400, "type " + type + " cannot be sent");
	}
	// The session decides who the message is from
	body["payload"]["userName"] = s->user;

	WrapperMessage msg;
	std::string problem;
	if (!decodeClientLine(jsonString(body), msg, problem)) {
		return error(400, problem);
	}
	if (!toBackend(msg)) {
		return error(503, "no backend connected");
	}
	return Response{};
}

// Streams the session's events until it ends or the client goes away. Each event is
// "id: N\ndata: <envelope>\n\n"; a reconnecting client sends Last-Event-ID (or ?since=N+1)
// and gets everything after it again. Comments keep proxies from timing the stream out.
void streamEvents(Socket fd, const Request &req, const std::shared_ptr<Session> &s) {
	uint64_t since = 0;
	auto last = req.headers.find("last-event-id");
	auto it = req.query.find("since");
	if (last != req.headers.end()) since = std::strtoull(last->second.c_str(), nullptr, 10) + 1;
	else if (it != req.query.end()) since = std::strtoull(it->second.c_str(), nullptr, 10);

	if (!sendRaw(fd, "HTTP/1.1 200 OK\r\n"
	                 "Content-Type: text/event-stream; charset=utf-8\r\n"
	                 "Cache-Control: no-store\r\n"
	                 "X-Accel-Buffering: no\r\n"
	                 "Transfer-Encoding: chunked\r\n"
	                 "Connection: close\r\n\r\n")) {
		return;
	}
	// Chunked, because some HTTP clients read a body without length only when the connection closes
	auto sendChunk = [fd](const std::string &data) {
		std::ostringstream size;
		size << std::hex << data.size() << "\r\n";
		return sendRaw(fd, size.str() + data + "\r\n");
	};

	std::unique_lock<std::mutex> lock(s->lock);
	// Everything before `since` has been received by the client
	while (!s->events.empty() && s->events.front().first < since) s->events.pop_front();
	uint64_t next = since;
	s->polling++;
	for (;;) {
		std::string out;
		for (const auto &e : s->events) {
			if (e.first < next) continue;
			out += "id: " + std::to_string(e.first) + "\ndata: " + e.second + "\n\n";
			next = e.first + 1;
		}
		if (out.empty() && s->ended) break;
		if (out.empty()) out = ": keepalive\n\n";
		lock.unlock();
		bool ok = sendChunk(out);
		lock.lock();
		if (!ok) break;
		s->changed.wait_for(lock, kKeepAlive, [&] { return s->ended || (!s->events.empty() && s->events.back().first >= next); });
	}
	s->polling--;
	s->lastSeen = Clock::now();
	lock.unlock();
	sendRaw(fd, "0\r\n\r\n");
}

Response route(const Request &req) {
	if (req.path == "/api/login") {
		return req.method == "POST" ? handleLogin(req) : error(405, "use POST");
	}
	if (req.path != "/api/send" && req.path != "/api/events" && req.path != "/api/logout") {
		return error(404, "unknown endpoint " + req.path);
	}
	auto s = authorize(req);
	if (!s) return error(401, "unknown or expired session");
	{
		std::lock_guard<std::mutex> lock(s->lock);
		s->lastSeen = Clock::now();
	}
	if (req.method != "POST") return error(405, "use POST");
	if (req.path == "/api/send") return handleSend(req, s);
	dropSession(s, "logout");
	return Response{};
}

void handleHttp(Socket fd) {
	std::string buf;
	Request req;
	while (readRequest(fd, buf, req)) {
		if (req.path == "/api/events" && req.method == "GET") {
			// The stream holds the connection until it ends
			if (auto s = authorize(req)) {
				streamEvents(fd, req, s);
				break;
			}
		}
		Response res = route(req);
		bool keepAlive = lower(req.headers["connection"]) != "close";
		if (!writeResponse(fd, res, keepAlive) || !keepAlive) break;
		req = Request();
	}
	closeSocket(fd);
}

// Logs out sessions whose client stopped polling.
void reapSessions() {
	for (;;) {
		std::this_thread::sleep_for(std::chrono::seconds(10));
		std::vector<std::shared_ptr<Session>> stale;
		{
			std::lock_guard<std::mutex> lock(stateLock);
			for (auto &entry : sessions) {
				std::lock_guard<std::mutex> l(entry.second->lock);
				if (entry.second->polling == 0 && Clock::now() - entry.second->lastSeen > kSessionTimeout) {
					stale.push_back(entry.second);
				}
			}
		}
		for (auto &s : stale) dropSession(s, "not polled");
	}
}

void handleBackend(std::shared_ptr<Backend> b) {
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
				b->send(pong());
				break;
			default:
				std::cerr << "backend: ignoring message type " << msg.type() << "\n";
				break;
		}
	}
	std::cerr << "backend disconnected\n";
	std::vector<std::shared_ptr<Session>> all;
	{
		std::lock_guard<std::mutex> lock(stateLock);
		if (backend == b) backend.reset();
		for (auto &entry : sessions) all.push_back(entry.second);
	}
	// Every user lost its legacy network connection
	for (auto &s : all) s->push(disconnected(s->user, 1, "backend disconnected"));
	closeSocket(b->fd);
}

void acceptBackends(Socket listener) {
	for (;;) {
		Socket fd = acceptOn(listener);
		if (fd < 0) continue;
		auto b = std::make_shared<Backend>();
		b->fd = fd;
		{
			std::lock_guard<std::mutex> lock(stateLock);
			if (backend) {
				std::cerr << "second backend rejected (one backend supported)\n";
				closeSocket(fd);
				continue;
			}
			backend = b;
		}
		std::thread(handleBackend, b).detach();
	}
}

}  // namespace

int main(int argc, char **argv) {
	int httpPort = argc > 1 ? std::atoi(argv[1]) : 5230;
	int backendPort = argc > 2 ? std::atoi(argv[2]) : 5231;
	std::string httpHost = argc > 3 ? argv[3] : "127.0.0.1";

	if (!initSockets()) {
		std::cerr << "socket init failed\n";
		return 1;
	}
	Socket http = listenOn(httpPort, httpHost);
	Socket backends = listenOn(backendPort, "127.0.0.1");
	if (http < 0 || backends < 0) {
		std::cerr << "cannot listen on " << httpHost << ":" << httpPort << " / 127.0.0.1:" << backendPort << "\n";
		return 1;
	}
	std::cerr << "HTTP on " << httpHost << ":" << httpPort << ", backend on 127.0.0.1:" << backendPort << "\n";

	std::thread(acceptBackends, backends).detach();
	std::thread(reapSessions).detach();
	for (;;) {
		Socket fd = acceptOn(http);
		if (fd < 0) continue;
		std::thread(handleHttp, fd).detach();
	}
}
