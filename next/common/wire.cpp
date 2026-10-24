#include "wire.h"

#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace next {

namespace {

SOCKET asSocket(Socket s) { return static_cast<SOCKET>(s); }

bool sendAll(Socket s, const char *data, size_t len) {
	while (len > 0) {
#ifdef _WIN32
		int n = ::send(asSocket(s), data, static_cast<int>(len), 0);
#else
		ssize_t n = ::send(static_cast<int>(s), data, len, 0);
#endif
		if (n <= 0) {
			return false;
		}
		data += n;
		len -= static_cast<size_t>(n);
	}
	return true;
}

bool recvAll(Socket s, char *data, size_t len) {
	while (len > 0) {
#ifdef _WIN32
		int n = ::recv(asSocket(s), data, static_cast<int>(len), 0);
#else
		ssize_t n = ::recv(static_cast<int>(s), data, len, 0);
#endif
		if (n <= 0) {
			return false;
		}
		data += n;
		len -= static_cast<size_t>(n);
	}
	return true;
}

}  // namespace

bool initSockets() {
#ifdef _WIN32
	WSADATA data;
	return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
	return true;
#endif
}

Socket listenOn(int port, const std::string &host) {
	SOCKET fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == INVALID_SOCKET) {
		return -1;
	}
	int yes = 1;
	::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes), sizeof(yes));

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(static_cast<uint16_t>(port));
	if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
		::closesocket(fd);
		return -1;
	}
	if (::bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0 || ::listen(fd, 16) != 0) {
		::closesocket(fd);
		return -1;
	}
	return static_cast<Socket>(fd);
}

Socket connectTo(const std::string &host, int port) {
	addrinfo hints{};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	addrinfo *res = nullptr;
	if (::getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0 || !res) {
		return -1;
	}
	SOCKET fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (fd == INVALID_SOCKET || ::connect(fd, res->ai_addr, static_cast<int>(res->ai_addrlen)) != 0) {
		if (fd != INVALID_SOCKET) {
			::closesocket(fd);
		}
		::freeaddrinfo(res);
		return -1;
	}
	::freeaddrinfo(res);
	return static_cast<Socket>(fd);
}

Socket acceptOn(Socket listener) {
	SOCKET fd = ::accept(asSocket(listener), nullptr, nullptr);
	return fd == INVALID_SOCKET ? -1 : static_cast<Socket>(fd);
}

void closeSocket(Socket s) {
	if (s < 0) {
		return;
	}
#ifdef _WIN32
	::closesocket(asSocket(s));
#else
	::close(static_cast<int>(s));
#endif
}

bool sendFrame(Socket s, const pbnetwork::WrapperMessage &msg) {
	std::string body;
	if (!msg.SerializeToString(&body)) {
		return false;
	}
	uint32_t len = htonl(static_cast<uint32_t>(body.size()));
	std::string frame(reinterpret_cast<const char *>(&len), 4);
	frame += body;
	return sendAll(s, frame.data(), frame.size());
}

bool sendRaw(Socket s, const std::string &data) {
	return sendAll(s, data.data(), data.size());
}

int recvSome(Socket s, char *buf, size_t len) {
#ifdef _WIN32
	int n = ::recv(asSocket(s), buf, static_cast<int>(len), 0);
#else
	ssize_t n = ::recv(static_cast<int>(s), buf, len, 0);
#endif
	return static_cast<int>(n);
}

bool recvFrame(Socket s, pbnetwork::WrapperMessage &msg) {
	uint32_t len = 0;
	if (!recvAll(s, reinterpret_cast<char *>(&len), 4)) {
		return false;
	}
	len = ntohl(len);
	std::string body(len, '\0');
	if (len > 0 && !recvAll(s, &body[0], len)) {
		return false;
	}
	return msg.ParseFromString(body);
}

}  // namespace next
