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
		self.add_event_handler("groupchat_subject", self.message)

		self.tests = {}
		self.tests["subject_set1"] = ["libpurple: Set subject", False]

	def message(self, msg):
		if msg['subject'] == "New subject":
			self.tests["subject_set1"][1] = True
			self.finished = True

	def start(self, event):
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, password=self.room_password, wait=True)
		self.send_message(mto=self.room, mbody=None, msubject="New subject", mtype='groupchat')

class Client(sleekxmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		sleekxmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("groupchat_subject", self.message)
		self.finished = False

		self.tests = {}
		self.tests["subject_set2"] = ["libpurple: Receive subject", False]

	def message(self, msg):
		if msg['subject'] == "New subject":
			self.tests["subject_set2"][1] = True
			self.finished = True

	def start(self, event):
		self.getRoster()
		self.sendPresence()
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, wait=True)
