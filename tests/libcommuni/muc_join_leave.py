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
		self.tests["online_received"] = ["libcommuni: Received available presence from 'client'", False]
		self.tests["offline_received"] = ["libcommuni: Received unavailable presence from 'client'", False]

	async def muc_got_online(self, presence):
		# In slixmpp, nick is in presence['from'].resource, not presence['muc']['nick']
		print(f"DEBUG: Responder muc_got_online called, from={presence['from']}, resource={presence['from'].resource if presence['from'] else 'None'}")
		if presence['from'] and presence['from'].resource == "client":
			self.tests["online_received"][1] = True

	async def muc_got_offline(self, presence):
		# In slixmpp, nick is in presence['from'].resource, not presence['muc']['nick']
		print(f"DEBUG: Responder muc_got_offline called, from={presence['from']}, resource={presence['from'].resource if presence['from'] else 'None'}")
		if presence['from'] and presence['from'].resource == "client":
			self.tests["offline_received"][1] = True
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
		self.add_event_handler("muc::" + room + "::got_online", self.muc_got_online)
		self.finished = False

		self.tests = {}
		self.tests["online_received"] = ["libcommuni: Received available presence from 'responder'", False]

	async def muc_got_online(self, presence):
		# In slixmpp, nick is in presence['from'].resource, not presence['muc']['nick']
		print(f"DEBUG: Client muc_got_online called, from={presence['from']}, resource={presence['from'].resource if presence['from'] else 'None'}")
		if presence['from'] and presence['from'].resource == "responder":
			self.tests["online_received"][1] = True
			self.plugin['xep_0045'].leave_muc(self.room, self.nick)

	async def start(self, event):
		self.get_roster()
		self.send_presence()
		self.plugin['xep_0045'].join_muc(self.room, self.nick)
