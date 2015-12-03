import protocol_pb2, socket, struct, sys, os

def WRAP(MESSAGE, TYPE):
 	wrap = protocol_pb2.WrapperMessage()
	wrap.type = TYPE
	wrap.payload = MESSAGE
	return wrap.SerializeToString()
	
class NetworkPlugin:
	"""
	Creates new NetworkPlugin and connects the Spectrum2 NetworkPluginServer.
	@param loop: Event loop.
	@param host: Host where Spectrum2 NetworkPluginServer runs.
	@param port: Port. 
	"""
	
	def __init__(self):
		self.m_pingReceived = False
		self.m_data = ""
		self.m_init_res = 0

	def handleMessage(self, user, legacyName, msg, nickname = "", xhtml = "", timestamp = ""):
		m = protocol_pb2.ConversationMessage()
		m.userName = user
		m.buddyName = legacyName
		m.message = msg
		m.nickname = nickname
		m.xhtml = xhtml
		m.timestamp = str(timestamp)

		message = WRAP(m.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_CONV_MESSAGE)
		self.send(message)

	def handleMessageAck(self, user, legacyName, ID):
		m = protocol_pb2.ConversationMessage()
		m.userName = user
		m.buddyName = legacyName
		m.message = ""
		m.id = ID

		message = WRAP(m.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_CONV_MESSAGE_ACK)
		self.send(message)


	def handleAttention(self, user, buddyName, msg):
		m = protocol_pb2.ConversationMessage()
		m.userName = user
		m.buddyName = buddyName
		m.message = msg

		message = WRAP(m.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_ATTENTION)
		self.send(message)

	def handleVCard(self, user, ID, legacyName, fullName, nickname, photo):
		vcard = protocol_pb2.VCard()
		vcard.userName = user
		vcard.buddyName = legacyName
		vcard.id = ID
		vcard.fullname = fullName
		vcard.nickname = nickname
		vcard.photo = photo 

		message = WRAP(vcard.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_VCARD)
		self.send(message)


	def handleSubject(self, user, legacyName, msg, nickname = ""):
		m = protocol_pb2.ConversationMessage()
		m.userName = user
		m.buddyName = legacyName
		m.message = msg 
		m.nickname = nickname

		message = WRAP(m.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_ROOM_SUBJECT_CHANGED)
		#print "SENDING MESSAGE"
		self.send(message)


	def handleBuddyChanged(self, user, buddyName, alias, groups, status, statusMessage = "", iconHash = "", blocked = False):
		buddy = protocol_pb2.Buddy()
		buddy.userName = user
		buddy.buddyName = buddyName
		buddy.alias = alias
		buddy.group.extend(groups)
		buddy.status = status
		buddy.statusMessage = statusMessage
		buddy.iconHash = iconHash
		buddy.blocked = blocked

		message = WRAP(buddy.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_BUDDY_CHANGED)
		self.send(message)

	def handleBuddyRemoved(self, user, buddyName):
		buddy = protocol_pb2.Buddy()
		buddy.userName = user
		buddy.buddyName = buddyName

		message = WRAP(buddy.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_BUDDY_REMOVED)
		self.send(message);

	def handleBuddyTyping(self, user, buddyName):
		buddy = protocol_pb2.Buddy()
		buddy.userName = user
		buddy.buddyName = buddyName

		message = WRAP(buddy.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_BUDDY_TYPING)
		self.send(message);

	def handleBuddyTyped(self, user, buddyName):
		buddy = protocol_pb2.Buddy()
		buddy.userName  = user
		buddy.buddyName = buddyName

		message = WRAP(buddy.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_BUDDY_TYPED)
		self.send(message);

	def handleBuddyStoppedTyping(self, user, buddyName):
		buddy = protocol_pb2.Buddy()
		buddy.userName = user
		buddy.buddyName = buddyName

		message = WRAP(buddy.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_BUDDY_STOPPED_TYPING)
		self.send(message)

	def handleAuthorization(self, user, buddyName):
		buddy = protocol_pb2.Buddy()
		buddy.userName = user
		buddy.buddyName = buddyName

		message = WRAP(buddy.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_AUTH_REQUEST)
		self.send(message)


	def handleConnected(self, user):
		d = protocol_pb2.Connected()
		d.user = user

		message = WRAP(d.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_CONNECTED)
		self.send(message);	


	def handleDisconnected(self, user, error = 0, msg = ""):
		d = protocol_pb2.Disconnected()
		d.user = user
		d.error = error
		d.message = msg

		message = WRAP(d.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_DISCONNECTED)
		self.send(message);


	def handleParticipantChanged(self, user, nickname, room, flags, status, statusMessage = "", newname = ""):
		d = protocol_pb2.Participant()
		d.userName = user
		d.nickname = nickname
		d.room = room
		d.flag = flags
		d.newname = newname
		d.status = status
		d.statusMessage = statusMessage

		message = WRAP(d.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_PARTICIPANT_CHANGED)
		self.send(message);


	def handleRoomNicknameChanged(self, user, r, nickname):
		room = protocol_pb2.Room()
		room.userName = user
		room.nickname = nickname
		room.room = r
		room.password = ""

		message = WRAP(room.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_ROOM_NICKNAME_CHANGED)
		self.send(message);

	def handleRoomList(self, rooms):
		roomList = protocol_pb2.RoomList()

		for room in rooms:
			roomList.room.append(room[0])
			roomList.name.append(room[1])

		message = WRAP(roomList.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_ROOM_LIST)
		self.send(message);


	def handleFTStart(self, user, buddyName, fileName, size):
		room = protocol_pb2.File()
		room.userName = user
		room.buddyName = buddyName
		room.fileName = fileName
		room.size = size

		message = WRAP(room.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_FT_START)
		self.send(message);

	def handleFTFinish(self, user, buddyName, fileName, size, ftid):
		room = protocol_pb2.File()
		room.userName = user
		room.buddyName = buddyName
		room.fileName = fileName
		room.size = size

		# Check later
		if ftid != 0:
			room.ftID = ftid 
			
		message = WRAP(room.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_FT_FINISH)
		self.send(message)


	def handleFTData(self, ftID, data):
		d = protocol_pb2.FileTransferData()
		d.ftid = ftID
		d.data = data

		message = WRAP(d.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_FT_DATA);
		self.send(message)

	def handleBackendConfig(self, section, key, value):
		c = protocol_pb2.BackendConfig()
		c.config = "[%s]\n%s = %s\n" % (section, key, value)

		message = WRAP(c.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_BACKEND_CONFIG);
		self.send(message)

	def handleLoginPayload(self, data):
		payload = protocol_pb2.Login()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		self.handleLoginRequest(payload.user, payload.legacyName, payload.password, payload.extraFields)

	def handleLogoutPayload(self, data):
		payload = protocol_pb2.Logout()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		self.handleLogoutRequest(payload.user, payload.legacyName)
	
	def handleStatusChangedPayload(self, data):
		payload = protocol_pb2.Status()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		self.handleStatusChangeRequest(payload.userName, payload.status, payload.statusMessage)

	def handleConvMessagePayload(self, data):
		payload = protocol_pb2.ConversationMessage()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		self.handleMessageSendRequest(payload.userName, payload.buddyName, payload.message, payload.xhtml, payload.id)
	
	def handleConvMessageAckPayload(self, data):
                payload = protocol_pb2.ConversationMessage()
                if (payload.ParseFromString(data) == False):
                        #TODO: ERROR
                        return
                self.handleMessageAckRequest(payload.userName, payload.buddyName, payload.id)



	def handleAttentionPayload(self, data):
		payload = protocol_pb2.ConversationMessage()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		self.handleAttentionRequest(payload.userName, payload.buddyName, payload.message)
	
	def handleFTStartPayload(self, data):
		payload = protocol_pb2.File()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		self.handleFTStartRequest(payload.userName, payload.buddyName, payload.fileName, payload.size, payload.ftID);

	def handleFTFinishPayload(self, data):
		payload = protocol_pb2.File()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		self.handleFTFinishRequest(payload.userName, payload.buddyName, payload.fileName, payload.size, payload.ftID)

	def handleFTPausePayload(self, data):
		payload = protocol_pb2.FileTransferData()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		self.handleFTPauseRequest(payload.ftID)

	def handleFTContinuePayload(self, data):
		payload = protocol_pb2.FileTransferData()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		self.handleFTContinueRequest(payload.ftID)

	def handleJoinRoomPayload(self, data):
		payload = protocol_pb2.Room()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		self.handleJoinRoomRequest(payload.userName, payload.room, payload.nickname, payload.password)

	def handleLeaveRoomPayload(self, data):
		payload = protocol_pb2.Room()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		self.handleLeaveRoomRequest(payload.userName, payload.room)

	def handleVCardPayload(self, data):
		payload = protocol_pb2.VCard()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		if payload.HasField('photo'):
			self.handleVCardUpdatedRequest(payload.userName, payload.photo, payload.nickname)
		elif len(payload.buddyName)  > 0:
			self.handleVCardRequest(payload.userName, payload.buddyName, payload.id)

	def handleBuddyChangedPayload(self, data):
		payload = protocol_pb2.Buddy()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		if payload.HasField('blocked'):
			self.handleBuddyBlockToggled(payload.userName, payload.buddyName, payload.blocked)
		else:
			groups = [g for g in payload.group]
			self.handleBuddyUpdatedRequest(payload.userName, payload.buddyName, payload.alias, groups);

	def handleBuddyRemovedPayload(self, data):
		payload = protocol_pb2.Buddy()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		groups = [g for g in payload.group]
		self.handleBuddyRemovedRequest(payload.userName, payload.buddyName, groups);

	def handleBuddiesPayload(self, data):
		payload = protocol_pb2.Buddies()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return

		self.handleBuddies(payload);

	def handleChatStatePayload(self, data, msgType):
		payload = protocol_pb2.Buddy()
		if (payload.ParseFromString(data) == False):
			#TODO: ERROR
			return
		if msgType == protocol_pb2.WrapperMessage.TYPE_BUDDY_TYPING:
				self.handleTypingRequest(payload.userName, payload.buddyName)
		elif  msgType == protocol_pb2.WrapperMessage.TYPE_BUDDY_TYPED:
				self.handleTypedRequest(payload.userName, payload.buddyName)
		elif msgType == protocol_pb2.WrapperMessage.TYPE_BUDDY_STOPPED_TYPING:
				self.handleStoppedTypingRequest(payload.userName, payload.buddyName)


	def handleDataRead(self, data):
		self.m_data += data
		while len(self.m_data) != 0:
			expected_size = 0
			if (len(self.m_data) >= 4):
				expected_size = struct.unpack('!I', self.m_data[0:4])[0]
				if (len(self.m_data) - 4 < expected_size):
					return
			else:
				return

			wrapper = protocol_pb2.WrapperMessage()
			if (wrapper.ParseFromString(self.m_data[4:expected_size+4]) == False):
				self.m_data = self.m_data[expected_size+4:]
				return

			self.m_data = self.m_data[4+expected_size:]

			if wrapper.type == protocol_pb2.WrapperMessage.TYPE_LOGIN:
					self.handleLoginPayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_LOGOUT:
					self.handleLogoutPayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_PING:
					self.sendPong()
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_CONV_MESSAGE:
					self.handleConvMessagePayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_JOIN_ROOM:
					self.handleJoinRoomPayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_LEAVE_ROOM:
					self.handleLeaveRoomPayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_VCARD:
					self.handleVCardPayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_BUDDY_CHANGED:
					self.handleBuddyChangedPayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_BUDDY_REMOVED:
					self.handleBuddyRemovedPayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_STATUS_CHANGED:
					self.handleStatusChangedPayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_BUDDY_TYPING:
					self.handleChatStatePayload(wrapper.payload, protocol_pb2.WrapperMessage.TYPE_BUDDY_TYPING)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_BUDDY_TYPED:
					self.handleChatStatePayload(wrapper.payload, protocol_pb2.WrapperMessage.TYPE_BUDDY_TYPED)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_BUDDY_STOPPED_TYPING:
					self.handleChatStatePayload(wrapper.payload, protocol_pb2.WrapperMessage.TYPE_BUDDY_STOPPED_TYPING)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_ATTENTION:
					self.handleAttentionPayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_FT_START:
					self.handleFTStartPayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_FT_FINISH:
					self.handleFTFinishPayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_FT_PAUSE:
					self.handleFTPausePayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_FT_CONTINUE:
					self.handleFTContinuePayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_EXIT:
				self.handleExitRequest()
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_CONV_MESSAGE_ACK:
                                self.handleConvMessageAckPayload(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_RAW_XML:
				self.handleRawXmlRequest(wrapper.payload)
			elif wrapper.type == protocol_pb2.WrapperMessage.TYPE_BUDDIES:
					self.handleBuddiesPayload(wrapper.payload)


	def send(self, data):
		header = struct.pack('!I',len(data))
		self.sendData(header + data)

	def checkPing(self):
		if (self.m_pingReceived == False):
			self.handleExitRequest()
		self.m_pingReceived = False


	def sendPong(self):
		self.m_pingReceived = True
		wrap = protocol_pb2.WrapperMessage()
		wrap.type = protocol_pb2.WrapperMessage.TYPE_PONG
		message = wrap.SerializeToString()
		self.send(message)
		self.sendMemoryUsage()
	

	def sendMemoryUsage(self):
		stats = protocol_pb2.Stats()

		stats.init_res = self.m_init_res
		res = 0
		shared = 0

		e_res, e_shared = self.handleMemoryUsage()

		stats.res = res + e_res
		stats.shared = shared + e_shared
		stats.id = str(os.getpid())

		message = WRAP(stats.SerializeToString(), protocol_pb2.WrapperMessage.TYPE_STATS)
		self.send(message)


	def handleLoginRequest(self, user, legacyName, password, extra):
		""" 
		Called when XMPP user wants to connect legacy network.
		You should connect him to legacy network and call handleConnected or handleDisconnected function later.
		@param user: XMPP JID of user for which this event occurs.
		@param legacyName: Legacy network name of this user used for login.
		@param password: Legacy network password of this user.
		"""

		#\msc
		#NetworkPlugin,YourNetworkPlugin,LegacyNetwork;
		#NetworkPlugin->YourNetworkPlugin [label="handleLoginRequest(...)", URL="\ref NetworkPlugin::handleLoginRequest()"];
		#YourNetworkPlugin->LegacyNetwork [label="connect the legacy network"];
		#--- [label="If password was valid and user is connected and logged in"];
		#YourNetworkPlugin<-LegacyNetwork [label="connected"];
		#YourNetworkPlugin->NetworkPlugin [label="handleConnected()", URL="\ref NetworkPlugin::handleConnected()"];
		#--- [label="else"];
		#YourNetworkPlugin<-LegacyNetwork [label="disconnected"];
		#YourNetworkPlugin->NetworkPlugin [label="handleDisconnected()", URL="\ref NetworkPlugin::handleDisconnected()"];
		#\endmsc

		raise NotImplementedError, "Implement me"

	def handleBuddies(self, buddies):
		pass

	def handleLogoutRequest(self, user, legacyName):
		"""
		Called when XMPP user wants to disconnect legacy network.
		You should disconnect him from legacy network.
		@param user: XMPP JID of user for which this event occurs.
		@param legacyName: Legacy network name of this user used for login.
		"""

		raise NotImplementedError, "Implement me"

	def handleMessageSendRequest(self, user, legacyName, message, xhtml = "", ID = 0):
		"""
		Called when XMPP user sends message to legacy network.
		@param user: XMPP JID of user for which this event occurs.
		@param legacyName: Legacy network name of buddy or room.
		@param message: Plain text message.
		@param xhtml: XHTML message.
		@param ID: message ID
		"""

		raise NotImplementedError, "Implement me"

	def handleMessageAckRequest(self, user, legacyName, ID = 0):
                """
                Called when XMPP user sends message to legacy network.
                @param user: XMPP JID of user for which this event occurs.
                @param legacyName: Legacy network name of buddy or room.
                @param ID: message ID
                """

                # raise NotImplementedError, "Implement me"
		pass


	def handleVCardRequest(self, user, legacyName, ID):
		""" Called when XMPP user requests VCard of buddy.
		@param user: XMPP JID of user for which this event occurs.
		@param legacyName: Legacy network name of buddy whose VCard is requested.
		@param ID: ID which is associated with this request. You have to pass it to handleVCard function when you receive VCard."""
			
		#\msc
		#NetworkPlugin,YourNetworkPlugin,LegacyNetwork;
		#NetworkPlugin->YourNetworkPlugin [label="handleVCardRequest(...)", URL="\ref NetworkPlugin::handleVCardRequest()"];
		#YourNetworkPlugin->LegacyNetwork [label="start VCard fetching"];
		#YourNetworkPlugin<-LegacyNetwork [label="VCard fetched"];
		#YourNetworkPlugin->NetworkPlugin [label="handleVCard()", URL="\ref NetworkPlugin::handleVCard()"];
		#\endmsc
	
		pass
			

	def handleVCardUpdatedRequest(self, user, photo, nickname):
		"""
		Called when XMPP user updates his own VCard.
		You should update the VCard in legacy network too.
		@param user: XMPP JID of user for which this event occurs.
		@param photo: Raw photo data.
		"""
		pass

	def handleJoinRoomRequest(self, user, room, nickname, pasword):
		pass
		
	def handleLeaveRoomRequest(self, user, room):
		pass
		
	def handleStatusChangeRequest(self, user, status, statusMessage):
		pass

	def handleBuddyUpdatedRequest(self, user,  buddyName, alias, groups):
		pass
		
	def handleBuddyRemovedRequest(self, user, buddyName, groups):
		pass
		
	def handleBuddyBlockToggled(self, user, buddyName, blocked):
		pass

	def handleTypingRequest(self, user, buddyName):
		pass
		
	def handleTypedRequest(self, user, buddyName):
		pass
		
	def handleStoppedTypingRequest(self, user, buddyName):
		pass
		
	def handleAttentionRequest(self, user, buddyName, message):
		pass

	def handleFTStartRequest(self, user, buddyName, fileName, size, ftID):
		pass
		
	def handleFTFinishRequest(self, user, buddyName, fileName, size, ftID):
		pass
		
	def handleFTPauseRequest(self, ftID):
		pass

	def handleFTContinueRequest(self, ftID):
		pass
	

	def handleMemoryUsage(self):
		return (0,0)

	def handleExitRequest(self):
		sys.exit(1)

	def handleRawXmlRequest(self, xml):
		pass

	def sendData(self, data):
		pass

