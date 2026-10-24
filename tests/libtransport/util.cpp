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
#include "transport/utf8.h"
#include "transport/Util.h"


using namespace Transport;

class UtilTest : public CPPUNIT_NS :: TestFixture{
	CPPUNIT_TEST_SUITE(UtilTest);
	CPPUNIT_TEST(encryptDecryptPassword);
	CPPUNIT_TEST(serializeGroups);
	CPPUNIT_TEST(replaceInvalid);
	CPPUNIT_TEST(base64);
	CPPUNIT_TEST(replaceAll);
	CPPUNIT_TEST(split);
	CPPUNIT_TEST(caseInsensitiveAffixes);
	CPPUNIT_TEST(toLowerUTF8);
	CPPUNIT_TEST(generateUUID);
	CPPUNIT_TEST_SUITE_END();

	public:
		void setUp (void) {
		}

		void tearDown (void) {

		}

	void encryptDecryptPassword() {
		std::string encrypted = StorageBackend::encryptPassword("password", "key");
		CPPUNIT_ASSERT_EQUAL(std::string("password"), StorageBackend::decryptPassword(encrypted, "key"));
	}

	void serializeGroups() {
		std::vector<std::string> groups;
		std::string g = "";
		
		CPPUNIT_ASSERT_EQUAL(g, StorageBackend::serializeGroups(groups));
		CPPUNIT_ASSERT_EQUAL(0, (int) StorageBackend::deserializeGroups(g).size());

		groups.push_back("Buddies");
		g = "Buddies";
		CPPUNIT_ASSERT_EQUAL(g, StorageBackend::serializeGroups(groups));
		CPPUNIT_ASSERT_EQUAL(1, (int) StorageBackend::deserializeGroups(g).size());
		CPPUNIT_ASSERT_EQUAL(g, StorageBackend::deserializeGroups(g)[0]);

		groups.push_back("Buddies2");
		g = "Buddies\nBuddies2";
		CPPUNIT_ASSERT_EQUAL(g, StorageBackend::serializeGroups(groups));
		CPPUNIT_ASSERT_EQUAL(2, (int) StorageBackend::deserializeGroups(g).size());
		CPPUNIT_ASSERT_EQUAL(std::string("Buddies"), StorageBackend::deserializeGroups(g)[0]);
		CPPUNIT_ASSERT_EQUAL(std::string("Buddies2"), StorageBackend::deserializeGroups(g)[1]);
	}

	void replaceInvalid() {
		std::string x("test\x80\xe0\xa0\xc0\xaf\xed\xa0\x80test");
		std::string a;
		CPPUNIT_ASSERT(x.end() != utf8::find_invalid(x.begin(), x.end()));
		utf8::replace_invalid(x.begin(), x.end(), std::back_inserter(a), '_');
		CPPUNIT_ASSERT_EQUAL(std::string("test____test"), a);

		a = "";
		utf8::remove_invalid(x.begin(), x.end(), std::back_inserter(a));
		CPPUNIT_ASSERT_EQUAL(std::string("testtest"), a);
	}

	void base64() {
		CPPUNIT_ASSERT_EQUAL(std::string(""), Util::base64Encode(""));
		CPPUNIT_ASSERT_EQUAL(std::string("Zg=="), Util::base64Encode("f"));
		CPPUNIT_ASSERT_EQUAL(std::string("Zm8="), Util::base64Encode("fo"));
		CPPUNIT_ASSERT_EQUAL(std::string("Zm9v"), Util::base64Encode("foo"));
		CPPUNIT_ASSERT_EQUAL(std::string("Zm9vYmFy"), Util::base64Encode("foobar"));
		CPPUNIT_ASSERT_EQUAL(std::string("f"), Util::base64Decode("Zg=="));
		CPPUNIT_ASSERT_EQUAL(std::string("fo"), Util::base64Decode("Zm8="));
		CPPUNIT_ASSERT_EQUAL(std::string("foobar"), Util::base64Decode("Zm9v\nYmFy"));

		std::string binary("\x00\xff\x10\x00", 4);
		CPPUNIT_ASSERT(binary == Util::base64Decode(Util::base64Encode(binary)));
	}

