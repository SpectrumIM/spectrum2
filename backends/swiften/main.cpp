// Transport includes
#include "transport/config.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"

// Swiften
#include "Swiften/Swiften.h"

#ifndef WIN32
// for signal handler
#include "unistd.h"
#include "signal.h"
#include "sys/wait.h"
#include "sys/signal.h"
#endif

// malloc_trim
#include "malloc.h"

// Boost
#include <boost/algorithm/string.hpp>
using namespace boost::filesystem;
using namespace boost::program_options;
using namespace Transport;

DEFINE_LOGGER(logger, "Swiften");

// eventloop
Swift::SimpleEventLoop *loop_;

// Plugins
class SwiftenPlugin;
SwiftenPlugin *np = NULL;

class SwiftenPlugin : public NetworkPlugin {
	public:
		Swift::BoostNetworkFactories *m_factories;
		Swift::BoostIOServiceThread m_boostIOServiceThread;
		boost::shared_ptr<Swift::Connection> m_conn;

		SwiftenPlugin(Config *config, Swift::SimpleEventLoop *loop, const std::string &host, int port) : NetworkPlugin() {
			this->config = config;
			m_factories = new Swift::BoostNetworkFactories(loop);
			m_conn = m_factories->getConnectionFactory()->createConnection();
			m_conn->onDataRead.connect(boost::bind(&SwiftenPlugin::_handleDataRead, this, _1));
			m_conn->connect(Swift::HostAddressPort(Swift::HostAddress(host), port));

			LOG4CXX_INFO(logger, "Starting the plugin.");
		}

		// NetworkPlugin uses this method to send the data to networkplugin server
		void sendData(const std::string &string) {
			m_conn->write(Swift::createSafeByteArray(string));
		}

		// This method has to call handleDataRead with all received data from network plugin server
		void _handleDataRead(boost::shared_ptr<Swift::SafeByteArray> data) {
			std::string d(data->begin(), data->end());
			handleDataRead(d);
		}

