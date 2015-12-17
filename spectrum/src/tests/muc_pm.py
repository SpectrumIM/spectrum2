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
		self.add_event_handler("message", self.message)

		self.tests = {}

	def message(self, msg):
		if msg['body'] == "abc" and msg['from'] == self.room + "/client":
			self.send_message(mto=self.room + "/client",
							mbody="echo %s" % msg['body'],
							mtype='chat')
		elif msg['body'] == "def" and msg['from'] == self.room + "/client":
			self.send_message(mto=self.room + "/client",
							mbody="echo %s" % msg['body'],
							mtype='chat')
		else:
			self.finished = True

	def start(self, event):
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, wait=True)

class Client(sleekxmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		sleekxmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("message", self.message)
		self.finished = False

		self.tests = {}
		self.tests["echo1_received"] = ["libcommuni: Send and receive private messages - 1st msg", False]
		self.tests["echo2_received"] = ["libcommuni: Send and receive private messages - 2nd msg", False]

	def message(self, msg):
		if msg['body'] == "echo abc" and msg['from'] == self.room + "/responder":
			self.tests["echo1_received"][1] = True
			self.send_message(mto=self.room + "/responder", mbody="def", mtype='chat')
		elif msg['body'] == "echo def" and msg['from'] == self.room + "/responder":
			self.tests["echo2_received"][1] = True
			self.finished = True

	def start(self, event):
		self.getRoster()
		self.sendPresence()
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, wait=True)
		self.send_message(mto=self.room + "/responder", mbody="abc", mtype='chat')
