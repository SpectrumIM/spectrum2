#!/usr/bin/python

import asyncore, argparse, protocol_pb2, socket, logging
from NetworkPlugin import NetworkPlugin

np = None


logger = logging.getLogger('Template Backend')
logger.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
ch.setFormatter(formatter)
logger.addHandler(ch)


def handleTransportData(data):
	"""
	This function is called when data is received from the NetworkPlugin server
	"""
	np.handleDataRead(data)
	
    
class SpectrumPlugin(NetworkPlugin):
	global logger
	def __init__(self, iochannel):
		NetworkPlugin.__init__(self)
		self.iochannel = iochannel
		logger.info("Starting plugin.")

	def sendData(self, string):
		"""
		NetworkPlugin uses this method to send the data to networkplugin server
		"""
		self.iochannel.sendData(string)
		logger.info("Starting plugin.")

	def handleLoginRequest(self, user, legacyName, password):
		self.handleConnected(user)
		logger.info("Added Echo Buddy")
		self.handleBuddyChanged(user, "echo", "Echo", [], protocol_pb2.STATUS_ONLINE)

	def handleLogoutRequest(self, user, legacyName):
		pass

	def handleMessageSendRequest(self, user, legacyName, message, xhtml = ""):
		logger.info("Message sent from "  + user + ' to ' + legacyName)
		if(legacyName == "echo"):
			logger.info("Message Echoed: " + message)
			self.handleMessage(user, legacyName, message)

	def handleBuddyUpdatedRequest(self, user, buddyName, alias, groups):
		logger.info("Added Buddy " + buddyName)
		self.handleBuddyChanged(user, buddyName, alias, groups, protocol_pb2.STATUS_ONLINE)
		
	def handleBuddyRemovedRequest(self, user, buddyName, groups):
		pass



class IOChannel(asyncore.dispatcher):
	def __init__(self, host, port, readCallBack):
		asyncore.dispatcher.__init__(self)
		self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
		self.connect((host, port))
		self.handleReceivedData = readCallBack
		self.send_buffer = ""

	def sendData(self, data):
		self.send_buffer += data

	def handle_connect(self):
		pass

	def handle_close(self):
		self.close()

	def handle_read(self):
		data = self.recv(65536)
		self.handleReceivedData(data)

	def handle_write(self):
		sent = self.send(self.send_buffer)
		self.send_buffer = self.send_buffer[sent:]

	def writable(self):
		return (len(self.send_buffer) > 0)

	def readable(self):
		return True


if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument('--host', type=str, required=True)
	parser.add_argument('--port', type=int, required=True)
	parser.add_argument('config_file', type=str)

	args = parser.parse_args()
	io = IOChannel(args.host, args.port, handleTransportData)
	np = SpectrumPlugin(io)
	asyncore.loop()
