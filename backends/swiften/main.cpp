// Transport includes
#include "transport/config.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"

#include "boost/date_time/posix_time/posix_time.hpp"

// Swiften
#include "Swiften/Swiften.h"

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

// eventloop
Swift::SimpleEventLoop *loop_;

// Plugins
class SwiftenPlugin;
NetworkPlugin *np = NULL;

class MUCController {
	public:
		MUCController(const std::string &user, boost::shared_ptr<Swift::Client> client, const std::string &room, const std::string &nickname, const std::string &password) {
			m_user = user;
			m_room = room;
			muc = client->getMUCManager()->createMUC(room);
			if (!password.empty()) {
				muc->setPassword(password);
			}

			muc->onJoinComplete.connect(boost::bind(&MUCController::handleJoinComplete, this, _1));
			muc->onJoinFailed.connect(boost::bind(&MUCController::handleJoinFailed, this, _1));
			muc->onOccupantJoined.connect(boost::bind(&MUCController::handleOccupantJoined, this, _1));
			muc->onOccupantPresenceChange.connect(boost::bind(&MUCController::handleOccupantPresenceChange, this, _1));
			muc->onOccupantLeft.connect(boost::bind(&MUCController::handleOccupantLeft, this, _1, _2, _3));
			muc->onOccupantRoleChanged.connect(boost::bind(&MUCController::handleOccupantRoleChanged, this, _1, _2, _3));
			muc->onOccupantAffiliationChanged.connect(boost::bind(&MUCController::handleOccupantAffiliationChanged, this, _1, _2, _3));

			muc->joinAs(nickname);
		}

		virtual ~MUCController() {
			muc->onJoinComplete.disconnect(boost::bind(&MUCController::handleJoinComplete, this, _1));
			muc->onJoinFailed.disconnect(boost::bind(&MUCController::handleJoinFailed, this, _1));
			muc->onOccupantJoined.disconnect(boost::bind(&MUCController::handleOccupantJoined, this, _1));
			muc->onOccupantPresenceChange.disconnect(boost::bind(&MUCController::handleOccupantPresenceChange, this, _1));
			muc->onOccupantLeft.disconnect(boost::bind(&MUCController::handleOccupantLeft, this, _1, _2, _3));
			muc->onOccupantRoleChanged.disconnect(boost::bind(&MUCController::handleOccupantRoleChanged, this, _1, _2, _3));
			muc->onOccupantAffiliationChanged.disconnect(boost::bind(&MUCController::handleOccupantAffiliationChanged, this, _1, _2, _3));
		}

		const std::string &getNickname() {
			//return muc->getCurrentNick();
			return m_nick;
		}

		void handleOccupantJoined(const Swift::MUCOccupant& occupant) {
			np->handleParticipantChanged(m_user, occupant.getNick(), m_room, occupant.getRole() == Swift::MUCOccupant::Moderator, pbnetwork::STATUS_ONLINE);
		}

		void handleOccupantLeft(const Swift::MUCOccupant& occupant, Swift::MUC::LeavingType type, const std::string& reason) {
			np->handleParticipantChanged(m_user, occupant.getNick(), m_room, occupant.getRole() == Swift::MUCOccupant::Moderator, pbnetwork::STATUS_NONE);
		}

		void handleOccupantPresenceChange(boost::shared_ptr<Swift::Presence> presence) {
			const Swift::MUCOccupant& occupant = muc->getOccupant(presence->getFrom().getResource());
			np->handleParticipantChanged(m_user, presence->getFrom().getResource(), m_room, (int) occupant.getRole() == Swift::MUCOccupant::Moderator, (pbnetwork::StatusType) presence->getShow(), presence->getStatus());
		}

		void handleOccupantRoleChanged(const std::string& nick, const Swift::MUCOccupant& occupant, const Swift::MUCOccupant::Role& oldRole) {

		}

		void handleOccupantAffiliationChanged(const std::string& nick, const Swift::MUCOccupant::Affiliation& affiliation, const Swift::MUCOccupant::Affiliation& oldAffiliation) {
// 			np->handleParticipantChanged(m_user, occupant->getNick(), m_room, (int) occupant.getRole() == Swift::MUCOccupant::Moderator, pbnetwork::STATUS_ONLINE);
		}

		void handleJoinComplete(const std::string& nick) {
			m_nick = nick;
		}

		void handleJoinFailed(boost::shared_ptr<Swift::ErrorPayload> error) {
			
		}

		void part() {
			muc->part();
		}

	private:
		Swift::MUC::ref muc;
		std::string m_user;
		std::string m_room;
		std::string m_nick;
};

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
				}
			}
			LOG4CXX_INFO(logger, user << ": Disconnected " << message);
			handleDisconnected(user, reconnect ? 0 : 3, message);

			boost::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				client->onConnected.disconnect(boost::bind(&SwiftenPlugin::handleSwiftConnected, this, user));
				client->onDisconnected.disconnect(boost::bind(&SwiftenPlugin::handleSwiftDisconnected, this, user, _1));
				client->onMessageReceived.disconnect(boost::bind(&SwiftenPlugin::handleSwiftMessageReceived, this, user, _1));
				m_users.erase(user);
				m_mucs.erase(user);
			}

