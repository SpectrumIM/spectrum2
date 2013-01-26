---
layout: page
title: Spectrum 2
---

## Spectrum backend protocol

Spectrum 2 communicates with backend using protocol based on [Google Protocol Buffers](http://code.google.com/p/protobuf/). This protocol has libraries for many languages, so backend can be written practically in any language.

This document describes how to implement new backend in different language than C++.

### Backend execution

When XMPP user wants to login to legacy network, Spectrum 2 executes particular backend according to configuration. Typically it executes backend with following arguments:

	/usr/bin/backend --host localhost --port 31640 --service.backend_id=1 "/etc/spectrum2/transports/sample_client.cfg"

As the first thing after start, backed should connect Spectrum 2 main instance according to `--host` and `--port` argument. `--service.backend_id` is ID of backend and the last argument is always path to config file. Note that Spectrum 2 can pass more
arguments to backend and backend should ignore them.

When Spectrum 2 starts the backend, you should see this in spectrum.login

	INFO  NetworkPluginServer: Backend should now connect to Spectrum2 instance. Spectrum2 won't accept any connection before backend connects

When you establish the connection between your backend and Spectrum 2, it shows this in log:

	NetworkPluginServer: New backend 0x84ad60 connected. Current backend count=1

### The protocol

When connection betwen backend and Spectrum 2 is establish, Spectrum 2 starts communicating with the backend.

The protocol is defined in in [include/transport/protocol.proto](https://github.com/hanzz/libtransport/blob/master/include/transport/protocol.proto) file. Usually there is tool for your language to compile this .proto file into source module which can be later used to serialize/deserialize structures in your language and make packets from them.

Once you serialize particular Protocol Buffer structure, you can send it in following format:

	| size | serialized Protocol Buffer structure |

* size - 4 bytes long size of "serialized Protocol Buffer structure" in network encoding (as returned by htonl() function - see "man htonl")

### Receiving PING message

First message sent by Spectrum 2 is always PING. Backend has to response this message with PONG.

You will see this event also in Spectrum 2 log:

	NetworkPluginServer: PING to 0x84ad60 (ID=)

Your backend will receive 4 bytes in network encoding. You should parse those 4 bytes to number representation and store it into variable N. Then you should read N bytes from the socket to receive serialized Protobuf message called WrapperMessage. Use the
module generated from protocol.proto to parse it.

WrapperMessage has two fields, type and payload:

* type - Type of the real message stored in the payload field.
* payload - Real message wrapped in this WrapperMessage. This is again serialized Protobuf message and can be parsed using the module generated from protocol.proto

In our case, the type is TYPE_PING and the payload is not defined. You should now answer the PING message

### Answering PING message

PING message has to be answered with PONG. To Answer this message, just generate WrapperMessage using the module generated from protocol.proto and set the type to TYPE_PONG. Now serialie the message, prepend it with its size as defined above and send it to Spectrum 2.

Once you answer the PING request properly, Spectrum 2 will show following message in log:

	Component: Connecting XMPP server 127.0.0.1 port 5347
