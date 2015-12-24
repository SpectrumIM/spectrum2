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
		self.add_event_handler("message", self.message)

		self.tests = {}
		self.tests["not_authorized"] = ["'Not Authorized' received", False]
		self.tests["help_received"] = ["Help received", False]
		self.tests["register_received"] = ["Password changed", False]
		self.tests["abc_received"] = ["Test message received", False]

	def message(self, msg):
		if msg['body'] == "Not Authorized":
			self.tests["not_authorized"][1] = True
		elif msg['body'].find("Try using") != -1:
			self.send_message(mto="spectrum2@spectrum2tests.xmpp.slack.com", mbody=".spectrum2 register client@localhost password #spectrum2_contactlist")
			self.tests["help_received"][1] = True
		elif msg['body'] == "You have successfully registered 3rd-party account. Spectrum 2 is now connecting to the 3rd-party network.":
			self.tests["register_received"][1] = True
		elif msg['body'] == "abc":
			self.tests["abc_received"][1] = True
			self.finished = True

	def start(self, event):
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, password=self.room_password, wait=False)

class Client(sleekxmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		sleekxmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("message", self.message)
		self.finished = False

		self.tests = {}

	def message(self, msg):
		pass
		#print "client", msg['body']
		#if msg['body'] == "echo abc" and msg['from'] == self.room + "/responder":
			#self.tests["echo1_received"][1] = True
			#self.send_message(mto=self.room + "/responder", mbody="def", mtype='chat')
		#elif msg['body'] == "echo def" and msg['from'] == self.room + "/responder":
			#self.tests["echo2_received"][1] = True
			#self.finished = True

	def start(self, event):
		self.getRoster()
		self.sendPresence()
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, wait=True)
		self.send_message(mto=self.room, mbody="abc", mtype='groupchat')
