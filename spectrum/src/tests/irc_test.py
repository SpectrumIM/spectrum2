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
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("groupchat_message", self.muc_message)

	def muc_message(self, msg):
		if msg['mucnick'] != self.nick:
			self.send_message(mto=msg['from'].bare,
							mbody="echo %s" % msg['body'],
							mtype='groupchat')

	def start(self, event):
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, wait=True)
		#time.sleep(1)
		#self.send_message(mto=self.room, mbody=self.message, mtype='groupchat')
		#time.sleep(10)
		#self.disconnect()

class Client(sleekxmpp.ClientXMPP):
	def __init__(self, jid, password, room, nick):
		sleekxmpp.ClientXMPP.__init__(self, jid, password)
		self.room = room
		self.nick = nick
		self.echo_received = False
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("groupchat_message", self.muc_message)

	def muc_message(self, msg):
		if msg['mucnick'] != self.nick:
			if msg['body'] == "echo abc":
				self.echo_received = True

	def start(self, event):
		self.getRoster()
		self.sendPresence()
		self.plugin['xep_0045'].joinMUC(self.room, self.nick, wait=True)
		self.send_message(mto=self.room, mbody="abc", mtype='groupchat')
		#time.sleep(10)
		#self.disconnect()

if __name__ == '__main__':
	os.system("../spectrum2 -n ./irc_test.cfg &")
	os.system("ngircd -f ngircd.conf &")
	time.sleep(1)
	responder = Responder("responder@localhost", "password", "#channel@localhost", "responder")
	responder.register_plugin('xep_0030')  # Service Discovery
	responder.register_plugin('xep_0045')  # Multi-User Chat
	responder.register_plugin('xep_0199')  # XMPP Ping
	responder['feature_mechanisms'].unencrypted_plain = True

	if responder.connect():
		responder.process(block=False)
	else:
		print "connect() failed"
		sys.exit(1)

	client = Client("client@localhost", "password", "#channel@localhost", "client")
	client.register_plugin('xep_0030')  # Service Discovery
	client.register_plugin('xep_0045')  # Multi-User Chat
	client.register_plugin('xep_0199')  # XMPP Ping
	client['feature_mechanisms'].unencrypted_plain = True

	time.sleep(2)

	if client.connect():
		client.process(block=False)
	else:
		print "connect() failed"
		sys.exit(1)

	time.sleep(10)
	client.disconnect()
	responder.disconnect()

	os.system("killall spectrum2")
	os.system("killall ngircd")
	os.system("killall spectrum2_libcommuni_backend")

	if client.echo_received == False:
		print "ERROR: 'echo abc' not received."
		sys.exit(1)

	print "ALL TESTS PASSED"
