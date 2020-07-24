// Transport includes
#include "transport/Config.h"
#include "transport/NetworkPlugin.h"
#include "transport/Logging.h"

#include "boost/date_time/posix_time/posix_time.hpp"

// Swiften
#include "Swiften/Swiften.h"
#include "Swiften/SwiftenCompat.h"
#include <Swiften/Version.h>
#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)

#ifndef WIN32
// for signal handler
#include "unistd.h"
#include "signal.h"
#include "sys/wait.h"
#include "sys/signal.h"
#endif

#ifndef __FreeBSD__
#ifndef __MACH__
// malloc_trim
#include "malloc.h"
#endif
#endif

// Boost
#include <boost/algorithm/string.hpp>
using namespace boost::filesystem;
using namespace boost::program_options;
using namespace Transport;

DEFINE_LOGGER(logger, "Swiften");
DEFINE_LOGGER(logger_xml, "backend.xml");

// eventloop
Swift::SimpleEventLoop *loop_;

// Plugins
class SwiftenPlugin;
NetworkPlugin *np = NULL;
Swift::XMPPSerializer *serializer;

class ForwardIQHandler : public Swift::IQHandler {
	public:
		std::map <std::string, std::string> m_id2resource;

		ForwardIQHandler(NetworkPlugin *np, const std::string &user) {
			m_np = np;
			m_user = user;
		}

		bool handleIQ(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::IQ> iq) {
			if (iq->getPayload<Swift::RosterPayload>() != NULL) {
				return false;
			}
			if (iq->getType() == Swift::IQ::Get) {
				m_id2resource[iq->getID()] = iq->getFrom().getResource();
			}

			iq->setTo(m_user);
			std::string xml = safeByteArrayToString(serializer->serializeElement(iq));
			m_np->sendRawXML(xml);
			return true;
		}

	private:
		NetworkPlugin *m_np;
		std::string m_user;

};

class SwiftenPlugin : public NetworkPlugin, Swift::XMPPParserClient {
	public:
		Swift::BoostNetworkFactories *m_factories;
		Swift::BoostIOServiceThread m_boostIOServiceThread;
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> m_conn;
		bool m_firstPing;

		Swift::FullPayloadSerializerCollection collection;
		Swift::XMPPParser *m_xmppParser;
		Swift::FullPayloadParserFactoryCollection m_collection2;

		SwiftenPlugin(Config *config, Swift::SimpleEventLoop *loop, const std::string &host, int port) : NetworkPlugin() {
			this->config = config;
			m_firstPing = true;
			m_factories = new Swift::BoostNetworkFactories(loop);
			m_conn = m_factories->getConnectionFactory()->createConnection();
			m_conn->onDataRead.connect(boost::bind(&SwiftenPlugin::_handleDataRead, this, _1));
			m_conn->connect(Swift::HostAddressPort(SWIFT_HOSTADDRESS(host), port));
#if HAVE_SWIFTEN_3
			serializer = new Swift::XMPPSerializer(&collection, Swift::ClientStreamType, false);
#else
			serializer = new Swift::XMPPSerializer(&collection, Swift::ClientStreamType);
#endif
			m_xmppParser = new Swift::XMPPParser(this, &m_collection2, m_factories->getXMLParserFactory());
			m_xmppParser->parse("<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='localhost' version='1.0'>");

			LOG4CXX_INFO(logger, "Starting the plugin.");
		}

		// NetworkPlugin uses this method to send the data to networkplugin server
		void sendData(const std::string &string) {
			m_conn->write(Swift::createSafeByteArray(string));
		}

		// This method has to call handleDataRead with all received data from network plugin server
		void _handleDataRead(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::SafeByteArray> data) {
			if (m_firstPing) {
				m_firstPing = false;
				NetworkPlugin::PluginConfig cfg;
				cfg.setRawXML(true);
				cfg.setNeedRegistration(false);
				sendConfig(cfg);
			}
			std::string d(data->begin(), data->end());
			handleDataRead(d);
		}

