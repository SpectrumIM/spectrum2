import optparse
import sys
import time
import subprocess
import os

import slixmpp
from slixmpp.jid import JID

# Verifies that our own messages sent elsewhere are delivered as carbons, and that our local
# messages are not.

# Requires XMPP backend with carbons enabled.

# NOTE: Prpl-jabber doesn't subscribe to carbons out of the box so
# What we can test:
# - Outside client gets carbons when Spectrum client speaks
# - Spectrum ignores its own messages as they're routed to it
# What we cannot test:
# - Spectrum passes through carbons of outside client messages

class Client(slixmpp.ClientXMPP):
	def __init__(self, jid, password, alt_jid, responder_jid):
		slixmpp.ClientXMPP.__init__(self, jid, password)
		self.full_jid = jid
		self.alt_jid = JID(alt_jid) # our own alternative jid (backend/spectrum)
		self.responder_jid = JID(responder_jid)
		self.register_plugin('xep_0030') # Service Discovery
		self.register_plugin('xep_0199') # XMPP Ping
		self.register_plugin('xep_0280') # Message Carbons
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("message", self.message)
		self.add_event_handler("carbon_sent", self.carbon_sent)
		self.add_event_handler("carbon_received", self.carbon_received)
		self['feature_mechanisms'].unencrypted_plain = True
		self.weird_cnt = 0
		self.cs_m1_cnt = 0
		self.cs_m2_cnt = 0
		self.cs_m3_cnt = 0

	def __str__(self):
		return "Client("+str(self.jid)+")"

	async def run(self, host, port):
		# Create and run the client in its own thread
		import threading
		import time
		
		# Store parameters for the thread
		client_jid = self.jid
		client_password = self.password
		client_alt_jid = self.alt_jid
		client_responder_jid = self.responder_jid
		client_full_jid = self.full_jid
		
		# We need to update self in the main thread with the client created in the thread
		# Use a queue to pass the client back
		import queue
		client_queue = queue.Queue()
		
		def create_and_run_client_in_thread():
			# Create event loop for this thread
			import asyncio
			loop = asyncio.new_event_loop()
			asyncio.set_event_loop(loop)
			
			# Create client in this thread (will use this thread's event loop)
			client = Client(client_full_jid, client_password, client_alt_jid, client_responder_jid)
			
			# Copy event handlers and other attributes from self to client
			# This is needed because the test expects certain attributes
			for attr_name in dir(self):
				if not attr_name.startswith('_') and not hasattr(client, attr_name):
					try:
						setattr(client, attr_name, getattr(self, attr_name))
					except:
						pass
            
			# Put the client in the queue for the main thread
			client_queue.put(client)
			
			try:
				to = (host, port)
				client.connect(*to)
				client.process(forever=True, timeout=60)
			except Exception as e:
				print(f"{client} connect() failed in thread: {e}")
		
		thread = threading.Thread(target=create_and_run_client_in_thread)
		thread.daemon = True
		thread.start()
		
		# Wait for client to be created and get it from the queue
		try:
			# Get the client with timeout
			new_client = client_queue.get(timeout=5)
			# Update self with the new client's attributes
			for attr_name in dir(new_client):
				if not attr_name.startswith('_'):
					try:
						setattr(self, attr_name, getattr(new_client, attr_name))
					except:
						pass
		except queue.Empty:
			print(f"Timeout waiting for client to be created in thread")
		
		time.sleep(2)  # Give it time to connect

	def start(self, event):
		#print str(self)+": on_start"
		self.get_roster()
		self.send_presence()
		#print str(self)+": enabling xep_0280..."
		self.plugin['xep_0280'].enable()
		#print str(self)+": xep_0280 enable()d"
		time.sleep(5) # let other clients connect
		
		# We'll send several pairs of messages from each client:
		# 1. Two unique ones
		# 2. Two identical ones
		# 3. Three times two unique ones
		# Each client should receive 1 copy of each message sent by another client.
		# Unique ones verify that it's the other message that we're getting,
		# identical ones verify that we're getting it even if it's identical to the one we've sent.
		
		#print str(self)+": sending to " + str(self.responder_jid)
		self.send_message(mto=self.responder_jid, mbody="Message 1 from "+str(self.full_jid))
		self.send_message(mto=self.responder_jid, mbody="Message 2");
		self.send_message(mto=self.responder_jid, mbody="Message 3 from "+str(self.full_jid))
		self.send_message(mto=self.responder_jid, mbody="Message 3 from "+str(self.full_jid))
		self.send_message(mto=self.responder_jid, mbody="Message 3 from "+str(self.full_jid))

	def message(self, msg):
		print(str(self)+": Weird unexpected message: "+str(msg))
		self.weird_cnt += 1

	def carbon_sent(self, msg):
		#print str(self)+": on_carbon_sent: "+str(msg)
		mfrom = msg['carbon_sent']['from']
		mbody = msg['carbon_sent']['body']

		if mfrom == self.jid:
			# Carbons are delivered with from==JID/other_resource, or with Spectrum, "/bot"
			print(str(self)+": Weird carbon_sent: from == own exact JID")
			self.weird_cnt += 1
			return

		if (mfrom.bare != JID(self.jid).bare) and (mfrom.bare != self.alt_jid.bare):
			# Carbons should be from our other resources
			print(str(self)+": Weird carbon_sent: from.bare != our JID")
			self.weird_cnt += 1
			return

		if (mbody.startswith("Message 1 from")):
			#print str(self)+": message1 carbon!"
			self.cs_m1_cnt += 1
			return

		if (mbody == "Message 2"):
			#print str(self)+": message2 carbon!"
			self.cs_m2_cnt += 1
			return

		if (mbody.startswith("Message 3 from")):
			#print str(self)+": message3 carbon!"
			self.cs_m3_cnt += 1
			return

		print(str(self)+": Weird unexpected carbon_sent: "+str(msg))
		self.weird_cnt += 1

	def carbon_received(self, msg):
		print(str(self)+": Weird unexpected carbon_received: "+str(msg))
		self.weird_cnt += 1

	def print_stats(self):
		print(str(self)+": msg1="+str(self.cs_m1_cnt)+", msg2="+str(self.cs_m2_cnt)+", msg3="+str(self.cs_m3_cnt)+", weird="+str(self.weird_cnt))


