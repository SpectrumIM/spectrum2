//#include "twitcurl.h"
//#include <cstdio>
//#include <iostream>
//#include <fstream>
//
//void printUsage()
//{
//    printf( "\nUsage:\ntwitterClient -u username -p password\n" );
//}
//
//int main( int argc, char* argv[] )
//{
//    std::string userName( "" );
//    std::string passWord( "" );
//    if( argc > 4 )
//    {
//        for( int i = 1; i < argc; i += 2 )
//        {
//            if( 0 == strncmp( argv[i], "-u", strlen("-u") ) )
//            {
//                userName = argv[i+1];
//            }
//            else if( 0 == strncmp( argv[i], "-p", strlen("-p") ) )
//            {
//                passWord = argv[i+1];
//            }
//        }
//        if( ( 0 == userName.length() ) || ( 0 == passWord.length() ) )
//        {
//            printUsage();
//            return 0;
//        }
//    }
//    else
//    {
//        printUsage();
//        return 0;
//    }
//
//    twitCurl twitterObj;
//    std::string tmpStr;
//    std::string replyMsg;
//    char tmpBuf[1024];
//
//    /* Set twitter username and password */
//    twitterObj.setTwitterUsername( userName );
//    twitterObj.setTwitterPassword( passWord );
//
//    /* Set proxy server usename, password, IP and port (if present) */
//    memset( tmpBuf, 0, 1024 );
//    printf( "\nDo you have a proxy server configured (0 for no; 1 for yes): " );
//    gets( tmpBuf );
//    tmpStr = tmpBuf;
//
//    if( std::string::npos != tmpStr.find( "1" ) )
//    {
//        char proxyIp[1024];
//        char proxyPort[1024];
//        char proxyUsername[1024];
//        char proxyPassword[1024];
//
//        memset( proxyIp, 0, 1024 );
//        memset( proxyPort, 0, 1024 );
//        memset( proxyUsername, 0, 1024 );
//        memset( proxyPassword, 0, 1024 );
//
//        printf( "\nEnter proxy server IP: " );
//        gets( proxyIp );
//        printf( "\nEnter proxy server port: " );
//        gets( proxyPort );
//        printf( "\nEnter proxy server username: " );
//        gets( proxyUsername );
//        printf( "\nEnter proxy server password: " );
//        gets( proxyPassword );
//
//        tmpStr = proxyIp;
//        twitterObj.setProxyServerIp( tmpStr );
//        tmpStr = proxyPort;
//        twitterObj.setProxyServerPort( tmpStr );
//        tmpStr = proxyUsername;
//        twitterObj.setProxyUserName( tmpStr );
//        tmpStr = proxyPassword;
//        twitterObj.setProxyPassword( tmpStr );
//    }
//
//    /* OAuth flow begins */
//    /* Step 0: Set OAuth related params. These are got by registering your app at twitter.com */
//    twitterObj.getOAuth().setConsumerKey( std::string( "qxfSCX7WN7SZl7dshqGZA" ) );
//    twitterObj.getOAuth().setConsumerSecret( std::string( "ypWapSj87lswvnksZ46hMAoAZvST4ePGPxAQw6S2o" ) );
//
////    twitterObj.getOAuth().setConsumerKey( std::string( "vlC5S1NCMHHg8mD1ghPRkA" ) );
////    twitterObj.getOAuth().setConsumerSecret( std::string( "3w4cIrHyI3IYUZW5O2ppcFXmsACDaENzFdLIKmEU84" ) );
//
//    /* Step 1: Check if we alredy have OAuth access token from a previous run */
//    std::string myOAuthAccessTokenKey("");
//    std::string myOAuthAccessTokenSecret("");
//    std::ifstream oAuthTokenKeyIn;
//    std::ifstream oAuthTokenSecretIn;
//
//    oAuthTokenKeyIn.open( "twitterClient_token_key.txt" );
//    oAuthTokenSecretIn.open( "twitterClient_token_secret.txt" );
//
//    memset( tmpBuf, 0, 1024 );
//    oAuthTokenKeyIn >> tmpBuf;
//    myOAuthAccessTokenKey = tmpBuf;
//
//    memset( tmpBuf, 0, 1024 );
//    oAuthTokenSecretIn >> tmpBuf;
//    myOAuthAccessTokenSecret = tmpBuf;
//
//    oAuthTokenKeyIn.close();
//    oAuthTokenSecretIn.close();
//
//    if( myOAuthAccessTokenKey.size() && myOAuthAccessTokenSecret.size() )
//    {
//        /* If we already have these keys, then no need to go through auth again */
//        printf( "\nUsing:\nKey: %s\nSecret: %s\n\n", myOAuthAccessTokenKey.c_str(), myOAuthAccessTokenSecret.c_str() );
//
//        twitterObj.getOAuth().setOAuthTokenKey( myOAuthAccessTokenKey );
//        twitterObj.getOAuth().setOAuthTokenSecret( myOAuthAccessTokenSecret );
//    }
//    else
//    {
//        /* Step 2: Get request token key and secret */
//        std::string authUrl;
//        twitterObj.oAuthRequestToken( authUrl );
//
//        /* Step 3: Get PIN  */
//        memset( tmpBuf, 0, 1024 );
//        printf( "\nDo you want to visit twitter.com for PIN (0 for no; 1 for yes): " );
//        gets( tmpBuf );
//        tmpStr = tmpBuf;
//        if( std::string::npos != tmpStr.find( "1" ) )
//        {
//            /* Ask user to visit twitter.com auth page and get PIN */
//            memset( tmpBuf, 0, 1024 );
//            printf( "\nPlease visit this link in web browser and authorize this application:\n%s", authUrl.c_str() );
//            printf( "\nEnter the PIN provided by twitter: " );
//            gets( tmpBuf );
//            tmpStr = tmpBuf;
//            twitterObj.getOAuth().setOAuthPin( tmpStr );
//        }
//        else
//        {
//            /* Else, pass auth url to twitCurl and get it via twitCurl PIN handling */
//            twitterObj.oAuthHandlePIN( authUrl );
//        }
//
//        /* Step 4: Exchange request token with access token */
//        twitterObj.oAuthAccessToken();
//
//        /* Step 5: Now, save this access token key and secret for future use without PIN */
//        twitterObj.getOAuth().getOAuthTokenKey( myOAuthAccessTokenKey );
//        twitterObj.getOAuth().getOAuthTokenSecret( myOAuthAccessTokenSecret );
//
//        /* Step 6: Save these keys in a file or wherever */
//        std::ofstream oAuthTokenKeyOut;
//        std::ofstream oAuthTokenSecretOut;
//
//        oAuthTokenKeyOut.open( "twitterClient_token_key.txt" );
//        oAuthTokenSecretOut.open( "twitterClient_token_secret.txt" );
//
//        oAuthTokenKeyOut.clear();
//        oAuthTokenSecretOut.clear();
//
//        oAuthTokenKeyOut << myOAuthAccessTokenKey.c_str();
//        oAuthTokenSecretOut << myOAuthAccessTokenSecret.c_str();
//
//        oAuthTokenKeyOut.close();
//        oAuthTokenSecretOut.close();
//    }
//    /* OAuth flow ends */
//
//    /* Post a new status message */
//    memset( tmpBuf, 0, 1024 );
//    printf( "\nEnter a new status message: " );
//    gets( tmpBuf );
//    tmpStr = tmpBuf;
//    replyMsg = "";
//    if( twitterObj.statusUpdate( tmpStr ) )
//    {
//        twitterObj.getLastWebResponse( replyMsg );
//        printf( "\ntwitterClient:: twitCurl::statusUpdate web response:\n%s\n", replyMsg.c_str() );
//    }
//    else
//    {
//        twitterObj.getLastCurlError( replyMsg );
//        printf( "\ntwitterClient:: twitCurl::statusUpdate error:\n%s\n", replyMsg.c_str() );
//    }
//
////    /* Search a string */
////    twitterObj.setTwitterApiType( twitCurlTypes::eTwitCurlApiFormatJson );
////    printf( "\nEnter string to search: " );
////    memset( tmpBuf, 0, 1024 );
////    gets( tmpBuf );
////    tmpStr = tmpBuf;
////    replyMsg = "";
////    if( twitterObj.search( tmpStr ) )
////    {
////        twitterObj.getLastWebResponse( replyMsg );
////        printf( "\ntwitterClient:: twitCurl::search web response:\n%s\n", replyMsg.c_str() );
////    }
////    else
////    {
////        twitterObj.getLastCurlError( replyMsg );
////        printf( "\ntwitterClient:: twitCurl::search error:\n%s\n", replyMsg.c_str() );
////    }
////
////    /* Get user timeline */
////    replyMsg = "";
////    printf( "\nGetting user timeline\n" );
////    if( twitterObj.timelineUserGet( false, false, 0 ) )
////    {
////        twitterObj.getLastWebResponse( replyMsg );
////        printf( "\ntwitterClient:: twitCurl::timelineUserGet web response:\n%s\n", replyMsg.c_str() );
////    }
////    else
////    {
////        twitterObj.getLastCurlError( replyMsg );
////        printf( "\ntwitterClient:: twitCurl::timelineUserGet error:\n%s\n", replyMsg.c_str() );
////    }
//
//#ifdef _TWITCURL_TEST_
////    /* Destroy a status message */
////    memset( statusMsg, 0, 1024 );
////    printf( "\nEnter status message id to delete: " );
////    gets( statusMsg );
////    tmpStr = statusMsg;
////    replyMsg = "";
////    if( twitterObj.statusDestroyById( tmpStr ) )
////    {
////        twitterObj.getLastWebResponse( replyMsg );
////        printf( "\ntwitterClient:: twitCurl::statusDestroyById web response:\n%s\n", replyMsg.c_str() );
////    }
////    else
////    {
////        twitterObj.getLastCurlError( replyMsg );
////        printf( "\ntwitterClient:: twitCurl::statusDestroyById error:\n%s\n", replyMsg.c_str() );
////    }
////
////    /* Get public timeline */
////    replyMsg = "";
////    printf( "\nGetting public timeline\n" );
////    if( twitterObj.timelinePublicGet() )
////    {
////        twitterObj.getLastWebResponse( replyMsg );
////        printf( "\ntwitterClient:: twitCurl::timelinePublicGet web response:\n%s\n", replyMsg.c_str() );
////    }
////    else
////    {
////        twitterObj.getLastCurlError( replyMsg );
////        printf( "\ntwitterClient:: twitCurl::timelinePublicGet error:\n%s\n", replyMsg.c_str() );
////    }
////
////    /* Get friend ids */
////    replyMsg = "";
////    printf( "\nGetting friend ids\n" );
////    tmpStr = "techcrunch";
////    if( twitterObj.friendsIdsGet( tmpStr, false ) )
////    {
////        twitterObj.getLastWebResponse( replyMsg );
////        printf( "\ntwitterClient:: twitCurl::friendsIdsGet web response:\n%s\n", replyMsg.c_str() );
////    }
////    else
////    {
////        twitterObj.getLastCurlError( replyMsg );
////        printf( "\ntwitterClient:: twitCurl::friendsIdsGet error:\n%s\n", replyMsg.c_str() );
////    }
////
////    /* Get trends */
////    if( twitterObj.trendsDailyGet() )
////    {
////        twitterObj.getLastWebResponse( replyMsg );
////        printf( "\ntwitterClient:: twitCurl::trendsDailyGet web response:\n%s\n", replyMsg.c_str() );
////    }
////    else
////    {
////        twitterObj.getLastCurlError( replyMsg );
////        printf( "\ntwitterClient:: twitCurl::trendsDailyGet error:\n%s\n", replyMsg.c_str() );
//    }
//#endif // _TWITCURL_TEST_
//
//    return 0;
//}