		void handleStreamStart(const Swift::ProtocolHeader&) {}
#if HAVE_SWIFTEN_3
		void handleElement(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::ToplevelElement> element) {
#else
		void handleElement(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Element> element) {
#endif
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Stanza> stanza = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::Stanza>(element);
			if (!stanza) {
				return;
			}

			std::string user = stanza->getFrom().toBare();

			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Client> client = m_users[user];
			if (!client)
				return;

			stanza->setFrom(client->getJID());

			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> message = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::Message>(stanza);
			if (message) {
				client->sendMessage(message);
				return;
			}

			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Presence> presence = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::Presence>(stanza);
			if (presence) {
				client->sendPresence(presence);
				return;
			}

			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::IQ> iq = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::IQ>(stanza);
			if (iq) {
				if (m_handlers[user]->m_id2resource.find(stanza->getID()) != m_handlers[user]->m_id2resource.end()) {
					std::string resource = m_handlers[user]->m_id2resource[stanza->getID()];
					if (resource.empty()) {
						iq->setTo(Swift::JID(iq->getTo().getNode(), iq->getTo().getDomain()));
					} else {
						iq->setTo(Swift::JID(iq->getTo().getNode(), iq->getTo().getDomain(), resource));
					}

					m_handlers[user]->m_id2resource.erase(stanza->getID());
				}
				client->getIQRouter()->sendIQ(iq);
				return;
			}
		}

		void handleStreamEnd() {}

		void handleRawXML(const std::string &xml) {
			m_xmppParser->parse(xml);
		}

		void handleSwiftDisconnected(const std::string &user, const boost::optional<Swift::ClientError> &error) {
			std::string message = "";
			bool reconnect = false;
			if (error) {
				switch(error->getType()) {
					case Swift::ClientError::UnknownError: message = ("Unknown Error"); reconnect = true; break;
					case Swift::ClientError::DomainNameResolveError: message = ("Unable to find server"); break;
					case Swift::ClientError::ConnectionError: message = ("Error connecting to server"); break;
					case Swift::ClientError::ConnectionReadError: message = ("Error while receiving server data"); reconnect = true; break;
					case Swift::ClientError::ConnectionWriteError: message = ("Error while sending data to the server"); reconnect = true; break;
					case Swift::ClientError::XMLError: message = ("Error parsing server data"); reconnect = true; break;
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
					case Swift::ClientError::CertificateCardRemoved: message = ("Certificate card has been removed"); break;
					case Swift::ClientError::RevokedError: message = ("Certificate has been revoked"); break;
					case Swift::ClientError::RevocationCheckFailedError: message = ("Certificate revocation check has failed"); break;
				}
			}
			LOG4CXX_INFO(logger, user << ": Disconnected " << message);
			handleDisconnected(user, reconnect ? 0 : 3, message);

			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				client->onConnected.disconnect(boost::bind(&SwiftenPlugin::handleSwiftConnected, this, user));
				client->onDisconnected.disconnect(boost::bind(&SwiftenPlugin::handleSwiftDisconnected, this, user, _1));
				client->onMessageReceived.disconnect(boost::bind(&SwiftenPlugin::handleSwiftMessageReceived, this, user, _1));
				m_users.erase(user);
				m_handlers.erase(user);
			}

#ifndef WIN32
#ifndef __FreeBSD__
#ifndef __MACH__
#if defined (__GLIBC__)
			// force returning of memory chunks allocated by libxml2 to kernel
			malloc_trim(0);
#endif
#endif
#endif
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
//			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Client> client = m_users[user];
//			if (client->getMUCRegistry()->isMUC(presence->getFrom().toBare())) {
//				return;
//			}
//
//			if (presence->getPayload<Swift::MUCUserPayload>() != NULL || presence->getPayload<Swift::MUCPayload>() != NULL) {
//				return;
//			}
//
//			LOG4CXX_INFO(logger, user << ": " << presence->getFrom().toBare().toString() << " presence changed");
//
//			std::string message = presence->getStatus();
//			std::string photo = "";
//
//			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::VCardUpdate> update = presence->getPayload<Swift::VCardUpdate>();
//			if (update) {
//				photo = update->getPhotoHash();
//			}
//
//			boost::optional<Swift::XMPPRosterItem> item = m_users[user]->getRoster()->getItem(presence->getFrom());
//			if (item) {
//				handleBuddyChanged(user, presence->getFrom().toBare().toString(), item->getName(), item->getGroups(), (pbnetwork::StatusType) presence->getShow(), message, photo);
//			}
//			else {
//				std::vector<std::string> groups;
//				handleBuddyChanged(user, presence->getFrom().toBare().toString(), presence->getFrom().toBare(), groups, (pbnetwork::StatusType) presence->getShow(), message, photo);
//			}
			presence->setTo(user);
			std::string xml = safeByteArrayToString(serializer->serializeElement(presence));
			sendRawXML(xml);
		}

		void handleSwiftMessageReceived(const std::string &user, Swift::Message::ref message) {
			message->setTo(user);
			std::string xml = safeByteArrayToString(serializer->serializeElement(message));
			sendRawXML(xml);
		}

		void handleSwiftenDataRead(const Swift::SafeByteArray &data) {
			std::string d = safeByteArrayToString(data);
			if (!boost::starts_with(d, "<auth")) {
				LOG4CXX_INFO(logger_xml, "XML IN " << d);
			}
		}

