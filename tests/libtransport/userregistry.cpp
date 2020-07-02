#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <Swiften/EventLoop/DummyEventLoop.h>
#include <Swiften/Server/Server.h>
#include <Swiften/Network/DummyNetworkFactories.h>
#include <Swiften/Network/DummyConnectionServer.h>
#include <Swiften/Network/ConnectionFactory.h>
#include <Swiften/Network/DummyTimerFactory.h>
#include "basictest.h"

using namespace Transport;

class UserRegistryTest : public CPPUNIT_NS :: TestFixture {
	CPPUNIT_TEST_SUITE(UserRegistryTest);
	CPPUNIT_TEST(login);
	CPPUNIT_TEST(loginInvalidPassword);
	CPPUNIT_TEST(loginTwoClientsValidPasswords);
	CPPUNIT_TEST(loginTwoClientsDifferentPasswords);
	CPPUNIT_TEST(loginDisconnect);
	CPPUNIT_TEST(removeLater);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
			state1 = Init;
			state2 = Init;
			std::istringstream ifs;
			cfg = new Config();
			cfg->load(ifs);

			loop = new Swift::DummyEventLoop();
			factories = new Swift::DummyNetworkFactories(loop);

			userRegistry = new UserRegistry(cfg, factories);
			userRegistry->onConnectUser.connect(bind(&UserRegistryTest::handleConnectUser, this, _1));
			userRegistry->onDisconnectUser.connect(bind(&UserRegistryTest::handleDisconnectUser, this, _1));

			server = new Swift::Server(loop, factories, userRegistry, "localhost", "0.0.0.0", 5222);
			server->start();
			connectionServer = server->getConnectionServer();

			client1 = factories->getConnectionFactory()->createConnection();
			SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::DummyConnectionServer>(connectionServer)->acceptConnection(client1);

			SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::DummyConnection>(client1)->onDataSent.connect(boost::bind(&UserRegistryTest::handleDataReceived, this, _1, client1));

			client2 = factories->getConnectionFactory()->createConnection();
			SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::DummyConnectionServer>(connectionServer)->acceptConnection(client2);

			SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::DummyConnection>(client2)->onDataSent.connect(boost::bind(&UserRegistryTest::handleDataReceived, this, _1, client2));

			loop->processEvents();
		}

		void tearDown (void) {
			delete server;
			SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::DummyConnection>(client1)->onDataSent.disconnect_all_slots();
			client1.reset();
			SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::DummyConnection>(client2)->onDataSent.disconnect_all_slots();
			client2.reset();
			connectionServer.reset();
			delete userRegistry;
			delete factories;
			delete loop;
			delete cfg;
			received1.clear();
			received2.clear();
		}

		void send(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> conn, const std::string &data) {
			SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::DummyConnection>(conn)->receive(Swift::createSafeByteArray(data));
			loop->processEvents();
		}

		void sendCredentials(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> conn, const std::string &username, const std::string &password, const std::string &b64) {
			std::vector<std::string> &received = conn == client1 ? received1 : received2;
			send(conn, "<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='localhost' version='1.0'>");
			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());
			CPPUNIT_ASSERT(received[0].find("<?xml version=\"1.0\"?>") == 0);
			CPPUNIT_ASSERT(received[1].find("PLAIN") != std::string::npos);
			received.clear();

			// username:test
			send(conn, "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='PLAIN'>" + b64 + "</auth>");
			if (conn == client1)
				CPPUNIT_ASSERT_EQUAL(Connecting, state1);
