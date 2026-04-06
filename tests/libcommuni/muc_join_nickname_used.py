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
		self.register_plugin('xep_0045')  # MUC plugin
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("muc::" + room + "::got_online", self.muc_got_online)
		self.add_event_handler("muc::" + room + "::got_offline", self.muc_got_offline)

		self.tests = {}
		self.tests["online_received"] = ["libcommuni: Received available presence", False]
		self.tests["offline_received"] = ["libcommuni: Received unavailable presence", False]

	async def muc_got_online(self, presence):
		# In slixmpp, nick is in presence['from'].resource, not presence['muc']['nick']
		if presence['from'].resource == "respond_":
			self.tests["online_received"][1] = True
			self.send_message(mto=self.room,
							mbody="disconnect please :)",
							mtype='groupchat')

	async def muc_got_offline(self, presence):
		# In slixmpp, nick is in presence['from'].resource, not presence['muc']['nick']
		if presence['from'].resource == "respond_":
			self.tests["offline_received"][1] = True
			self.finished = True

	async def start(self, event):
		self.plugin['xep_0045'].join_muc(self.room, "respond", password=self.room_password)

class Client(slixmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		slixmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.register_plugin('xep_0045')  # MUC plugin
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("groupchat_message", self.muc_message)
		self.finished = False

		self.tests = {}

	async def muc_message(self, msg):
		self.plugin['xep_0045'].leave_muc(self.room, "respond_")

	async def start(self, event):
		self.get_roster()
		self.send_presence()
		self.plugin['xep_0045'].join_muc(self.room, "respond")
