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
		self.tests["online_received"] = ["MUC: Received available presence", False]
		self.tests["offline_received"] = ["MUC: Received unavailable presence", False]

		self.register_plugin('xep_0045')  # MUC plugin
	def muc_got_online(self, presence):
		if presence['muc']['nick'] == "client":
			self.tests["online_received"][1] = True

	def muc_got_offline(self, presence):
		if presence['muc']['nick'] == "client":
			self.tests["offline_received"][1] = True
			self.finished = True

	def start(self, event):
		self.plugin['xep_0045'].join_muc(self.room, self.nick, password=self.room_password)

class Client(slixmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		slixmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.add_event_handler("session_start", self.start)
		self.finished = False

		self.tests = {}

		self.register_plugin('xep_0045')  # MUC plugin
	def start(self, event):
		self.get_roster()
		self.send_presence()
		self.plugin['xep_0045'].join_muc(self.room, self.nick)
		self.plugin['xep_0045'].leave_muc(self.room, self.nick)
		self.finished = True