// 			else
// 				CPPUNIT_ASSERT_EQUAL(Connecting, state2);
			CPPUNIT_ASSERT_EQUAL(password, userRegistry->getUserPassword(username));
			CPPUNIT_ASSERT_EQUAL(std::string(""), userRegistry->getUserPassword("unknown@localhost"));
		}

		void bindSession(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> conn) {
			std::vector<std::string> &received = conn == client1 ? received1 : received2;

			send(conn, "<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='localhost' version='1.0'>");
			CPPUNIT_ASSERT_EQUAL(2, (int) received.size());
			CPPUNIT_ASSERT(received[0].find("<?xml version=\"1.0\"?>") == 0);
			CPPUNIT_ASSERT(received[1].find("urn:ietf:params:xml:ns:xmpp-bind") != std::string::npos);
			CPPUNIT_ASSERT(received[1].find("urn:ietf:params:xml:ns:xmpp-session") != std::string::npos);
			
		}

		void handleDataReceived(const Swift::SafeByteArray &data, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> conn) {
			if (conn == client1) {
				received1.push_back(safeByteArrayToString(data));
// 				std::cout << received1.back() << "\n";
			}
			else {
				received2.push_back(safeByteArrayToString(data));
// 				std::cout << received2.back() << "\n";
			}
		}

		void handleConnectUser(const Swift::JID &user) {
			state1 = Connecting;
		}

		void handleDisconnectUser(const Swift::JID &user) {
			state1 = Disconnected;
		}

		void login() {
			sendCredentials(client1, "username@localhost", "test", "AHVzZXJuYW1lAHRlc3Q=");

			userRegistry->onPasswordValid("username@localhost");
			loop->processEvents();
			CPPUNIT_ASSERT_EQUAL(std::string(""), userRegistry->getUserPassword("username@localhost"));
			CPPUNIT_ASSERT_EQUAL(1, (int) received1.size());
			CPPUNIT_ASSERT_EQUAL(std::string("<success xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"></success>"), received1[0]);
			received1.clear();

			bindSession(client1);
			
		}

		void loginInvalidPassword() {
			sendCredentials(client1, "username@localhost", "test", "AHVzZXJuYW1lAHRlc3Q=");

			userRegistry->onPasswordInvalid("username@localhost");
			loop->processEvents();
			CPPUNIT_ASSERT_EQUAL(std::string(""), userRegistry->getUserPassword("username@localhost"));
			CPPUNIT_ASSERT_EQUAL(2, (int) received1.size());
			CPPUNIT_ASSERT_EQUAL(std::string("<failure xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"/>"), received1[0]);
			CPPUNIT_ASSERT_EQUAL(std::string("</stream:stream>"), received1[1]);
		}

		void loginTwoClientsValidPasswords() {
			sendCredentials(client1, "username@localhost", "test", "AHVzZXJuYW1lAHRlc3Q=");
			sendCredentials(client2, "username@localhost", "test", "AHVzZXJuYW1lAHRlc3Q=");
			CPPUNIT_ASSERT_EQUAL(2, (int) received1.size());
			CPPUNIT_ASSERT_EQUAL(0, (int) received2.size());
			CPPUNIT_ASSERT_EQUAL(std::string("<failure xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"/>"), received1[0]);
			CPPUNIT_ASSERT_EQUAL(std::string("</stream:stream>"), received1[1]);

			userRegistry->onPasswordValid("username@localhost");
			loop->processEvents();
			CPPUNIT_ASSERT_EQUAL(std::string(""), userRegistry->getUserPassword("username@localhost"));
			CPPUNIT_ASSERT_EQUAL(1, (int) received2.size());
			CPPUNIT_ASSERT_EQUAL(std::string("<success xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"></success>"), received2[0]);
		}

		void loginTwoClientsDifferentPasswords() {
			// first is valid, second invalid.
			sendCredentials(client1, "username@localhost", "test", "AHVzZXJuYW1lAHRlc3Q=");
			sendCredentials(client2, "username@localhost", "test2", "AHVzZXJuYW1lAHRlc3Qy");
			CPPUNIT_ASSERT_EQUAL(2, (int) received1.size());
			CPPUNIT_ASSERT_EQUAL(0, (int) received2.size());
			CPPUNIT_ASSERT_EQUAL(std::string("<failure xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"/>"), received1[0]);
			CPPUNIT_ASSERT_EQUAL(std::string("</stream:stream>"), received1[1]);
			userRegistry->onPasswordValid("username@localhost");
			loop->processEvents();
			CPPUNIT_ASSERT_EQUAL(std::string(""), userRegistry->getUserPassword("username@localhost"));
			CPPUNIT_ASSERT_EQUAL(1, (int) received2.size());
			CPPUNIT_ASSERT_EQUAL(std::string("<success xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"></success>"), received2[0]);
		}

		void loginDisconnect() {
			sendCredentials(client1, "username@localhost", "test", "AHVzZXJuYW1lAHRlc3Q=");

			client1->onDisconnected(boost::optional<Swift::Connection::Error>());
			loop->processEvents();
			CPPUNIT_ASSERT_EQUAL(Disconnected, state1);
		}

		void removeLater() {
			sendCredentials(client1, "username@localhost", "test", "AHVzZXJuYW1lAHRlc3Q=");

			userRegistry->removeLater("username@localhost");
			loop->processEvents();
			CPPUNIT_ASSERT_EQUAL(0, (int) received1.size());

			dynamic_cast<Swift::DummyTimerFactory *>(factories->getTimerFactory())->setTime(10);
			loop->processEvents();

			CPPUNIT_ASSERT_EQUAL(2, (int) received1.size());
			CPPUNIT_ASSERT_EQUAL(std::string("<failure xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"/>"), received1[0]);
			CPPUNIT_ASSERT_EQUAL(std::string("</stream:stream>"), received1[1]);
		}
		
		

	private:
		typedef enum {
			Init,
			Connecting,
			Disconnected,
		} State;

		UserRegistry *userRegistry;
		Config *cfg;
		Swift::Server *server;
		Swift::DummyNetworkFactories *factories;
		Swift::DummyEventLoop *loop;
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::ConnectionServer> connectionServer;
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> client1;
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> client2;
		std::vector<std::string> received1;
		std::vector<std::string> received2;
		State state1;
		State state2;

};

// magic line to support unity build, do not remove me
CPPUNIT_TEST_SUITE_REGISTRATION (UserRegistryTest);