	void replaceAll() {
		std::string s = "/var/lib/$jid/$jid.sql";
		Util::replaceAll(s, "$jid", "icq.localhost");
		CPPUNIT_ASSERT_EQUAL(std::string("/var/lib/icq.localhost/icq.localhost.sql"), s);
		CPPUNIT_ASSERT_EQUAL(std::string("aaaa"), Util::replaceAllCopy("aa", "a", "aa"));
		CPPUNIT_ASSERT_EQUAL(std::string("abc"), Util::replaceAllCopy("abc", "", "x"));
	}

	void split() {
		std::vector<std::string> v = Util::split("", '\n');
		CPPUNIT_ASSERT_EQUAL(1, (int) v.size());
		CPPUNIT_ASSERT_EQUAL(std::string(""), v[0]);

		v = Util::split("a\n\nb\n", '\n');
		CPPUNIT_ASSERT_EQUAL(4, (int) v.size());
		CPPUNIT_ASSERT_EQUAL(std::string("a"), v[0]);
		CPPUNIT_ASSERT_EQUAL(std::string(""), v[1]);
		CPPUNIT_ASSERT_EQUAL(std::string("b"), v[2]);
		CPPUNIT_ASSERT_EQUAL(std::string(""), v[3]);
	}

	void caseInsensitiveAffixes() {
		CPPUNIT_ASSERT(Util::iStartsWith("<BR/>text", "<br/>"));
		CPPUNIT_ASSERT(!Util::iStartsWith("<b", "<br"));
		CPPUNIT_ASSERT(Util::iEndsWith("text<Br>", "<br>"));
		CPPUNIT_ASSERT(!Util::iEndsWith("r>", "<br>"));
	}

	void toLowerUTF8() {
		CPPUNIT_ASSERT_EQUAL(std::string("hanzz@icq.com"), Util::toLowerUTF8("HanZZ@ICQ.com"));
		// Latin-1, Latin Extended-A, Greek, Cyrillic
		CPPUNIT_ASSERT_EQUAL(std::string("\xC3\xA4\xC3\xB6\xC3\xBC\xC3\x9F"), Util::toLowerUTF8("\xC3\x84\xC3\x96\xC3\x9C\xC3\x9F")); // ÄÖÜß
		CPPUNIT_ASSERT_EQUAL(std::string("\xC5\xBE\xC5\x82"), Util::toLowerUTF8("\xC5\xBD\xC5\x81")); // ŽŁ
		CPPUNIT_ASSERT_EQUAL(std::string("\xCE\xB1\xCF\x89"), Util::toLowerUTF8("\xCE\x91\xCE\xA9")); // ΑΩ
		CPPUNIT_ASSERT_EQUAL(std::string("\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82"), Util::toLowerUTF8("\xD0\x9F\xD0\xA0\xD0\x98\xD0\x92\xD0\x95\xD0\xA2")); // ПРИВЕТ
		CPPUNIT_ASSERT_EQUAL(std::string("\xD1\x91"), Util::toLowerUTF8("\xD0\x81")); // Ё
		// Invalid UTF-8 falls back to ASCII-only lowercasing
		CPPUNIT_ASSERT_EQUAL(std::string("ab\xC0"), Util::toLowerUTF8("AB\xC0"));
	}

	void generateUUID() {
		std::string uuid = Util::generateUUID();
		CPPUNIT_ASSERT_EQUAL(36, (int) uuid.size());
		CPPUNIT_ASSERT_EQUAL('-', uuid[8]);
		CPPUNIT_ASSERT_EQUAL('-', uuid[23]);
		CPPUNIT_ASSERT_EQUAL('4', uuid[14]);
		CPPUNIT_ASSERT(uuid != Util::generateUUID());
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION (UtilTest);
