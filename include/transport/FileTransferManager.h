/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * Copyright (C) 2011, Jan Kaluza <hanzz.k@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#pragma once
#include <Swiften/Version.h>
#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)

#include <Swiften/Elements/StreamInitiationFileInfo.h>
#include <Swiften/FileTransfer/CombinedOutgoingFileTransferManager.h>
#include <Swiften/FileTransfer/IncomingFileTransferManager.h>
#include <Swiften/FileTransfer/SOCKS5BytestreamRegistry.h>
#include <Swiften/FileTransfer/SOCKS5BytestreamServer.h>


#if HAVE_SWIFTEN_3
#include <Swiften/FileTransfer/SOCKS5BytestreamProxiesManager.h>
#include <Swiften/FileTransfer/SOCKS5BytestreamServerManager.h>
#else
#include <Swiften/FileTransfer/ConnectivityManager.h>
#include <Swiften/FileTransfer/DefaultLocalJingleTransportCandidateGeneratorFactory.h>
#include <Swiften/FileTransfer/DefaultRemoteJingleTransportCandidateSelectorFactory.h>
#include <Swiften/FileTransfer/SOCKS5BytestreamProxy.h>
#endif

namespace Transport {

class UserManager;
class User;
class Component;
class Buddy;

class FileTransferManager {
	public:
		typedef struct Transfer {
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::OutgoingFileTransfer> ft;
			Swift::JID from;
			Swift::JID to;
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::ReadBytestream> readByteStream;
		} Transfer;

		FileTransferManager(Component *component, UserManager *userManager);
		virtual ~FileTransferManager();
		
		FileTransferManager::Transfer sendFile(User *user, Buddy *buddy, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::ReadBytestream> byteStream, const Swift::StreamInitiationFileInfo &info);

	private:
		Component *m_component;
		UserManager *m_userManager;
		Swift::CombinedOutgoingFileTransferManager* m_outgoingFTManager;
		Swift::RemoteJingleTransportCandidateSelectorFactory* m_remoteCandidateSelectorFactory;
		Swift::LocalJingleTransportCandidateGeneratorFactory* m_localCandidateGeneratorFactory;
		Swift::JingleSessionManager *m_jingleSessionManager;
		Swift::SOCKS5BytestreamRegistry* m_bytestreamRegistry;
#if HAVE_SWIFTEN_3
		Swift::SOCKS5BytestreamServerManager* m_proxyServerManager;
		Swift::SOCKS5BytestreamProxiesManager *m_proxyManager;
#else
		Swift::SOCKS5BytestreamServer* m_bytestreamServer;
		Swift::SOCKS5BytestreamProxy* m_bytestreamProxy;
		Swift::SOCKS5BytestreamServer *bytestreamServer;
		Swift::ConnectivityManager* m_connectivityManager;
#endif
};

}
