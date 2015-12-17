import optparse
import sys
import time
import subprocess
import os

import sleekxmpp


class Responder(sleekxmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		sleekxmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.finished = False
		self.add_event_handler("session_start", self.start)
		self.tests = {}

	def start(self, event):
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, wait=True)

class Client(sleekxmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		sleekxmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("groupchat_message", self.muc_message)
		self.finished = False

		self.tests = {}
		self.tests["whois_received"] = ["libcommuni: Receive /whois command response", False]

	def muc_message(self, msg):
		if msg['mucnick'] != self.nick:
			if msg["body"] == "responder is connected to irc.example.net (responder)\nresponder is a user on channels: @#channel":
				self.tests["whois_received"][1] = True
				self.finished = True

	def start(self, event):
		self.getRoster()
		self.sendPresence()
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, wait=True)
		self.send_message(mto=self.room, mbody="/whois responder", mtype='groupchat')
