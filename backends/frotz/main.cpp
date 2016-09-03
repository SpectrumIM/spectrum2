/*
 * Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
 *
 * This example is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include "transport/Config.h"
#include "transport/NetworkPlugin.h"
#include "Swiften/Swiften.h"
#include <boost/filesystem.hpp>
#include "unistd.h"
#include "signal.h"
#include "sys/wait.h"
#include "sys/signal.h"
Swift::SimpleEventLoop *loop_;

using namespace boost::program_options;
using namespace Transport;

class FrotzNetworkPlugin;
FrotzNetworkPlugin * np = NULL;

#define	PARENT_READ	p.readpipe[0]
#define	CHILD_WRITE	p.readpipe[1]
#define CHILD_READ	p.writepipe[0]
#define PARENT_WRITE	p.writepipe[1]

typedef struct dfrotz_ {
	pid_t pid;
	std::string game;
	int readpipe[2];
	int writepipe[2];
} dfrotz;

using namespace boost::filesystem;

static const char *howtoplay = "To move around, just type the direction you want to go.  Directions can be\n"
"abbreviated:  NORTH to N, SOUTH to S, EAST to E, WEST to W, NORTHEAST to\n"
"NE, NORTHWEST to NW, SOUTHEAST to SE, SOUTHWEST to SW, UP to U, and DOWN\n"
"to D.  IN and OUT will also work in certain places.\n"
"\n"
"There are many differnet kinds of sentences used in Interactive Fiction.\n"
"Here are some examples:\n"
"\n"
"> WALK TO THE NORTH\n"
"> WEST\n"
"> NE\n"
"> DOWN\n"
"> TAKE THE BIRDCAGE\n"
"> READ ABOUT DIMWIT FLATHEAD\n"
"> LOOK UP MEGABOZ IN THE ENCYCLOPEDIA\n"
"> LIE DOWN IN THE PINK SOFA\n"
"> EXAMINE THE SHINY COIN\n"
"> PUT THE RUSTY KEY IN THE CARDBOARD BOX\n"
"> SHOW MY BOW TIE TO THE BOUNCER\n"
"> HIT THE CRAWLING CRAB WITH THE GIANT NUTCRACKER\n"
"> ASK THE COWARDLY KING ABOUT THE CROWN JEWELS\n"
"\n"
"You can use multiple objects with certain verbs if you separate them by\n"
"the word \"AND\" or by a comma.  Here are some examples:\n"
"\n"
"> TAKE THE BOOK AND THE FROG\n"
"> DROP THE JAR OF PEANUT BUTTER, THE SPOON, AND THE LEMMING FOOD\n"
"> PUT THE EGG AND THE PENCIL IN THE CABINET\n"
"\n"
"You can include several inputs on one line if you separate them by the\n"
"word \"THEN\" or by a period.  Each input will be handled in order, as\n"
"though you had typed them individually at seperate prompts.  For example,\n"
"you could type all of the following at once, before pressing the ENTER (or\n"
"RETURN) key:\n"
"\n"
"> TURN ON THE LIGHT. TAKE THE BOOK THEN READ ABOUT THE JESTER IN THE BOOK\n"
"\n"
"If the story doesn't understand one of the sentences on your input line,\n"
"or if an unusual event occurs, it will ignore the rest of your input line.\n"
"\n"
"The words \"IT\" and \"ALL\" can be very useful.  For example:\n"
"\n"
"> EXAMINE THE APPLE.  TAKE IT.  EAT IT\n"
"> CLOSE THE HEAVY METAL DOOR.  LOCK IT\n"
"> PICK UP THE GREEN BOOT.  SMELL IT.  PUT IT ON.\n"
"> TAKE ALL\n"
"> TAKE ALL THE TOOLS\n"
"> DROP ALL THE TOOLS EXCEPT THE WRENCH AND MINIATURE HAMMER\n"
"> TAKE ALL FROM THE CARTON\n"
"> GIVE ALL BUT THE RUBY SLIPPERS TO THE WICKED WITCH\n"
"\n"
"The word \"ALL\" refers to every visible object except those inside\n"
"something else.  If there were an apple on the ground and an orange inside\n"
"a cabinet, \"TAKE ALL\" would take the apple but not the orange.\n"
"\n"
"There are three kinds of questions you can ask:  \"WHERE IS (something)\",\n"
"\"WHAT IS (something)\", and \"WHO IS (someone)\".  For example:\n"
"\n"
"> WHO IS LORD DIMWIT?\n"
"> WHAT IS A GRUE?\n"
"> WHERE IS EVERYBODY?\n"
"\n"
"When you meet intelligent creatures, you can talk to them by typing their\n"
"name, then a comma, then whatever you want to say to them.  Here are some\n"
"examples:\n"
"\n"
"> JESTER, HELLO\n"
"> GUSTAR WOOMAX, TELL ME ABOUT THE COCONUT\n"
"> UNCLE OTTO, GIVE ME YOUR WALLET\n"
"> HORSE, WHERE IS YOUR SADDLE?\n"
"> BOY, RUN HOME THEN CALL THE POLICE\n"
"> MIGHTY WIZARD, TAKE THIS POISONED APPLE.  EAT IT\n"
"\n"
"Notice that in the last two examples, you are giving the characters more\n"
"than one command on the same input line.  Keep in mind, however, that many\n"
"creatures don't care for idle chatter; your actions will speak louder than\n"
"your words.  \n";


static void start_dfrotz(dfrotz &p, const std::string &game) {
// 	p.writepipe[0] = -1;

	if (pipe(p.readpipe) < 0 || pipe(p.writepipe) < 0) {
	}

	std::cout << "dfrotz -p " << game << "\n";

	if ((p.pid = fork()) < 0) {
		/* FATAL: cannot fork child */
	}
	else if (p.pid == 0) {
		close(PARENT_WRITE);
		close(PARENT_READ);

		dup2(CHILD_READ,  0);  close(CHILD_READ);
		dup2(CHILD_WRITE, 1);  close(CHILD_WRITE);

		execlp("dfrotz", "-p", game.c_str(), NULL);

	}
	else {
		close(CHILD_READ);
		close(CHILD_WRITE);
	}
}

