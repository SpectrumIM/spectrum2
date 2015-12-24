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
		self.nick = nick
		self.finished = False
		self.room_password = room_password
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("groupchat_message", self.muc_message)

		self.tests = {}

	def muc_message(self, msg):
		if msg['body'] == "ready":
			self.send_message(mto=self.room, mbody=None, msubject='The new subject', mtype='groupchat')

	def start(self, event):
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, password=self.room_password, wait=True)

class Client(sleekxmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		sleekxmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("groupchat_subject", self.muc_subject)
		self.finished = False

		self.tests = {}
		self.tests["subject_changed"] = ["libcommuni: Change topic", False]

	def muc_subject(self, msg):
		if msg['subject'] == "The new subject":
			self.tests["subject_changed"][1] = True
			self.finished = True

	def start(self, event):
		self.getRoster()
		self.sendPresence()
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, wait=True)
		self.send_message(mto=self.room, mbody="ready", mtype='groupchat')
