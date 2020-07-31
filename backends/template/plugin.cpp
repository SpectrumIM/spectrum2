#include "plugin.h"
// Transport includes
#include "transport/Config.h"
#include "transport/NetworkPlugin.h"
#include "transport/Logging.h"

// Swiften
#include "Swiften/Swiften.h"

// Boost
#include <boost/algorithm/string.hpp>
using namespace boost::filesystem;
using namespace boost::program_options;
using namespace Transport;

DEFINE_LOGGER(logger, "Backend Template");

Plugin::Plugin(Config *config, Swift::SimpleEventLoop *loop, const std::string &host, int port) : NetworkPlugin() {
	this->config = config;
	m_factories = new Swift::BoostNetworkFactories(loop);
	m_conn = m_factories->getConnectionFactory()->createConnection();
	m_conn->onDataRead.connect(boost::bind(&Plugin::_handleDataRead, this, _1));
	m_conn->connect(Swift::HostAddressPort(*(Swift::HostAddress::fromString(host)), port));

	LOG4CXX_INFO(logger, "Starting the plugin.");
}

// NetworkPlugin uses this method to send the data to networkplugin server
void Plugin::sendData(const std::string &string) {
	m_conn->write(Swift::createSafeByteArray(string));
}

// This method has to call handleDataRead with all received data from network plugin server
void Plugin::_handleDataRead(std::shared_ptr<Swift::SafeByteArray> data) {
	std::string d(data->begin(), data->end());
	handleDataRead(d);
}

void Plugin::handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
	handleConnected(user);
	LOG4CXX_INFO(logger, user << ": Added buddy - Echo.");
	handleBuddyChanged(user, "echo", "Echo", std::vector<std::string>(), pbnetwork::STATUS_ONLINE);
}

void Plugin::handleLogoutRequest(const std::string &user, const std::string &legacyName) {
}

void Plugin::handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml, const std::string &id) {
	LOG4CXX_INFO(logger, "Sending message from " << user << " to " << legacyName << ".");
	if (legacyName == "echo") {
		handleMessage(user, legacyName, message);
	}
}

void Plugin::handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) {
	LOG4CXX_INFO(logger, user << ": Added buddy " << buddyName << ".");
	handleBuddyChanged(user, buddyName, alias, groups, pbnetwork::STATUS_ONLINE);
}

void Plugin::handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) {

}
