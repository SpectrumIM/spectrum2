import optparse
import sys
import time
import subprocess
import os

import slixmpp


class Responder(slixmpp.ClientXMPP):
	def __init__(self, jid, password, room, room_password, nick):
		slixmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.room_password = room_password
		self.nick = nick
		self.finished = False
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("muc::" + room + "::got_online", self.muc_got_online)
		self.add_event_handler("muc::" + room + "::got_offline", self.muc_got_offline)

		self.tests = {}
		self.tests["online_received"] = ["libcommuni: Received available presence from 'client'", False]
		self.tests["offline_received"] = ["libcommuni: Received unavailable presence frin 'client'", False]

	def muc_got_online(self, presence):
		if presence['muc']['nick'] == "client":
			self.tests["online_received"][1] = True

	def muc_got_offline(self, presence):
		if presence['muc']['nick'] == "client":
			self.tests["offline_received"][1] = True
			self.finished = True

	def start(self, event):
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, password=self.room_password, wait=True)

class Client(slixmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		slixmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("muc::" + room + "::got_online", self.muc_got_online)
		self.finished = False

		self.tests = {}
		self.tests["online_received"] = ["libcommuni: Received available presence from 'responder'", False]

	def muc_got_online(self, presence):
		if presence['muc']['nick'] == "responder":
			self.tests["online_received"][1] = True
			self.plugin['xep_0045'].leaveMUC(self.room, self.nick)

	def start(self, event):
		self.getRoster()
		self.sendPresence()
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, wait=True)
