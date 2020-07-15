import optparse
import sys
import time
import subprocess
import os

import sleekxmpp


class Responder(sleekxmpp.ClientXMPP):
	def __init__(self, jid, password, room, room_password, nick):
		sleekxmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.room_password = room_password
		self.nick = nick
		self.finished = False
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("muc::" + room + "::presence", self.muc_got_online)

		self.tests = {}
		self.tests["online_received"] = ["libcommuni: Received away presence from 'client'", False]

	def muc_got_online(self, presence):
		if presence['muc']['nick'] == "client" and presence['show'] == "away":
			self.tests["online_received"][1] = True
			self.finished = True

	def start(self, event):
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, password=self.room_password, wait=True)

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
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, wait=True)
		self.sendPresence(ptype = "away")