class FrotzNetworkPlugin : public NetworkPlugin {
	public:
		Swift::BoostNetworkFactories *m_factories;
		Swift::BoostIOServiceThread m_boostIOServiceThread;
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> m_conn;

		FrotzNetworkPlugin(Config *config, Swift::SimpleEventLoop *loop, const std::string &host, int port) : NetworkPlugin() {
			this->config = config;
			m_factories = new Swift::BoostNetworkFactories(loop);
			m_conn = m_factories->getConnectionFactory()->createConnection();
			m_conn->onDataRead.connect(boost::bind(&FrotzNetworkPlugin::_handleDataRead, this, _1));
			m_conn->connect(Swift::HostAddressPort(Swift::HostAddress(host), port));
// 			m_conn->onConnectFinished.connect(boost::bind(&FrotzNetworkPlugin::_handleConnected, this, _1));
// 			m_conn->onDisconnected.connect(boost::bind(&FrotzNetworkPlugin::handleDisconnected, this));
		}

		void sendData(const std::string &string) {
			m_conn->write(Swift::createSafeByteArray(string));
		}

		void _handleDataRead(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::SafeByteArray> data) {
			std::string d(data->begin(), data->end());
			handleDataRead(d);
		}

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
			np->handleConnected(user);
			std::vector<std::string> groups;
			groups.push_back("ZCode");
			np->handleBuddyChanged(user, "zcode", "ZCode", groups, pbnetwork::STATUS_ONLINE);
// 			sleep(1);
// 			np->handleMessage(np->m_user, "zork", first_msg);
		}

		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
			if (games.find(user) != games.end()) {
				kill(games[user].pid, SIGTERM);
				games.erase(user);
			}