class Responder(slixmpp.ClientXMPP):
	def __init__(self, jid, password, client1_jid, client2_jid):
		slixmpp.ClientXMPP.__init__(self, jid, password)
		self.add_event_handler("session_start", self.start)
		self.add_event_handler("message", self.message)
		self.client1_jid = JID(client1_jid)
		self.client2_jid = JID(client2_jid)
		self.incoming_cnt = 0

	def __str__(self):
		return "Responder("+str(self.jid)+")"

	async def run(self, host, port):
		# Create and run the responder in its own thread
		import threading
		import time
		import queue
		
		# Store parameters for the thread
		responder_jid = self.jid
		responder_password = self.password
		responder_client1_jid = self.client1_jid
		responder_client2_jid = self.client2_jid
		
		# Queue to get the responder object back
		responder_queue = queue.Queue()
		
		def create_and_run_responder_in_thread():
			# Create event loop for this thread
			import asyncio
			loop = asyncio.new_event_loop()
			asyncio.set_event_loop(loop)
			
			# Create responder in this thread
			responder = Responder(responder_jid, responder_password, 
								responder_client1_jid, responder_client2_jid)
			
			# Copy attributes from self to responder
			for attr_name in dir(self):
				if not attr_name.startswith('_') and not hasattr(responder, attr_name):
					try:
						setattr(responder, attr_name, getattr(self, attr_name))
					except:
						pass
            
			# Put responder in queue for main thread
			responder_queue.put(responder)
			
			try:
				to = (host, port)
				responder.connect(*to)
				responder.process(forever=True, timeout=60)
			except Exception as e:
				print(f"{responder} connect() failed in thread: {e}")
		
		thread = threading.Thread(target=create_and_run_responder_in_thread)
		thread.daemon = True
		thread.start()
		
		# Wait for responder to be created and get it from the queue
		try:
			new_responder = responder_queue.get(timeout=5)
			# Update self with the new responder's attributes
			for attr_name in dir(new_responder):
				if not attr_name.startswith('_'):
					try:
						setattr(self, attr_name, getattr(new_responder, attr_name))
					except:
						pass
		except queue.Empty:
			print(f"Timeout waiting for responder to be created in thread")
		
		time.sleep(2)  # Give it time to connect

	def start(self, event):
		#print "Responder: on_start"
		self.get_roster()
		self.send_presence()

	def message(self, msg):
		#print "Responder: on_message: "+str(msg)
		self.incoming_cnt += 1

	def print_stats(self):
		print(str(self)+": incoming_cnt="+str(self.incoming_cnt))


