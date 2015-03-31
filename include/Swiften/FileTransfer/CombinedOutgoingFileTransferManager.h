/*
 * Copyright (c) 2011 Tobias Markmann
 * Licensed under the simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

#include <Swiften/JID/JID.h>

#include "transport/presenceoracle.h"
#include <Swiften/FileTransfer/OutgoingFileTransfer.h>
#include <Swiften/Version.h>
#define HAVE_SWIFTEN_3  SWIFTEN_VERSION >= 0x030000

namespace Swift {

class JingleSessionManager;
class IQRouter;
class EntityCapsProvider;
class RemoteJingleTransportCandidateSelectorFactory;
class LocalJingleTransportCandidateGeneratorFactory;
class OutgoingFileTransfer;
class JID;
class IDGenerator;
class ReadBytestream;
class StreamInitiationFileInfo;
class SOCKS5BytestreamRegistry;
class SOCKS5BytestreamProxy;
class SOCKS5BytestreamServer;
class PresenceOracle;

class CombinedOutgoingFileTransferManager {
public:
	CombinedOutgoingFileTransferManager(JingleSessionManager* jingleSessionManager, IQRouter* router, EntityCapsProvider* capsProvider, RemoteJingleTransportCandidateSelectorFactory* remoteFactory, LocalJingleTransportCandidateGeneratorFactory* localFactory, SOCKS5BytestreamRegistry* bytestreamRegistry, SOCKS5BytestreamProxy* bytestreamProxy, Transport::PresenceOracle* presOracle, SOCKS5BytestreamServer *server);
	~CombinedOutgoingFileTransferManager();
	
	boost::shared_ptr<OutgoingFileTransfer> createOutgoingFileTransfer(const JID& from, const JID& to, boost::shared_ptr<ReadBytestream>, const StreamInitiationFileInfo&);

private:
	boost::optional<JID> highestPriorityJIDSupportingJingle(const JID& bareJID);
	boost::optional<JID> highestPriorityJIDSupportingSI(const JID& bareJID);
	JingleSessionManager* jsManager;
	IQRouter* iqRouter;
	EntityCapsProvider* capsProvider;
	RemoteJingleTransportCandidateSelectorFactory* remoteFactory;
	LocalJingleTransportCandidateGeneratorFactory* localFactory;
	IDGenerator *idGenerator;
	SOCKS5BytestreamRegistry* bytestreamRegistry;
	SOCKS5BytestreamProxy* bytestreamProxy;
	Transport::PresenceOracle* presenceOracle;
	SOCKS5BytestreamServer *bytestreamServer;
};

}
