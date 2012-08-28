#include "transport/userregistry.h"
#include "transport/userregistration.h"
#include "transport/config.h"
#include "transport/storagebackend.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/conversation.h"
#include "transport/usermanager.h"
#include "transport/localbuddy.h"
#include "transport/settingsadhoccommand.h"
#include "transport/filetransfermanager.h"
#include "transport/adhocmanager.h"
#include "transport/memoryreadbytestream.h"
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <Swiften/Swiften.h>
#include <Swiften/EventLoop/DummyEventLoop.h>
#include <Swiften/Server/Server.h>
#include <Swiften/Network/DummyNetworkFactories.h>
#include <Swiften/Network/DummyConnectionServer.h>
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Server/ServerFromClientSession.h"
#include "Swiften/Parser/PayloadParsers/FullPayloadParserFactoryCollection.h"
#include "basictest.h"

using namespace Transport;

class FileTransferManagerTest : public CPPUNIT_NS :: TestFixture, public BasicTest {
	CPPUNIT_TEST_SUITE(FileTransferManagerTest);
	CPPUNIT_TEST(sendFileNoMethod);
	CPPUNIT_TEST(sendFileSIIBB);
	CPPUNIT_TEST_SUITE_END();

	public:
		FileTransferManager *manager;
		std::string data;
		FileTransferManager::Transfer transfer;
		bool dataNeeded;
		Swift::FileTransfer::State::FTState ftState;
		boost::shared_ptr<MemoryReadBytestream> bytestream;

		void setUp (void) {
			setMeUp();

			dataNeeded = false;
			data =  "This is our testing file\n";
			manager = new FileTransferManager(component, userManager);

			connectUser();
			add2Buddies();
			received.clear();
		}

		void tearDown (void) {
			received.clear();
			delete manager;
			disconnectUser();
			tearMeDown();
		}

