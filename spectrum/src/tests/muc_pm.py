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
			self.send_message(mto=self.room + "/client",
							mbody="echo %s" % msg['body'],
							mtype='chat')

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
		self.tests["echo_received"] = ["libcommuni: Send and receive private messages", False]

	def message(self, msg):
		if msg['body'] == "echo abc" and msg['from'] == self.room + "/responder":
			self.tests["echo_received"][1] = True
			self.finished = True

	def start(self, event):
		self.getRoster()
		self.sendPresence()
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, wait=True)
		self.send_message(mto=self.room + "/responder", mbody="abc", mtype='chat')