#include "transport/config.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"
#include "Swiften/Swiften.h"
#include "unistd.h"
#include "signal.h"
#include "sys/wait.h"
#include "sys/signal.h"
#include <boost/algorithm/string.hpp>
#include "twitcurl.h"
#include <iostream>
#include <map>
#include <vector>
#include <cstdio>

using namespace boost::filesystem;
using namespace boost::program_options;
using namespace Transport;

DEFINE_LOGGER(logger, "Twitter Backend");
Swift::SimpleEventLoop *loop_; // Event Loop
class TwitterPlugin; // The plugin
TwitterPlugin * np = NULL;

class TwitterPlugin : public NetworkPlugin {
	public:
		Swift::BoostNetworkFactories *m_factories;
		Swift::BoostIOServiceThread m_boostIOServiceThread;
		boost::shared_ptr<Swift::Connection> m_conn;

		TwitterPlugin(Config *config, Swift::SimpleEventLoop *loop, const std::string &host, int port) : NetworkPlugin() {
			this->config = config;
			m_factories = new Swift::BoostNetworkFactories(loop);
			m_conn = m_factories->getConnectionFactory()->createConnection();
			m_conn->onDataRead.connect(boost::bind(&TwitterPlugin::_handleDataRead, this, _1));
			m_conn->connect(Swift::HostAddressPort(Swift::HostAddress(host), port));

			LOG4CXX_INFO(logger, "Starting the plugin.");
		}

