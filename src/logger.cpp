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

#include "transport/logger.h"
#include "transport/usermanager.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/storagebackend.h"
#include "transport/userregistration.h"
#include "transport/abstractbuddy.h"
#include "transport/rostermanager.h"
#include <boost/bind.hpp>

using namespace boost;

namespace Transport {

Logger::Logger(Component *component) {
	component->onConnected.connect(bind(&Logger::handleConnected, this));
	component->onConnectionError.connect(bind(&Logger::handleConnectionError, this, _1));
	component->onXMLIn.connect(bind(&Logger::handleXMLIn, this, _1));
	component->onXMLOut.connect(bind(&Logger::handleXMLOut, this, _1));
}

Logger::~Logger(){
}

void Logger::setStorageBackend(StorageBackend *storage) {
	storage->onStorageError.connect(bind(&Logger::handleStorageError, this, _1, _2));
}

void Logger::setUserRegistration(UserRegistration *userRegistration) {
	userRegistration->onUserRegistered.connect(bind(&Logger::handleUserRegistered, this, _1));
	userRegistration->onUserUnregistered.connect(bind(&Logger::handleUserUnregistered, this, _1));
	userRegistration->onUserUpdated.connect(bind(&Logger::handleUserUpdated, this, _1));
}

void Logger::setUserManager(UserManager *userManager) {
	userManager->onUserCreated.connect(bind(&Logger::handleUserCreated, this, _1));
	userManager->onUserDestroyed.connect(bind(&Logger::handleUserDestroyed, this, _1));
}

void Logger::setRosterManager(RosterManager *rosterManager) {
	rosterManager->onBuddySet.connect(bind(&Logger::handleBuddySet, this, _1));
	rosterManager->onBuddyUnset.connect(bind(&Logger::handleBuddyUnset, this, _1));
}

void Logger::handleConnected() {
	std::cout << "[COMPONENT] Connected to Jabber Server!\n";
}

void Logger::handleConnectionError(const Swift::ComponentError &error) {
	std::cout << "[COMPONENT] Connection Error!\n";
	switch (error.getType()) {
		case Swift::ComponentError::UnknownError: std::cout << "[COMPONENT] Disconnect reason: UnknownError\n"; break;
		case Swift::ComponentError::ConnectionError: std::cout << "[COMPONENT] Disconnect reason: ConnectionError\n"; break;
		case Swift::ComponentError::ConnectionReadError: std::cout << "[COMPONENT] Disconnect reason: ConnectionReadError\n"; break;
		case Swift::ComponentError::ConnectionWriteError: std::cout << "[COMPONENT] Disconnect reason: ConnectionWriteError\n"; break;
		case Swift::ComponentError::XMLError: std::cout << "[COMPONENT] Disconnect reason: XMLError\n"; break;
		case Swift::ComponentError::AuthenticationFailedError: std::cout << "[COMPONENT] Disconnect reason: AuthenticationFailedError\n"; break;
		case Swift::ComponentError::UnexpectedElementError: std::cout << "[COMPONENT] Disconnect reason: UnexpectedElementError\n"; break; 
	};
}

void Logger::handleXMLIn(const std::string &data) {
	std::cout << "[XML IN] " << data << "\n";
}

void Logger::handleXMLOut(const std::string &data) {
	std::cout << "[XML OUT] " << data << "\n";
}

void Logger::handleStorageError(const std::string &statement, const std::string &error) {
	std::cout << "[SQL ERROR] \"" << error << "\" during statement \"" << statement << "\"\n";
}

void Logger::handleUserRegistered(const UserInfo &user) {
	std::cout << "[REGISTRATION] User \"" << user.jid << "\" registered as \"" << user.uin << "\"\n";
}

void Logger::handleUserUnregistered(const UserInfo &user) {
	std::cout << "[REGISTRATION] User \"" << user.jid << "\" unregistered \"" << user.uin << "\"\n";
}

void Logger::handleUserUpdated(const UserInfo &user) {
	std::cout << "[REGISTRATION] User \"" << user.jid << "\" updated \"" << user.uin << "\"\n";
}

void Logger::handleUserCreated(User *user) {
	std::cout << "[USERMANAGER] User \"" << user->getJID().toString() << "\" (UIN: \"" << user->getUserInfo().uin << "\") connected and User class has been created\n";
}

void Logger::handleUserDestroyed(User *user) {
	std::cout << "[USERMANAGER] User \"" << user->getJID().toBare().toString() << "\" (UIN: \"" << user->getUserInfo().uin << "\") disconnected and User class is going to be destroyed\n";
}

void Logger::handleBuddySet(AbstractBuddy *buddy) {
	std::cout << "[ROSTERMANAGER] \"" << buddy->getRosterManager()->getUser()->getJID().toBare().toString() << "\": Buddy \"" << buddy->getSafeName() << "\" (ALIAS: \"" << buddy->getAlias() << "\") has been bound with this user's roster.\n";
}

void Logger::handleBuddyUnset(AbstractBuddy *buddy) {
	std::cout << "[ROSTERMANAGER] \"" << buddy->getRosterManager()->getUser()->getJID().toBare().toString() << "\": Buddy \"" << buddy->getSafeName() << "\" (ALIAS: \"" << buddy->getAlias() << "\") has been unbound with this user's roster.\n";
}

}
