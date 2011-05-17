#! /usr/bin/python

from pbnetwork_pb2 import *
import time
import sys
import socket
import struct


connected = Connected()
connected.name = "hanzz.k@gmail.com"

wrapper = WrapperMessage()
wrapper.type = WrapperMessage.TYPE_CONNECTED
wrapper.payload = connected.SerializeToString()

sck = socket.socket()
sck.connect(("localhost", 10000))

message = wrapper.SerializeToString();
header = struct.pack(">L", len(message))
print [header]
sck.send(header + message+header + message);
sck.send(header + message);
sck.send(header + message);
sck.send(header + message);
sck.send(header + message);

print sck.recv(255)
