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
		self.add_event_handler("muc::" + room + "::presence", self.muc_got_online)

		self.tests = {}
		self.tests["online_received"] = ["libcommuni: Received away presence from 'client'", False]

	async def muc_got_online(self, presence):
		# In slixmpp, nick is in presence['from'].resource, not presence['muc']['nick']
		if presence['from'].resource == "client" and presence['show'] == "away":
			self.tests["online_received"][1] = True
			self.finished = True

	async def start(self, event):
		self.plugin['xep_0045'].join_muc(self.room, self.nick, password=self.room_password)

class Client(slixmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		slixmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.register_plugin('xep_0045')  # MUC plugin
		self.add_event_handler("session_start", self.start)
		self.finished = False

		self.tests = {}

	async def start(self, event):
		self.get_roster()
		self.send_presence()
		self.plugin['xep_0045'].join_muc(self.room, self.nick)
		self.send_presence(pshow = "away")