// 			exit(0);
		}

		void readMessage(const std::string &user) {
			static char buf[15000];
			buf[0] = 0;
			int repeated = 0;
			while (strlen(buf) == 0) {
				ssize_t len = read(games[user].readpipe[0], buf, 15000);
				if (len > 0) {
					buf[len] = 0;
				}
				usleep(1000);
				repeated++;
				if (repeated > 30)
					return;
			}
			np->handleMessage(user, "zcode", buf);

			std::string msg = "save\n";
			write(games[user].writepipe[1], msg.c_str(), msg.size());

			msg = user + "_" + games[user].game + ".save\n";
			write(games[user].writepipe[1], msg.c_str(), msg.size());
			ignoreMessage(user);
		}

		void ignoreMessage(const std::string &user) {
			usleep(1000000);
			static char buf[15000];
			buf[0] = 0;
			int repeated = 0;
			while (strlen(buf) == 0) {
				ssize_t len = read(games[user].readpipe[0], buf, 15000);
				if (len > 0) {
					buf[len] = 0;
				}
				usleep(1000);
				repeated++;
				if (repeated > 30)
					return;
			}

			std::cout << "ignoring: " << buf << "\n";
		}

		std::vector<std::string> getGames() {
			std::vector<std::string> games;
			path p(".");
			directory_iterator end_itr;
			for (directory_iterator itr(p); itr != end_itr; ++itr) {
				if (extension(itr->path()) == ".z5") {
#if BOOST_FILESYSTEM_VERSION == 3
					games.push_back(itr->path().filename().string());
#else
					games.push_back(itr->path().leaf());
#endif
				}
			}
			return games;
		}

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "", const std::string &id = "") {
			std::cout << "aaa\n";
			if (message.find("start") == 0) {
				std::string game = message.substr(6);
				std::vector<std::string> lst = getGames();
				if (std::find(lst.begin(), lst.end(), game) == lst.end()) {
					np->handleMessage(user, "zcode", "Unknown game");
					return;
				}
				np->handleMessage(user, "zcode", "Starting the game");

				dfrotz d;
				d.game = game;
				start_dfrotz(d, game);
				games[user] = d;
				fcntl(games[user].readpipe[0], F_SETFL, O_NONBLOCK);

				if (boost::filesystem::exists(user + "_" + games[user].game + ".save")) {

					std::string msg = "restore\n";
					write(games[user].writepipe[1], msg.c_str(), msg.size());

					msg = user + "_" + games[user].game + ".save\n";
					write(games[user].writepipe[1], msg.c_str(), msg.size());

					ignoreMessage(user);

					msg = "l\n";
					write(games[user].writepipe[1], msg.c_str(), msg.size());
				}

				readMessage(user);
			}
			else if (message == "stop" && games.find(user) != games.end()) {
				kill(games[user].pid, SIGTERM);
				games.erase(user);
				np->handleMessage(user, "zcode", "Game stopped");
			}
			else if (message == "howtoplay") {
				np->handleMessage(user, "zcode", howtoplay);
			}
			else if (games.find(user) != games.end()) {
				std::string msg = message + "\n";
				write(games[user].writepipe[1], msg.c_str(), msg.size());
				readMessage(user);
			}
			else {
				std::string games;
				BOOST_FOREACH(const std::string &game, getGames()) {
					games += game + "\n";
				}
				np->handleMessage(user, "zcode", "Games are saved/loaded automatically. Use \"restart\" to restart existing game. Emulator commands are:\nstart <game>\nstop\nhowtoplay\n\nList of games:\n" + games);
			}
		}

		void handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password) {
		}

		void handleLeaveRoomRequest(const std::string &user, const std::string &room) {
		}

		std::map<std::string, dfrotz> games;
		std::string first_msg;
	private:
		
		Config *config;
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

	std::string error;
	Config *cfg = Config::createFromArgs(argc, argv, error, host, port);
	if (cfg == NULL) {
		std::cerr << error;
		return 1;
	}

	Swift::SimpleEventLoop eventLoop;
	loop_ = &eventLoop;
	np = new FrotzNetworkPlugin(cfg, &eventLoop, host, port);
	loop_->run();

	delete cfg;

	return 0;
}
