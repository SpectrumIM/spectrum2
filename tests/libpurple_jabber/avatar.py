import optparse
import sys
import time
import subprocess
import os

import sleekxmpp

import struct
import zlib
def chunk(t, data):
	return struct.pack('>I', len(data)) + t + data
#+ struct.pack('>I', zlib.crc32(t + data))
png = b'\x89PNG\r\n\x1A\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 1, 1, 1, 0, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(struct.pack('>BB', 0, 0))) + chunk(b'IEND', b'')


class Responder(sleekxmpp.ClientXMPP):
	def __init__(self, jid, password, room, room_password, nick):
		sleekxmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.room_password = room_password
		self.nick = nick
		self.finished = False
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("vcard_avatar_update", self.vcard_avatar_update)

		self.tests = {}
		self.tests["photohash"] = ["libpurple: Photo hash received in Presence", False]

	def vcard_avatar_update(self, presence):
		self.tests["photohash"][1] = True
		self.finished = True

	def start(self, event):
		self.getRoster()
		self.sendPresence()

class Client(sleekxmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		sleekxmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.add_event_handler("session_start", self.start)
		self.finished = False

		self.tests = {}

	def start(self, event):
		self.getRoster()
		self.sendPresence()
		self['xep_0153'].set_avatar(avatar=png, mtype='image/png')
