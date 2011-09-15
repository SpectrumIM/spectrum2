/*
 * Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
 *
 * This example is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include "transport/config.h"
#include "transport/networkplugin.h"
#include "frotz.h"

#ifndef MSDOS_16BIT
#define cdecl
#endif

Swift::SimpleEventLoop *loop_;

extern "C" void spectrum_get_line(char *s);

char input[15000];

void send_array();

void spectrum_get_line(char *s) {
	std::cout << "running event loop\n";
	dumb_show_screen(FALSE);
	send_array();
// 	while(strlen(input) == 0) {
		loop_->run();
// 	}
	strcpy(s, input);
	strcpy(input, "");
	std::cout << "got message " << s << "\n";
}

using namespace boost::program_options;
using namespace Transport;

extern void interpret (void);
extern void init_memory (void);
extern void init_undo (void);
extern void reset_memory (void);

/* Story file name, id number and size */

char *story_name = "zork.z5";

enum story story_id = UNKNOWN;
long story_size = 0;

/* Story file header data */

zbyte h_version = 0;
zbyte h_config = 0;
zword h_release = 0;
zword h_resident_size = 0;
zword h_start_pc = 0;
zword h_dictionary = 0;
zword h_objects = 0;
zword h_globals = 0;
zword h_dynamic_size = 0;
zword h_flags = 0;
zbyte h_serial[6] = { 0, 0, 0, 0, 0, 0 };
zword h_abbreviations = 0;
zword h_file_size = 0;
zword h_checksum = 0;
zbyte h_interpreter_number = 0;
zbyte h_interpreter_version = 0;
zbyte h_screen_rows = 0;
zbyte h_screen_cols = 0;
zword h_screen_width = 0;
zword h_screen_height = 0;
zbyte h_font_height = 1;
zbyte h_font_width = 1;
zword h_functions_offset = 0;
zword h_strings_offset = 0;
zbyte h_default_background = 0;
zbyte h_default_foreground = 0;
zword h_terminating_keys = 0;
zword h_line_width = 0;
zbyte h_standard_high = 1;
zbyte h_standard_low = 0;
zword h_alphabet = 0;
zword h_extension_table = 0;
zbyte h_user_name[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

zword hx_table_size = 0;
zword hx_mouse_x = 0;
zword hx_mouse_y = 0;
zword hx_unicode_table = 0;

/* Stack data */

zword stack[STACK_SIZE];
zword *sp = 0;
zword *fp = 0;
zword frame_count = 0;

/* IO streams */

int ostream_screen = TRUE;
int ostream_script = FALSE;
int ostream_memory = FALSE;
int ostream_record = FALSE;
int istream_replay = FALSE;
int message = FALSE;

/* Current window and mouse data */

int cwin = 0;
int mwin = 0;

int mouse_y = 0;
int mouse_x = 0;

/* Window attributes */

int enable_wrapping = FALSE;
int enable_scripting = FALSE;
int enable_scrolling = FALSE;
int enable_buffering = FALSE;

/* User options */

/*
int option_attribute_assignment = 0;
int option_attribute_testing = 0;
int option_context_lines = 0;
int option_object_locating = 0;
int option_object_movement = 0;
int option_left_margin = 0;
int option_right_margin = 0;
int option_ignore_errors = 0;
int option_piracy = 0;
int option_undo_slots = MAX_UNDO_SLOTS;
int option_expand_abbreviations = 0;
int option_script_cols = 80;
int option_save_quetzal = 1;
*/

int option_sound = 1;
char *option_zcode_path;


/* Size of memory to reserve (in bytes) */

long reserve_mem = 0;

/*
 * z_piracy, branch if the story file is a legal copy.
 *
 *	no zargs used
 *
 */

void z_piracy (void)
{

    branch (!f_setup.piracy);

}/* z_piracy */

class FrotzNetworkPlugin;
FrotzNetworkPlugin * np = NULL;

class FrotzNetworkPlugin : public NetworkPlugin {
	public:
		FrotzNetworkPlugin(Config *config, Swift::SimpleEventLoop *loop, const std::string &host, int port) : NetworkPlugin(loop, host, port) {
			this->config = config;
		}

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
			m_user = user;
			np->handleConnected(user);
			Swift::StatusShow status;
			np->handleBuddyChanged(user, "zork", "Zork", "Games", status.getType());
		}

		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
		}

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &/*xhtml*/) {
			std::string msg = message + "\n";
			strcpy(input, msg.c_str());
			loop_->stop();
		}

		void handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password) {
		}

		void handleLeaveRoomRequest(const std::string &user, const std::string &room) {
		}
std::string m_user;
	private:
		
		Config *config;
};

void send_array() {
	np->handleMessage(np->m_user, "zork", frotz_get_array());
	std::cout << "sending " << frotz_get_array() << "\n";
	frotz_reset_array();
}

int main (int argc, char* argv[]) {
	std::string host;
	int port;


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

// 	QStringList channels;
// 	for (int i = 3; i < argc; ++i)
// 	{
// 		channels.append(argv[i]);
// 	}
// 
// 	MyIrcSession session;
// 	session.setNick(argv[2]);
// 	session.setAutoJoinChannels(channels);
// 	session.connectToServer(argv[1], 6667);

	Config config;
	if (!config.load(argv[5])) {
		std::cerr << "Can't open " << argv[1] << " configuration file.\n";
		return 1;
	}

	Swift::SimpleEventLoop eventLoop;
	loop_ = &eventLoop;
	np = new FrotzNetworkPlugin(&config, &eventLoop, host, port);

	os_init_setup ();

	os_process_arguments (argc, argv);

	init_buffer ();

	init_err ();

	init_memory ();

	init_process ();

	init_sound ();

	os_init_screen ();

	init_undo ();

	z_restart ();

	interpret ();

	reset_memory ();

	os_reset_screen ();

	return 0;
}