		void handleSwiftenDataWritten(const Swift::SafeByteArray &data) {
			LOG4CXX_INFO(logger_xml, "XML OUT " << safeByteArrayToString(data));
		}

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
			LOG4CXX_INFO(logger, user << ": connecting as " << legacyName);
			Swift::JID jid(legacyName);
			if (legacyName.find("/") == std::string::npos) {
				jid = Swift::JID(legacyName + "/Spectrum");
			}
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Client> client = SWIFTEN_SHRPTR_NAMESPACE::make_shared<Swift::Client>(jid, password, m_factories);
			m_users[user] = client;
			client->setAlwaysTrustCertificates();
			client->onConnected.connect(boost::bind(&SwiftenPlugin::handleSwiftConnected, this, user));
			client->onDisconnected.connect(boost::bind(&SwiftenPlugin::handleSwiftDisconnected, this, user, _1));
			client->onMessageReceived.connect(boost::bind(&SwiftenPlugin::handleSwiftMessageReceived, this, user, _1));
			client->getRoster()->onInitialRosterPopulated.connect(boost::bind(&SwiftenPlugin::handleSwiftRosterReceived, this, user));
			client->getPresenceOracle()->onPresenceChange.connect(boost::bind(&SwiftenPlugin::handleSwiftPresenceChanged, this, user, _1));
			client->onDataRead.connect(boost::bind(&SwiftenPlugin::handleSwiftenDataRead, this, _1));
			client->onDataWritten.connect(boost::bind(&SwiftenPlugin::handleSwiftenDataWritten, this, _1));
			client->getSubscriptionManager()->onPresenceSubscriptionRequest.connect(boost::bind(&SwiftenPlugin::handleSubscriptionRequest, this, user, _1, _2, _3));
			client->getSubscriptionManager()->onPresenceSubscriptionRevoked.connect(boost::bind(&SwiftenPlugin::handleSubscriptionRevoked, this, user, _1, _2));
			Swift::ClientOptions opt;
			opt.allowPLAINWithoutTLS = true;
			client->connect(opt);

			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ForwardIQHandler> handler = SWIFTEN_SHRPTR_NAMESPACE::make_shared<ForwardIQHandler>(this, user);
			client->getIQRouter()->addHandler(handler);
			m_handlers[user] = handler;
		}

		void handleSubscriptionRequest(const std::string &user, const Swift::JID& jid, const std::string& message, Swift::Presence::ref presence) {
			handleSwiftPresenceChanged(user, presence);
		}

		void handleSubscriptionRevoked(const std::string &user, const Swift::JID& jid, const std::string& message) {
			Swift::Presence::ref presence = Swift::Presence::create();
			presence->setTo(user);
			presence->setFrom(jid);
			presence->setType(Swift::Presence::Unsubscribe);
			handleSwiftPresenceChanged(user, presence);
		}

		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				client->onConnected.disconnect(boost::bind(&SwiftenPlugin::handleSwiftConnected, this, user));
//				client->onDisconnected.disconnect(boost::bind(&SwiftenPlugin::handleSwiftDisconnected, this, user, _1));
				client->onMessageReceived.disconnect(boost::bind(&SwiftenPlugin::handleSwiftMessageReceived, this, user, _1));
				client->getRoster()->onInitialRosterPopulated.disconnect(boost::bind(&SwiftenPlugin::handleSwiftRosterReceived, this, user));
				client->getPresenceOracle()->onPresenceChange.disconnect(boost::bind(&SwiftenPlugin::handleSwiftPresenceChanged, this, user, _1));
				client->disconnect();
			}
		}

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &msg, const std::string &xhtml = "", const std::string &id = "") {
		}

		void handleVCardRequest(const std::string &user, const std::string &legacyName, unsigned int id) {
		}

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) {
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				LOG4CXX_INFO(logger, user << ": Added/Updated buddy " << buddyName << ".");
				if (!client->getRoster()->containsJID(buddyName) || client->getRoster()->getSubscriptionStateForJID(buddyName) != Swift::RosterItemPayload::Both) {
					Swift::RosterItemPayload item;
					item.setName(alias);
					item.setJID(buddyName);
					item.setGroups(groups);
					SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::RosterPayload> roster(new Swift::RosterPayload());
					roster->addItem(item);
					Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(roster, client->getIQRouter());
//					request->onResponse.connect(boost::bind(&RosterController::handleRosterSetError, this, _1, roster));
					request->send();
					client->getSubscriptionManager()->requestSubscription(buddyName);
				}
				else {
					Swift::JID contact(buddyName);
					Swift::RosterItemPayload item(contact, alias, client->getRoster()->getSubscriptionStateForJID(contact));
					item.setGroups(groups);
					SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::RosterPayload> roster(new Swift::RosterPayload());
					roster->addItem(item);
					Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(roster, client->getIQRouter());
//					request->onResponse.connect(boost::bind(&RosterController::handleRosterSetError, this, _1, roster));
					request->send();
				}

			}
		}

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) {
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				Swift::RosterItemPayload item(buddyName, "", Swift::RosterItemPayload::Remove);
				SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::RosterPayload> roster(new Swift::RosterPayload());
				roster->addItem(item);
				Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(roster, client->getIQRouter());
//				request->onResponse.connect(boost::bind(&RosterController::handleRosterSetError, this, _1, roster));
				request->send();
			}
		}

		void handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password) {

		}

		void handleLeaveRoomRequest(const std::string &user, const std::string &room) {

		}

	private:
		Config *config;
		std::map<std::string, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Client> > m_users;
		std::map<std::string, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<ForwardIQHandler> > m_handlers;
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

	std::string error;
	Config *cfg = Config::createFromArgs(argc, argv, error, host, port);
	if (cfg == NULL) {
		std::cerr << error;
		return 1;
	}

	Logging::initBackendLogging(cfg);

	Swift::SimpleEventLoop eventLoop;
	loop_ = &eventLoop;
	np = new SwiftenPlugin(cfg, &eventLoop, host, port);
	loop_->run();

	return 0;
}