		void handleSwiftDisconnected(const std::string &user, const boost::optional<Swift::ClientError> &error) {
			std::string message = "";
			if (error) {
				switch(error->getType()) {
					case Swift::ClientError::UnknownError: message = ("Unknown Error"); break;
					case Swift::ClientError::DomainNameResolveError: message = ("Unable to find server"); break;
					case Swift::ClientError::ConnectionError: message = ("Error connecting to server"); break;
					case Swift::ClientError::ConnectionReadError: message = ("Error while receiving server data"); break;
					case Swift::ClientError::ConnectionWriteError: message = ("Error while sending data to the server"); break;
					case Swift::ClientError::XMLError: message = ("Error parsing server data"); break;
					case Swift::ClientError::AuthenticationFailedError: message = ("Login/password invalid"); break;
					case Swift::ClientError::CompressionFailedError: message = ("Error while compressing stream"); break;
					case Swift::ClientError::ServerVerificationFailedError: message = ("Server verification failed"); break;
					case Swift::ClientError::NoSupportedAuthMechanismsError: message = ("Authentication mechanisms not supported"); break;
					case Swift::ClientError::UnexpectedElementError: message = ("Unexpected response"); break;
					case Swift::ClientError::ResourceBindError: message = ("Error binding resource"); break;
					case Swift::ClientError::SessionStartError: message = ("Error starting session"); break;
					case Swift::ClientError::StreamError: message = ("Stream error"); break;
					case Swift::ClientError::TLSError: message = ("Encryption error"); break;
					case Swift::ClientError::ClientCertificateLoadError: message = ("Error loading certificate (Invalid password?)"); break;
					case Swift::ClientError::ClientCertificateError: message = ("Certificate not authorized"); break;

					case Swift::ClientError::UnknownCertificateError: message = ("Unknown certificate"); break;
					case Swift::ClientError::CertificateExpiredError: message = ("Certificate has expired"); break;
					case Swift::ClientError::CertificateNotYetValidError: message = ("Certificate is not yet valid"); break;
					case Swift::ClientError::CertificateSelfSignedError: message = ("Certificate is self-signed"); break;
					case Swift::ClientError::CertificateRejectedError: message = ("Certificate has been rejected"); break;
					case Swift::ClientError::CertificateUntrustedError: message = ("Certificate is not trusted"); break;
					case Swift::ClientError::InvalidCertificatePurposeError: message = ("Certificate cannot be used for encrypting your connection"); break;
					case Swift::ClientError::CertificatePathLengthExceededError: message = ("Certificate path length constraint exceeded"); break;
					case Swift::ClientError::InvalidCertificateSignatureError: message = ("Invalid certificate signature"); break;
					case Swift::ClientError::InvalidCAError: message = ("Invalid Certificate Authority"); break;
					case Swift::ClientError::InvalidServerIdentityError: message = ("Certificate does not match the host identity"); break;
				}
			}
			LOG4CXX_INFO(logger, user << ": Disconnected " << message);
			handleDisconnected(user, 3, message);

			boost::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				client->onConnected.disconnect(boost::bind(&SwiftenPlugin::handleSwiftConnected, this, user));
				client->onDisconnected.disconnect(boost::bind(&SwiftenPlugin::handleSwiftDisconnected, this, user, _1));
				client->onMessageReceived.disconnect(boost::bind(&SwiftenPlugin::handleSwiftMessageReceived, this, user, _1));
				m_users.erase(user);
			}

#ifndef WIN32
			// force returning of memory chunks allocated by libxml2 to kernel
			malloc_trim(0);
#endif
		}

		void handleSwiftConnected(const std::string &user) {
			LOG4CXX_INFO(logger, user << ": Connected to XMPP server.");
			handleConnected(user);
			m_users[user]->requestRoster();
			Swift::Presence::ref response = Swift::Presence::create();
			response->setFrom(m_users[user]->getJID());
			m_users[user]->sendPresence(response);
		}

		void handleSwiftRosterReceived(const std::string &user) {
			Swift::PresenceOracle *oracle = m_users[user]->getPresenceOracle();
			BOOST_FOREACH(const Swift::XMPPRosterItem &item, m_users[user]->getRoster()->getItems()) {
				Swift::Presence::ref lastPresence = oracle->getLastPresence(item.getJID());
				pbnetwork::StatusType status = lastPresence ? ((pbnetwork::StatusType) lastPresence->getShow()) : pbnetwork::STATUS_NONE;
				handleBuddyChanged(user, item.getJID().toBare().toString(),
								   item.getName(), item.getGroups(), status);
			}
		}

		void handleSwiftPresenceChanged(const std::string &user, Swift::Presence::ref presence) {
			LOG4CXX_INFO(logger, user << ": " << presence->getFrom().toBare().toString() << " presence changed");

			std::string message = presence->getStatus();
			std::string photo = "";

			boost::shared_ptr<Swift::VCardUpdate> update = presence->getPayload<Swift::VCardUpdate>();
			if (update) {
				photo = update->getPhotoHash();
			}

			boost::optional<Swift::XMPPRosterItem> item = m_users[user]->getRoster()->getItem(presence->getFrom());
			if (item) {
				handleBuddyChanged(user, presence->getFrom().toBare().toString(), item->getName(), item->getGroups(), (pbnetwork::StatusType) presence->getShow(), message, photo);
			}
			else {
				std::vector<std::string> groups;
				handleBuddyChanged(user, presence->getFrom().toBare().toString(), presence->getFrom().toBare(), groups, (pbnetwork::StatusType) presence->getShow(), message, photo);
			}
		}

		void handleSwiftMessageReceived(const std::string &user, Swift::Message::ref message) {
			std::string body = message->getBody();
			boost::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				handleMessage(user, message->getFrom().toBare().toString(), body, "", "");
			}
		}

		void handleSwiftVCardReceived(const std::string &user, unsigned int id, Swift::VCard::ref vcard, Swift::ErrorPayload::ref error) {
			if (error || !vcard) {
				LOG4CXX_INFO(logger, user << ": error fetching VCard with id=" << id);
				handleVCard(user, id, "", "", "", "");
				return;
			}
			LOG4CXX_INFO(logger, user << ": VCard fetched - id=" << id);
			std::string photo((const char *)&vcard->getPhoto()[0], vcard->getPhoto().size());
			handleVCard(user, id, vcard->getFullName(), vcard->getFullName(), vcard->getNickname(), photo);
		}

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
			LOG4CXX_INFO(logger, user << ": connecting as " << legacyName);
			boost::shared_ptr<Swift::Client> client = boost::make_shared<Swift::Client>(Swift::JID(legacyName), password, m_factories);
			m_users[user] = client;
			client->setAlwaysTrustCertificates();
			client->onConnected.connect(boost::bind(&SwiftenPlugin::handleSwiftConnected, this, user));
			client->onDisconnected.connect(boost::bind(&SwiftenPlugin::handleSwiftDisconnected, this, user, _1));
			client->onMessageReceived.connect(boost::bind(&SwiftenPlugin::handleSwiftMessageReceived, this, user, _1));
			client->getRoster()->onInitialRosterPopulated.connect(boost::bind(&SwiftenPlugin::handleSwiftRosterReceived, this, user));
			client->getPresenceOracle()->onPresenceChange.connect(boost::bind(&SwiftenPlugin::handleSwiftPresenceChanged, this, user, _1));
			Swift::ClientOptions opt;
			opt.allowPLAINWithoutTLS = true;
			client->connect(opt);
		}

		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
			boost::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				client->onConnected.disconnect(boost::bind(&SwiftenPlugin::handleSwiftConnected, this, user));
