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
		self.nick = nick
		self.finished = False
		self.room_password = room_password
		self.add_event_handler("session_start", self.start)
		self.tests = {}

		self.register_plugin('xep_0045')  # MUC plugin
	def start(self, event):
		self.plugin['xep_0045'].join_muc(self.room, self.nick, password=self.room_password)

class Client(slixmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		slixmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("groupchat_message", self.muc_message)
		self.finished = False

		self.tests = {}
		self.tests["whois1_received"] = ["libcommuni: Receive /whois command response", False]
		self.tests["whois2_received"] = ["libcommuni: Receive /whois command response for invalid nickname", False]

		self.register_plugin('xep_0045')  # MUC plugin
	def muc_message(self, msg):
		if msg['mucnick'] != self.nick:
			if msg["body"] == "responder is connected to irc.example.net (responder)\nresponder is a user on channels: @#channel":
				self.tests["whois1_received"][1] = True
			elif msg["body"] == "nonexisting: No such client":
				self.tests["whois2_received"][1] = True
				self.finished = True

	def start(self, event):
		self.get_roster()
		self.send_presence()
		self.plugin['xep_0045'].join_muc(self.room, self.nick)
		self.send_message(mto=self.room, mbody="/whois responder", mtype='groupchat')
		self.send_message(mto=self.room, mbody="/whois nonexisting", mtype='groupchat')
