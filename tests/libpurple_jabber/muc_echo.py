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
		self.room_password = room_password
		self.finished = False
		self.register_plugin("xep_0004") # Forms
		self.register_plugin("xep_0045") # MUC
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("groupchat_message", self.muc_message)

		self.tests = {}

	def muc_message(self, msg):
		if msg['mucnick'] != self.nick:
			self.send_message(mto=msg['from'].bare,
							mbody="echo %s" % msg['body'],
							mtype='groupchat')

	def start(self, event):
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, password=self.room_password, wait=True)
		# Room creator must provide initial configuration or the room will be item-not-found for others
		# Not supported by Spectrum though
		#form = self.plugin['xep_0004'].make_form(ftype='submit')
		#self.plugin['xep_0045'].setRoomConfig(self.room, form)
		self.finished = True # Just reply to requests

class Client(slixmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		slixmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.register_plugin("xep_0045") # MUC
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("groupchat_message", self.muc_message)
		self.finished = False

		self.tests = {}
		self.tests["echo_received"] = ["MUC: Send and receive messages", False]

	def muc_message(self, msg):
		if msg['mucnick'] != self.nick:
			if msg['body'] == "echo abc" or msg['body'] == "<owner> echo abc":
				self.tests["echo_received"][1] = True
				self.finished = True

	def start(self, event):
		self.getRoster()
		self.sendPresence()
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, wait=True)
		self.send_message(mto=self.room, mbody="abc", mtype='groupchat')
