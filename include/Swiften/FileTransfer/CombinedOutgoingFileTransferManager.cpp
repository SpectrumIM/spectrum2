/*
 * Copyright (c) 2011 Tobias Markmann
 * Licensed under the simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include "CombinedOutgoingFileTransferManager.h"

#include <boost/smart_ptr/make_shared.hpp>

#include <Swiften/JID/JID.h>
#include "Swiften/Disco/EntityCapsProvider.h"
#include <Swiften/Jingle/JingleSessionManager.h>
#include <Swiften/Jingle/JingleSessionImpl.h>
#include <Swiften/Jingle/JingleContentID.h>
#include <Swiften/FileTransfer/OutgoingJingleFileTransfer.h>
#include <Swiften/FileTransfer/MyOutgoingSIFileTransfer.h>
#include <Swiften/FileTransfer/SOCKS5BytestreamServer.h>
#include <Swiften/Base/IDGenerator.h>
#include <Swiften/Elements/Presence.h>
#include <Swiften/Base/foreach.h>


namespace Swift {

CombinedOutgoingFileTransferManager::CombinedOutgoingFileTransferManager(JingleSessionManager* jingleSessionManager, IQRouter* router, EntityCapsProvider* capsProvider, RemoteJingleTransportCandidateSelectorFactory* remoteFactory, LocalJingleTransportCandidateGeneratorFactory* localFactory, SOCKS5BytestreamRegistry* bytestreamRegistry, SOCKS5BytestreamProxy* bytestreamProxy, Transport::PresenceOracle *presOracle, SOCKS5BytestreamServer *bytestreamServer) : jsManager(jingleSessionManager), iqRouter(router), capsProvider(capsProvider), remoteFactory(remoteFactory), localFactory(localFactory), bytestreamRegistry(bytestreamRegistry), bytestreamProxy(bytestreamProxy), presenceOracle(presOracle), bytestreamServer(bytestreamServer) {
	idGenerator = new IDGenerator();
}

CombinedOutgoingFileTransferManager::~CombinedOutgoingFileTransferManager() {
	delete idGenerator;
}

boost::shared_ptr<OutgoingFileTransfer> CombinedOutgoingFileTransferManager::createOutgoingFileTransfer(const JID& from, const JID& receipient, boost::shared_ptr<ReadBytestream> readBytestream, const StreamInitiationFileInfo& fileInfo) {
	// check if receipient support Jingle FT
	boost::optional<JID> fullJID = highestPriorityJIDSupportingJingle(receipient);
	if (!fullJID.is_initialized()) {
		fullJID = highestPriorityJIDSupportingSI(receipient);
	}
	else {
		JingleSessionImpl::ref jingleSession = boost::make_shared<JingleSessionImpl>(from, receipient, idGenerator->generateID(), iqRouter);

		//jsManager->getSession(receipient, idGenerator->generateID());
		assert(jingleSession);
		jsManager->registerOutgoingSession(from, jingleSession);
#if !HAVE_SWIFTEN_3
		boost::shared_ptr<OutgoingJingleFileTransfer> jingleFT =  boost::shared_ptr<OutgoingJingleFileTransfer>(new OutgoingJingleFileTransfer(jingleSession, remoteFactory, localFactory, iqRouter, idGenerator, from, receipient, readBytestream, fileInfo, bytestreamRegistry, bytestreamProxy));
		return jingleFT;
#endif
	}

	if (!fullJID.is_initialized()) {
		return boost::shared_ptr<OutgoingFileTransfer>();
	}
	
	// otherwise try SI
	boost::shared_ptr<MyOutgoingSIFileTransfer> jingleFT =  boost::shared_ptr<MyOutgoingSIFileTransfer>(new MyOutgoingSIFileTransfer(idGenerator->generateID(), from, fullJID.get(), fileInfo.getName(), fileInfo.getSize(), fileInfo.getDescription(), readBytestream, iqRouter, bytestreamServer, bytestreamRegistry));
	// else fail
	
	return jingleFT;
}

boost::optional<JID> CombinedOutgoingFileTransferManager::highestPriorityJIDSupportingJingle(const JID& bareJID) {
	JID fullReceipientJID;
	int priority = INT_MIN;
	
	//getAllPresence(bareJID) gives you all presences for the bare JID (i.e. all resources) Remko Tronçon @ 11:11
	std::vector<Presence::ref> presences = presenceOracle->getAllPresence(bareJID);

	//iterate over them
	foreach(Presence::ref pres, presences) {
		if (pres->getPriority() > priority) {
			// look up caps from the jid
			DiscoInfo::ref info = capsProvider->getCaps(pres->getFrom());
			if (info && info->hasFeature(DiscoInfo::JingleFeature) && info->hasFeature(DiscoInfo::JingleFTFeature) &&
				info->hasFeature(DiscoInfo::JingleTransportsIBBFeature)) {
			
				priority = pres->getPriority();
				fullReceipientJID = pres->getFrom();
			}
		}
	}
	
	return fullReceipientJID.isValid() ? boost::optional<JID>(fullReceipientJID) : boost::optional<JID>();
}

boost::optional<JID> CombinedOutgoingFileTransferManager::highestPriorityJIDSupportingSI(const JID& bareJID) {
	JID fullReceipientJID;
	int priority = INT_MIN;
	
	//getAllPresence(bareJID) gives you all presences for the bare JID (i.e. all resources) Remko Tronçon @ 11:11
	std::vector<Presence::ref> presences = presenceOracle->getAllPresence(bareJID);

	//iterate over them
	foreach(Presence::ref pres, presences) {
		if (pres->getPriority() > priority) {
			// look up caps from the jid
			DiscoInfo::ref info = capsProvider->getCaps(pres->getFrom());
			if (info && info->hasFeature("http://jabber.org/protocol/si/profile/file-transfer")) {
			
				priority = pres->getPriority();
				fullReceipientJID = pres->getFrom();
			}
		}
	}
	
	return fullReceipientJID.isValid() ? boost::optional<JID>(fullReceipientJID) : boost::optional<JID>();
}

}
