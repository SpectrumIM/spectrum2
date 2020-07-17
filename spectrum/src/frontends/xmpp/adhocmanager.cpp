/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2012, Jan Kaluza <hanzz@soc.pidgin.im>
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

#include "adhocmanager.h"
#include "adhoccommandfactory.h"
#include "discoitemsresponder.h"
#include "transport/UserManager.h"
#include "transport/User.h"
#include "transport/Logging.h"
#include "transport/StorageBackend.h"
#include "transport/Transport.h"
#include "transport/Config.h"

#include "XMPPFrontend.h"

namespace Transport {

DEFINE_LOGGER(adHocManagerLogger, "AdHocManager");

AdHocManager::AdHocManager(Component *component, DiscoItemsResponder *discoItemsResponder, UserManager *userManager, StorageBackend *storageBackend) : Swift::Responder<Swift::Command>(static_cast<XMPPFrontend *>(component->getFrontend())->getIQRouter()){
	m_component = component;
	m_discoItemsResponder = discoItemsResponder;
	m_userManager = userManager;
	m_storageBackend = storageBackend;

	m_collectTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer(20);
	m_collectTimer->onTick.connect(boost::bind(&AdHocManager::removeOldSessions, this));
	m_collectTimer->start();

	m_userManager->onUserCreated.connect(boost::bind(&AdHocManager::handleUserCreated, this, _1));
}

AdHocManager::~AdHocManager() {
	m_collectTimer->stop();
	stop();
}

void AdHocManager::start() {
	Swift::Responder<Swift::Command>::start();
}

void AdHocManager::stop() {
	Swift::Responder<Swift::Command>::stop();

	for (SessionsMap::iterator it = m_sessions.begin(); it != m_sessions.end(); it++) {
		std::vector<std::string> candidates;
		for (CommandsMap::iterator ct = it->second.begin(); ct != it->second.end(); ct++) {
			delete ct->second;
		}
	}

	m_sessions.clear();
}

void AdHocManager::handleUserCreated(User *user) {
	for (std::map<std::string, AdHocCommandFactory *>::const_iterator it = m_factories.begin(); it != m_factories.end(); it++) {
		for (std::map<std::string, std::string>::const_iterator it2 = it->second->getUserSettings().begin(); it2 != it->second->getUserSettings().end(); it2++) {
			std::string value = CONFIG_STRING_DEFAULTED(m_component->getConfig(), it->second->getNode() + "." + it2->first, it2->second);
			if (it2->second == "true" || it2->second == "1" || it2->second == "false" || it2->second == "0") {
				value = CONFIG_BOOL_DEFAULTED(m_component->getConfig(), it->second->getNode() + "." + it2->first, it2->second == "true" || it2->second == "1") ? "1" : "0";
			}
			if (m_storageBackend) {
				int type = (int) TYPE_BOOLEAN;
				m_storageBackend->getUserSetting(user->getUserInfo().id, it2->first, type, value);
			}
			user->addUserSetting(it2->first, value);
		}
	}
}

void AdHocManager::addAdHocCommand(AdHocCommandFactory *factory) {
	if (m_factories.find(factory->getNode()) != m_factories.end()) {
		LOG4CXX_ERROR(adHocManagerLogger, "Command with node " << factory->getNode() << " is already registered. Ignoring this attempt.");
		return;
	}

	m_factories[factory->getNode()] = factory;
	m_discoItemsResponder->addAdHocCommand(factory->getNode(), factory->getName());
}

void AdHocManager::removeOldSessions() {
	unsigned long removedCommands = 0;
	time_t now = time(NULL);

	std::vector<std::string> toRemove;
	for (SessionsMap::iterator it = m_sessions.begin(); it != m_sessions.end(); it++) {
		std::vector<std::string> candidates;
		for (CommandsMap::iterator ct = it->second.begin(); ct != it->second.end(); ct++) {
			if (now - ct->second->getLastActivity() > 15*60) {
				candidates.push_back(it->first);
				delete ct->second;
				removedCommands++;
			}
		}

		BOOST_FOREACH(std::string &key, candidates) {
			it->second.erase(key);
		}

		if (it->second.empty()) {
			toRemove.push_back(it->first);
		}
	}

	BOOST_FOREACH(std::string &key, toRemove) {
		m_sessions.erase(key);
	}

	if (removedCommands > 0) {
		LOG4CXX_INFO(adHocManagerLogger, "Removed " << removedCommands << " old commands sessions.");
	}
}

bool AdHocManager::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> payload) {
	return false;
}

bool AdHocManager::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> payload) {
	AdHocCommand *command = NULL;
	// Try to find AdHocCommand according to 'from' and session_id
	if (!payload->getSessionID().empty() && m_sessions.find(from) != m_sessions.end()) {
		if (m_sessions[from].find(payload->getSessionID()) != m_sessions[from].end()) {
			command = m_sessions[from][payload->getSessionID()];
		}
		else {
			LOG4CXX_ERROR(adHocManagerLogger, from.toString() << ": Unknown session id " << payload->getSessionID() << " - ignoring");
			sendError(from, id, Swift::ErrorPayload::BadRequest, Swift::ErrorPayload::Modify);
			return true;
		}
	}
	// Check if we can create command with this node
	else if (m_factories.find(payload->getNode()) != m_factories.end()) {
		command = m_factories[payload->getNode()]->createAdHocCommand(m_component, m_userManager, m_storageBackend, from, to);
		m_sessions[from][command->getId()] = command;
		LOG4CXX_INFO(adHocManagerLogger, from.toString() << ": Started new AdHoc command session with node " << payload->getNode());
	}
	else {
		LOG4CXX_INFO(adHocManagerLogger, from.toString() << ": Unknown node " << payload->getNode() << ". Can't start AdHoc command session.");
		sendError(from, id, Swift::ErrorPayload::BadRequest, Swift::ErrorPayload::Modify);
		return true;
	}

	if (!command) {
		LOG4CXX_ERROR(adHocManagerLogger, from.toString() << ": createAdHocCommand for node " << payload->getNode() << " returned NULL pointer");
		sendError(from, id, Swift::ErrorPayload::BadRequest, Swift::ErrorPayload::Modify);
		return true;
	}

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Command> response = command->handleRequest(payload);
	if (!response) {
		LOG4CXX_ERROR(adHocManagerLogger, from.toString() << ": handleRequest for node " << payload->getNode() << " returned NULL pointer");
		sendError(from, id, Swift::ErrorPayload::BadRequest, Swift::ErrorPayload::Modify);
		return true;
	}

	response->setSessionID(command->getId());

	sendResponse(from, id, response);

	command->refreshLastActivity();

	// Command completed, so we can remove it now
	if (response->getStatus() == Swift::Command::Completed || response->getStatus() == Swift::Command::Canceled) {
		// update userSettings map if user is already connected
		User *user = m_userManager->getUser(from.toBare().toString());
		if (user) {
			handleUserCreated(user);
		}

		m_sessions[from].erase(command->getId());
		if (m_sessions[from].empty()) {
			m_sessions.erase(from);
		}
		delete command;
	}
	

	return true;
}

}