	void connectUser() {
		CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
		userRegistry->isValidUserPassword(Swift::JID("user@localhost/resource"), serverFromClientSession.get(), Swift::createSafeByteArray("password"));
		loop->processEvents();
		CPPUNIT_ASSERT_EQUAL(1, userManager->getUserCount());

		User *user = userManager->getUser("user@localhost");
		CPPUNIT_ASSERT(user);

		UserInfo userInfo = user->getUserInfo();
		CPPUNIT_ASSERT_EQUAL(std::string("password"), userInfo.password);
		CPPUNIT_ASSERT(user->isReadyToConnect() == true);
		CPPUNIT_ASSERT(user->isConnected() == false);

		user->setConnected(true);
		CPPUNIT_ASSERT(user->isConnected() == true);

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::DiscoInfo>());
		received.clear();
	}

	void disconnectUser() {
		userManager->disconnectUser("user@localhost");
		dynamic_cast<Swift::DummyTimerFactory *>(factories->getTimerFactory())->setTime(10);
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(0, userManager->getUserCount());
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::Presence *>(getStanza(received[0])));
	}

	void add2Buddies() {
		User *user = userManager->getUser("user@localhost");
		CPPUNIT_ASSERT(user);

		std::vector<std::string> grp;
		grp.push_back("group1");
		LocalBuddy *buddy = new LocalBuddy(user->getRosterManager(), -1, "buddy1", "Buddy 1", grp, BUDDY_JID_ESCAPING);
		buddy->setStatus(Swift::StatusShow(Swift::StatusShow::Away), "status1");
		user->getRosterManager()->setBuddy(buddy);

		std::vector<std::string> grp2;
		grp2.push_back("group2");
		buddy = new LocalBuddy(user->getRosterManager(), -1, "buddy2", "Buddy 2", grp2, BUDDY_JID_ESCAPING);
		buddy->setStatus(Swift::StatusShow(Swift::StatusShow::Away), "status2");
		user->getRosterManager()->setBuddy(buddy);
	}


	void handleFTDataNeeded() {
		dataNeeded = true;
// 		std::cout << "data needed\n";
	}

	void handleFTStateChanged(Swift::FileTransfer::State state) {
// 		std::cout << "state changed " << state.state << "\n";
		ftState = state.state;
	}

	void _sendFile() {
		User *user = userManager->getUser("user@localhost");
		
		Swift::StreamInitiationFileInfo fileInfo;
		fileInfo.setSize(data.size());
		fileInfo.setName("test.pdf");

		bytestream = boost::shared_ptr<MemoryReadBytestream>(new MemoryReadBytestream(data.size()));
		bytestream->onDataNeeded.connect(boost::bind(&FileTransferManagerTest::handleFTDataNeeded, this));

		transfer = manager->sendFile(user, user->getRosterManager()->getBuddy("buddy1"), bytestream, fileInfo);
	}

	void sendFileNoMethod() {
		_sendFile();
		CPPUNIT_ASSERT(!transfer.ft);
	}
	
	void sendFileSIIBB() {
		User *user = userManager->getUser("user@localhost");
		boost::shared_ptr<Swift::DiscoInfo> info(new Swift::DiscoInfo());
		info->addFeature("http://jabber.org/protocol/si/profile/file-transfer");
		user->handleDiscoInfo("user@localhost/resource", info);

		// Send file request
		_sendFile();
		CPPUNIT_ASSERT(transfer.ft);
	
		transfer.ft->onStateChange.connect(boost::bind(&FileTransferManagerTest::handleFTStateChanged, this, _1));
		transfer.ft->start();
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::IQ::Set, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::StreamInitiation>());

		CPPUNIT_ASSERT_EQUAL(std::string("test.pdf"), dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getPayload<Swift::StreamInitiation>()->getFileInfo()->getName());
		CPPUNIT_ASSERT_EQUAL((int) data.size(), (int) dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getPayload<Swift::StreamInitiation>()->getFileInfo()->getSize());

		// request acceptation
		Swift::IQ::ref accept = Swift::IQ::createResult(getStanza(received[0])->getFrom(), getStanza(received[0])->getTo(), getStanza(received[0])->getID());
		Swift::StreamInitiation::ref payload(new Swift::StreamInitiation());
		payload->setRequestedMethod("http://jabber.org/protocol/ibb");
		accept->addPayload(payload);
		injectIQ(accept);
		loop->processEvents();
		received.erase(received.begin());

		// handle open packet and accept the FT
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::IQ::Set, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::IBB>());
		CPPUNIT_ASSERT_EQUAL(Swift::IBB::Open, getStanza(received[0])->getPayload<Swift::IBB>()->getAction());
		injectIQ(Swift::IQ::createResult(getStanza(received[0])->getFrom(), getStanza(received[0])->getTo(), getStanza(received[0])->getID()));
		loop->processEvents();

		CPPUNIT_ASSERT_EQUAL(true, dataNeeded);
		CPPUNIT_ASSERT_EQUAL(Swift::FileTransfer::State::Transferring, ftState);

		bytestream->appendData(data); // this will be send in one packet...
		loop->processEvents();
		received.erase(received.begin());

		// we received data now
		CPPUNIT_ASSERT_EQUAL(1, (int) received.size());
		CPPUNIT_ASSERT(dynamic_cast<Swift::IQ *>(getStanza(received[0])));
		CPPUNIT_ASSERT_EQUAL(Swift::IQ::Set, dynamic_cast<Swift::IQ *>(getStanza(received[0]))->getType());
		CPPUNIT_ASSERT(getStanza(received[0])->getPayload<Swift::IBB>());
		CPPUNIT_ASSERT_EQUAL(Swift::IBB::Data, getStanza(received[0])->getPayload<Swift::IBB>()->getAction());
		injectIQ(Swift::IQ::createResult(getStanza(received[0])->getFrom(), getStanza(received[0])->getTo(), getStanza(received[0])->getID()));
		loop->processEvents();
		received.erase(received.begin());
// 		dumpReceived();
		// TODO: Check that we receive Swift::IBB:Close stanza. This is not done right now because of Swiften bug...
	}


};

CPPUNIT_TEST_SUITE_REGISTRATION (FileTransferManagerTest);
