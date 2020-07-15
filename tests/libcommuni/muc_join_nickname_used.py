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
		self.add_event_handler("muc::" + room + "::got_online", self.muc_got_online)
		self.add_event_handler("muc::" + room + "::got_offline", self.muc_got_offline)

		self.tests = {}
		self.tests["online_received"] = ["libcommuni: Received available presence", False]
		self.tests["offline_received"] = ["libcommuni: Received unavailable presence", False]

	def muc_got_online(self, presence):
		if presence['muc']['nick'] == "respond_":
			self.tests["online_received"][1] = True
			self.send_message(mto=self.room,
							mbody="disconnect please :)",
							mtype='groupchat')

	def muc_got_offline(self, presence):
		if presence['muc']['nick'] == "respond_":
			self.tests["offline_received"][1] = True
			self.finished = True

	def start(self, event):
		self.plugin['xep_0045'].joinMUC(self.room, "respond", password=self.room_password, wait=True)

class Client(sleekxmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		sleekxmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("groupchat_message", self.muc_message)
		self.finished = False

		self.tests = {}

	def muc_message(self, msg):
		self.plugin['xep_0045'].leaveMUC(self.room, "respond_")

	def start(self, event):
		self.getRoster()
		self.sendPresence()
		self.plugin['xep_0045'].joinMUC(self.room, "respond", wait=True)
