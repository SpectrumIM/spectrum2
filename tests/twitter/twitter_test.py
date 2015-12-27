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

		self.tests = {}


class Client(sleekxmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		sleekxmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("message", self.message)
		self.finished = False

		self.tests = {}
		self.tests["timeline_poll"] = ["Timeline received automatically", False]
		self.tests["help"] = ["#help command", False]
		self.tests["status1"] = ["#status command response", False]
		self.tests["status2"] = ["#status command pushes status", False]
		self.tests["follow"] = ["#follow command", False]
		self.tests["friends"] = ["#friends command", False]
		self.tests["unfollow"] = ["#unfollow command", False]
		self.tests["friends2"] = ["#friends after unfollow command", False]

		self.status = "timeline"
		self.timestamp = int(time.time())

	def message(self, msg):
		if self.status == "timeline" and msg['body'].find("spectrum2tests") != -1 and msg['body'].find("MsgId") != -1:
			self.tests["timeline_poll"][1] = True
			self.send_message(mto=msg['from'], mbody="#help")
			self.status = "help"
		elif self.status == "help" and msg['body'].find("You will receive tweets of people you follow") != -1:
			self.tests["help"][1] = True
			self.send_message(mto=msg['from'], mbody="#status My testing status " + str(self.timestamp))
			self.status = "status"
		elif self.status == "status" and msg['body'].find("Status Update successful") != -1:
			self.tests["status1"][1] = True
		elif self.status == "status" and msg['body'].find("spectrum2tests") != -1 and msg['body'].find("MsgId") != -1:
			self.tests["status2"][1] = True
			self.status = "follow"
			self.send_message(mto=msg['from'], mbody="#follow colinpwheeler")
		elif self.status == "follow" and msg['body'] == "You are now following colinpwheeler":
			self.status = "friends"
			self.tests["follow"][1] = True
			self.send_message(mto=msg['from'], mbody="#friends")
		elif self.status == "friends" and msg['body'].find("USER LIST") != -1 and msg['body'].find("colinpwheeler") != -1:
			self.status = "unfollow"
			self.tests["friends"][1] = True
			self.send_message(mto=msg['from'], mbody="#unfollow colinpwheeler")
		elif self.status == "unfollow" and msg['body'] == "You are not following colinpwheeler anymore":
			self.status = "friends2"
			self.tests["unfollow"][1] = True
			self.send_message(mto=msg['from'], mbody="#friends")
		elif self.status == "friends2" and msg['body'].find("USER LIST") != -1 and msg['body'].find("colinpwheeler") == -1:
			self.tests["friends2"][1] = True
			self.finished = True

	def start(self, event):
		self.getRoster()
		self.sendPresence()