#ifndef WIN32
#ifndef __FreeBSD__
#ifndef __MACH__
			// force returning of memory chunks allocated by libxml2 to kernel
			malloc_trim(0);
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
			boost::shared_ptr<Swift::Client> client = m_users[user];
			if (client->getMUCRegistry()->isMUC(presence->getFrom().toBare())) {
				return;
			}

			if (presence->getPayload<Swift::MUCUserPayload>() != NULL || presence->getPayload<Swift::MUCPayload>() != NULL) {
				return;
			}

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
				if (message->getType() == Swift::Message::Groupchat) {
					boost::shared_ptr<Swift::Delay> delay = message->getPayload<Swift::Delay>();
					std::string timestamp = "";
					if (delay) {
						timestamp = boost::posix_time::to_iso_string(delay->getStamp());
					}
					handleMessage(user, message->getFrom().toBare().toString(), body, message->getFrom().getResource(), "", timestamp);
				}
				else {
					if (client->getMUCRegistry()->isMUC(message->getFrom().toBare())) {
						handleMessage(user, message->getFrom().toBare().toString(), body, message->getFrom().getResource(), "", "", false, true);
					}
					else {
						handleMessage(user, message->getFrom().toBare().toString(), body, "", "");
					}
				}
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
				m_mucs.erase(user);
				m_users.erase(user);
			}
		}

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &msg, const std::string &xhtml = "", const std::string &id = "") {
			LOG4CXX_INFO(logger, "Sending message from " << user << " to " << legacyName << ".");
			boost::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				boost::shared_ptr<Swift::Message> message(new Swift::Message());
				message->setTo(Swift::JID(legacyName));
				message->setFrom(client->getJID());
				message->setBody(msg);
				if (client->getMUCRegistry()->isMUC(legacyName)) {
					message->setType(Swift::Message::Groupchat);
					boost::shared_ptr<MUCController> muc = m_mucs[user][legacyName];
// 					handleMessage(user, legacyName, msg, muc->getNickname(), xhtml);
				}

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
			boost::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				LOG4CXX_INFO(logger, user << ": Added/Updated buddy " << buddyName << ".");
				if (!client->getRoster()->containsJID(buddyName)) {
					Swift::RosterItemPayload item;
					item.setName(alias);
					item.setJID(buddyName);
					item.setGroups(groups);
					boost::shared_ptr<Swift::RosterPayload> roster(new Swift::RosterPayload());
					roster->addItem(item);
					Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(roster, client->getIQRouter());
// 					request->onResponse.connect(boost::bind(&RosterController::handleRosterSetError, this, _1, roster));
					request->send();
					client->getSubscriptionManager()->requestSubscription(buddyName);
				}
				else {
					Swift::JID contact(buddyName);
					Swift::RosterItemPayload item(contact, alias, client->getRoster()->getSubscriptionStateForJID(contact));
					item.setGroups(groups);
					boost::shared_ptr<Swift::RosterPayload> roster(new Swift::RosterPayload());
					roster->addItem(item);
					Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(roster, client->getIQRouter());
// 					request->onResponse.connect(boost::bind(&RosterController::handleRosterSetError, this, _1, roster));
					request->send();
				}

			}
		}

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) {
			boost::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				Swift::RosterItemPayload item(buddyName, "", Swift::RosterItemPayload::Remove);
				boost::shared_ptr<Swift::RosterPayload> roster(new Swift::RosterPayload());
				roster->addItem(item);
				Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(roster, client->getIQRouter());
// 				request->onResponse.connect(boost::bind(&RosterController::handleRosterSetError, this, _1, roster));
				request->send();
			}
		}

		void handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password) {
			boost::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				if (client->getMUCRegistry()->isMUC(room)) {
					return;
				}

				boost::shared_ptr<MUCController> muc = boost::shared_ptr<MUCController>( new MUCController(user, client, room, nickname, password));
				m_mucs[user][room] = muc;
			}
		}

		void handleLeaveRoomRequest(const std::string &user, const std::string &room) {
			boost::shared_ptr<Swift::Client> client = m_users[user];
			if (client) {
				if (!client->getMUCRegistry()->isMUC(room)) {
					return;
				}

				boost::shared_ptr<MUCController> muc = m_mucs[user][room];
				if (!muc) {
					m_mucs[user].erase(room);
					return;
				}

				muc->part();
				m_mucs[user].erase(room);
			}
		}

	private:
		Config *config;
		std::map<std::string, boost::shared_ptr<Swift::Client> > m_users;
		std::map<std::string, std::map<std::string, boost::shared_ptr<MUCController> > > m_mucs;
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
