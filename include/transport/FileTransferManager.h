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

#include <Swiften/Elements/StreamInitiationFileInfo.h>
#include <Swiften/FileTransfer/CombinedOutgoingFileTransferManager.h>
#include <Swiften/FileTransfer/IncomingFileTransferManager.h>
#include <Swiften/FileTransfer/SOCKS5BytestreamRegistry.h>
#include <Swiften/FileTransfer/SOCKS5BytestreamServer.h>


#include <Swiften/FileTransfer/SOCKS5BytestreamProxiesManager.h>
#include <Swiften/FileTransfer/SOCKS5BytestreamServerManager.h>

namespace Transport {

class UserManager;
class User;
class Component;
class Buddy;

class FileTransferManager {
	public:
		typedef struct Transfer {
			std::shared_ptr<Swift::OutgoingFileTransfer> ft;
			Swift::JID from;
			Swift::JID to;
			std::shared_ptr<Swift::ReadBytestream> readByteStream;
		} Transfer;

		FileTransferManager(Component *component, UserManager *userManager);
		virtual ~FileTransferManager();
		
		FileTransferManager::Transfer sendFile(User *user, Buddy *buddy, std::shared_ptr<Swift::ReadBytestream> byteStream, const Swift::StreamInitiationFileInfo &info);

	private:
		Component *m_component;
		UserManager *m_userManager;
		Swift::CombinedOutgoingFileTransferManager* m_outgoingFTManager;
		Swift::RemoteJingleTransportCandidateSelectorFactory* m_remoteCandidateSelectorFactory;
		Swift::LocalJingleTransportCandidateGeneratorFactory* m_localCandidateGeneratorFactory;
		Swift::JingleSessionManager *m_jingleSessionManager;
		Swift::SOCKS5BytestreamRegistry* m_bytestreamRegistry;
		Swift::SOCKS5BytestreamServerManager* m_proxyServerManager;
		Swift::SOCKS5BytestreamProxiesManager *m_proxyManager;
};

}