class TestCase():
	def __init__(self, env):
		# Only run for configurations with backend_jids (legacy XMPP)
		self.enabled = not ((not env.client_backend_jid) or (not env.responder_backend_jid))
		self.env = env
		self.tests = {}
		self.tests["no_weird"] = ["backend-libpurple: No weird carbon-related messages delivered", False]
		self.tests["message1"] = ["backend-libpurple: Carbons are delivered to clients across spectrum boundary", False]
		self.tests["message1_exact"] = ["backend-libpurple: No unexpected carbons are delivered", False]
		self.tests["message2"] = ["backend-libpurple: Identical carbons are delivered correctly", False]
		self.tests["message3"] = ["backend-libpurple: Repeated carbons are delivered correctly", False]
		self.tests["responder_got_all"] = ["backend-libpurple: All messages have been delivered to responder", False]

	async def setup(self):
		# We need two clients: one connecting to spectrum, the other to backend.
		self.client1 = Client(self.env.client_jid+"/res1", self.env.client_password,
				self.env.client_backend_jid+"/res1", self.env.responder_jid)
		await self.client1.run(self.env.spectrum.host, self.env.spectrum.port)
		self.client2 = Client(self.env.client_backend_jid+"/res2", self.env.client_backend_password,
				self.env.client_jid+"/res1", self.env.responder_backend_jid+"/res2")
		await self.client2.run(self.env.backend.host, self.env.backend.port)
		# And one buddy at backend
		self.responder = Responder(self.env.responder_backend_jid, self.env.responder_backend_password,
			self.client1.alt_jid,
			self.client2.jid)
		await self.responder.run(self.env.backend.host, self.env.backend.port)

	def finished(self):
		return (
			self.client1.cs_m1_cnt>=1 and self.client1.cs_m2_cnt>=1 and self.client1.cs_m3_cnt>=3 and
			self.client2.cs_m1_cnt>=1 and self.client2.cs_m2_cnt>=1 and self.client2.cs_m3_cnt>=3
			and self.responder.incoming_cnt>=10 )

	def teardown(self):
		if hasattr(self, 'client1') and hasattr(self, 'client2'):
			self.tests["no_weird"][1] = self.client1.weird_cnt==0 and self.client2.weird_cnt==0
			self.tests["message1"][1] = self.client1.cs_m1_cnt>=1 and self.client2.cs_m1_cnt>=1
			self.tests["message1_exact"][1] = self.client1.cs_m1_cnt==1 and self.client2.cs_m1_cnt==1
			self.tests["message2"][1] = self.client1.cs_m2_cnt==1 and self.client2.cs_m2_cnt==1
			self.tests["message3"][1] = self.client1.cs_m3_cnt==3 and self.client2.cs_m3_cnt==3
			fail = not ( self.tests["no_weird"][1] and self.tests["message1_exact"][1]
				and self.tests["message2"][1] and self.tests["message3"][1] )
			if fail:
				self.client1.print_stats()
				self.client2.print_stats()
		if hasattr(self, 'client1'):
			self.client1.disconnect()
		if hasattr(self, 'client2'):
			self.client2.disconnect()
		if hasattr(self, 'responder'):
			self.tests["responder_got_all"][1] = (self.responder.incoming_cnt==10)
			self.responder.disconnect()
			if not self.tests["responder_got_all"][1]:
				self.responder.print_stats()
		self.tests = self.tests.values() # the wrapper expects value pairs
		pass