// 				client->onDisconnected.disconnect(boost::bind(&SwiftenPlugin::handleSwiftDisconnected, this, user, _1));
				client->onMessageReceived.disconnect(boost::bind(&SwiftenPlugin::handleSwiftMessageReceived, this, user, _1));
				client->getRoster()->onInitialRosterPopulated.disconnect(boost::bind(&SwiftenPlugin::handleSwiftRosterReceived, this, user));
				client->getPresenceOracle()->onPresenceChange.disconnect(boost::bind(&SwiftenPlugin::handleSwiftPresenceChanged, this, user, _1));
				client->disconnect();
			}
		}

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &msg, const std::string &xhtml = "") {
			LOG4CXX_INFO(logger, "Sending message from " << user << " to " << legacyName << ".");
			boost::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				boost::shared_ptr<Swift::Message> message(new Swift::Message());
				message->setTo(Swift::JID(legacyName));
				message->setFrom(client->getJID());
				message->setBody(msg);

				client->sendMessage(message);
			}
		}

		void handleVCardRequest(const std::string &user, const std::string &legacyName, unsigned int id) {
			boost::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				LOG4CXX_INFO(logger, user << ": fetching VCard of " << legacyName << " id=" << id);
				Swift::GetVCardRequest::ref request = Swift::GetVCardRequest::create(Swift::JID(legacyName), client->getIQRouter());
				request->onResponse.connect(boost::bind(&SwiftenPlugin::handleSwiftVCardReceived, this, user, id, _1, _2));
				request->send();
			}
		}

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) {
			LOG4CXX_INFO(logger, user << ": Added/Updated buddy " << buddyName << ".");
// 			handleBuddyChanged(user, buddyName, alias, groups, pbnetwork::STATUS_ONLINE);
		}

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) {

		}

	private:
		Config *config;
		std::map<std::string, boost::shared_ptr<Swift::Client> > m_users;
};

#ifndef WIN32
static void spectrum_sigchld_handler(int sig)
{
	int status;
	pid_t pid;

	do {
		pid = waitpid(-1, &status, WNOHANG);
	} while (pid != 0 && pid != (pid_t)-1);

	if ((pid == (pid_t) - 1) && (errno != ECHILD)) {
		char errmsg[BUFSIZ];
		snprintf(errmsg, BUFSIZ, "Warning: waitpid() returned %d", pid);
		perror(errmsg);
	}
}
#endif


int main (int argc, char* argv[]) {
	std::string host;
	int port;

#ifndef WIN32
	if (signal(SIGCHLD, spectrum_sigchld_handler) == SIG_ERR) {
		std::cout << "SIGCHLD handler can't be set\n";
		return -1;
	}
#endif

	std::string configFile;
	boost::program_options::variables_map vm;
	boost::program_options::options_description desc("Usage: spectrum <config_file.cfg>\nAllowed options");
	desc.add_options()
		("help", "help")
		("host,h", boost::program_options::value<std::string>(&host)->default_value(""), "Host to connect to")
		("port,p", boost::program_options::value<int>(&port)->default_value(10000), "Port to connect to")
		("config", boost::program_options::value<std::string>(&configFile)->default_value(""), "Config file")
		;

	try
	{
		boost::program_options::positional_options_description p;
		p.add("config", -1);
		boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
			options(desc).positional(p).allow_unregistered().run(), vm);
		boost::program_options::notify(vm);
			
		if(vm.count("help"))
		{
			std::cout << desc << "\n";
			return 1;
		}

		if(vm.count("config") == 0) {
			std::cout << desc << "\n";
			return 1;
		}
	}
	catch (std::runtime_error& e)
	{
		std::cout << desc << "\n";
		return 1;
	}
	catch (...)
	{
		std::cout << desc << "\n";
		return 1;
	}

	Config config(argc, argv);
	if (!config.load(configFile)) {
		std::cerr << "Can't open " << argv[1] << " configuration file.\n";
		return 1;
	}

	Logging::initBackendLogging(&config);

	Swift::SimpleEventLoop eventLoop;
	loop_ = &eventLoop;
	np = new SwiftenPlugin(&config, &eventLoop, host, port);
	loop_->run();

	return 0;
}
