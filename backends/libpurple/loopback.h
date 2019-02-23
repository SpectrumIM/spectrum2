#include <utime.h>
#include <string>
#include <list>
#include "glib.h"
#include "purple.h"

/*
Ignore copies of "our own messages" but forward "our messages" from elsewhere on the legacy network
to frontend.
1. Flag outgoing messages and check for that flag -- not all protocols preserve flags.
2. Store outgoing messages for a while and compare.
*/

//Additional PURPLE_MESSAGE_* flags as a hack to track the origin of the message.
typedef enum {
    PURPLE_MESSAGE_SPECTRUM2_ORIGINATED = 0x80000000,
} PurpleMessageSpectrum2Flags;

//Track outgoing messages and check incoming messages against them.
struct MessageFingerprint {
	PurpleConversation *conv;
	time_t when;
	std::string message;
};

class MessageLoopbackTracker : protected std::list<MessageFingerprint> {
protected:
	guint m_timer;
	bool m_autotrim;
public:
	//Tracking is only valid for a few seconds to avoid falsely matching foreign messages with
	//identical text
	static const time_t CarbonTimeout = 2;

	MessageLoopbackTracker();
	~MessageLoopbackTracker();

	//Registers an outgoing message
	void add(PurpleConversation *conv, const std::string &message);

	//Locates a message matching the given one well enough and removes it
	bool matchAndRemove(PurpleConversation *conv, const std::string &message, const time_t &when);

	//Removes messages older than a given time
	void trim(const time_t min_time);
	void setAutotrim(bool autotrimEnabled);
};
