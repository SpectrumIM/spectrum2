#include "client_json.h"

#include <memory>
#include <sstream>

#include <google/protobuf/util/json_util.h>
#include <json/json.h>

using pbnetwork::WrapperMessage;

namespace next {

namespace {

// Message class carried by a payload of the given type; null if the type has no payload.
std::unique_ptr<google::protobuf::Message> payloadFor(WrapperMessage::Type type) {
	switch (type) {
		case WrapperMessage::TYPE_LOGIN: return std::make_unique<pbnetwork::Login>();
		case WrapperMessage::TYPE_LOGOUT: return std::make_unique<pbnetwork::Logout>();
		case WrapperMessage::TYPE_CONNECTED: return std::make_unique<pbnetwork::Connected>();
		case WrapperMessage::TYPE_DISCONNECTED: return std::make_unique<pbnetwork::Disconnected>();
		case WrapperMessage::TYPE_CONV_MESSAGE: return std::make_unique<pbnetwork::ConversationMessage>();
		case WrapperMessage::TYPE_JOIN_ROOM: return std::make_unique<pbnetwork::Room>();
		case WrapperMessage::TYPE_LEAVE_ROOM: return std::make_unique<pbnetwork::Room>();
		default: return nullptr;
	}
}

}  // namespace

bool decodeClientLine(const std::string &line, WrapperMessage &out, std::string &error) {
	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;
	std::istringstream in(line);
	if (!Json::parseFromStream(builder, in, &root, &errs)) {
		error = "bad JSON: " + errs;
		return false;
	}
	if (!root.isObject() || !root["type"].isString()) {
		error = "missing \"type\"";
		return false;
	}

	WrapperMessage::Type type;
	if (!WrapperMessage::Type_Parse(root["type"].asString(), &type)) {
		error = "unknown type " + root["type"].asString();
		return false;
	}
	out = WrapperMessage();
	out.set_type(type);

	auto payload = payloadFor(type);
	if (!payload) {
		return true;  // PING, PONG and similar carry no payload
	}

	Json::StreamWriterBuilder writer;
	writer["indentation"] = "";
	std::string body = Json::writeString(writer, root.get("payload", Json::objectValue));
	auto status = google::protobuf::util::JsonStringToMessage(body, payload.get());
	if (!status.ok()) {
		error = "bad payload: " + std::string(status.message());
		return false;
	}
	out.set_payload(payload->SerializeAsString());
	return true;
}

bool encodeClientLine(const WrapperMessage &msg, std::string &line) {
	const std::string &name = WrapperMessage::Type_Name(msg.type());

	std::string json = "{}";
	if (auto payload = payloadFor(msg.type())) {
		if (!payload->ParseFromString(msg.payload())) {
			return false;
		}
		google::protobuf::util::JsonPrintOptions options;
		options.always_print_primitive_fields = false;
		json.clear();  // MessageToJsonString appends
		if (!google::protobuf::util::MessageToJsonString(*payload, &json, options).ok()) {
			return false;
		}
	}

	line = "{\"type\":\"" + name + "\",\"payload\":" + json + "}";
	return true;
}

}  // namespace next
