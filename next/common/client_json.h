// JSON envelope for client sockets. One JSON object per line:
//   {"type": "TYPE_LOGIN", "payload": {"user": "alice", "legacyName": "alice", "password": "x"}}
// "type" is the pbnetwork::WrapperMessage_Type name. "payload" is the protobuf JSON
// (field names from protocol.proto) of the matching message.
#pragma once

#include <string>

#include "protocol.pb.h"

namespace next {

// Parses one line into a WrapperMessage (binary payload). Returns false on bad input.
bool decodeClientLine(const std::string &line, pbnetwork::WrapperMessage &out, std::string &error);

// Encodes a WrapperMessage as one line (no trailing newline).
bool encodeClientLine(const pbnetwork::WrapperMessage &msg, std::string &line);

}  // namespace next