		// Send data to NetworkPlugin server
		void sendData(const std::string &string) {
			m_conn->write(Swift::createSafeByteArray(string));
		}

		// Receive date from the NetworkPlugin server and invoke the appropirate payload handler (implement in the NetworkPlugin class)
		void _handleDataRead(boost::shared_ptr<Swift::SafeByteArray> data) {
			std::string d(data->begin(), data->end());
			handleDataRead(d);
		}
		
		// User trying to login into his twitter account
		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
			if(sessions.count(user)) {
				LOG4CXX_INFO(logger, "A session corresponding to " + user + "  is already active\n")
				return;
			}
			
			//twitCurl &twitterObj = sessions[user];
			//std::string myOAuthAccessTokenSecret, myOAuthAccessTokenKey;
        	//twitterObj.getOAuth().getOAuthTokenKey( myOAuthAccessTokenKey );
        	//twitterObj.getOAuth().getOAuthTokenSecret( myOAuthAccessTokenSecret );

			//if(myOAuthAccessTokenSecret.size() && myOAuthAccessTokenKey.size()) {	
			//}
			handleConnected(user);
			handleBuddyChanged(user, "twitter-account", "twitter", std::vector<std::string>(), pbnetwork::STATUS_ONLINE);
		}
		
		// User logging out
		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
		}


		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "") {
			LOG4CXX_INFO(logger, "Sending message from " << user << " to " << legacyName << ".");
		}

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) {
			LOG4CXX_INFO(logger, user << ": Added buddy " << buddyName << ".");
			handleBuddyChanged(user, buddyName, alias, groups, pbnetwork::STATUS_ONLINE);
		}

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) {

		}

	private:
		Config *config;
		std::map<std::string, twitCurl> sessions;
};

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


int main (int argc, char* argv[]) {
	std::string host;
	int port;

	if (signal(SIGCHLD, spectrum_sigchld_handler) == SIG_ERR) {
		std::cout << "SIGCHLD handler can't be set\n";
		return -1;
	}

	boost::program_options::options_description desc("Usage: spectrum [OPTIONS] <config_file.cfg>\nAllowed options");
	desc.add_options()
		("host,h", value<std::string>(&host), "host")
		("port,p", value<int>(&port), "port")
		;
	try
	{
		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
		boost::program_options::notify(vm);
	}
	catch (std::runtime_error& e)
	{
		std::cout << desc << "\n";
		exit(1);
	}
	catch (...)
	{
		std::cout << desc << "\n";
		exit(1);
	}


	if (argc < 5) {
		return 1;
	}

	Config config;
	if (!config.load(argv[5])) {
		std::cerr << "Can't open " << argv[1] << " configuration file.\n";
		return 1;
	}

	Logging::initBackendLogging(&config);

	Swift::SimpleEventLoop eventLoop;
	loop_ = &eventLoop;
	np = new TwitterPlugin(&config, &eventLoop, host, port);
	loop_->run();

	return 0;
}
