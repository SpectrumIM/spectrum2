// Minimal wire layer for the "next" subproject: blocking TCP sockets and
// length-prefixed pbnetwork::WrapperMessage frames. Depends only on protocol.proto.
#pragma once

#include <cstdint>
#include <string>

#include "protocol.pb.h"

namespace next {

using Socket = std::intptr_t;

// Creates the sockets layer (WSAStartup on Windows). Call once at startup.
bool initSockets();

// Listens on 0.0.0.0:port. Returns -1 on error.
Socket listenOn(int port);

// Connects to host:port. Returns -1 on error.
Socket connectTo(const std::string &host, int port);

// Accepts one connection from a listening socket. Returns -1 on error.
Socket acceptOn(Socket listener);

void closeSocket(Socket s);

// Frame = 4-byte big-endian length + serialized WrapperMessage.
// sendFrame is not thread-safe per socket; callers hold a mutex.
bool sendFrame(Socket s, const pbnetwork::WrapperMessage &msg);
bool recvFrame(Socket s, pbnetwork::WrapperMessage &msg);

// Raw bytes (used by the IRC backend, which speaks text on its own socket).
bool sendRaw(Socket s, const std::string &data);
// Returns bytes read, 0 on close, -1 on error.
int recvSome(Socket s, char *buf, size_t len);

// Builds a WrapperMessage of the given type with a serialized payload.
template <typename T>
pbnetwork::WrapperMessage makeWrapper(pbnetwork::WrapperMessage_Type type, const T &payload) {
	pbnetwork::WrapperMessage wrap;
	wrap.set_type(type);
	wrap.set_payload(payload.SerializeAsString());
	return wrap;
}

}  // namespace next
