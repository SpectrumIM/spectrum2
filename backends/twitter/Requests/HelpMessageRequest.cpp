#include "HelpMessageRequest.h"
DEFINE_LOGGER(helpRequestLogger, "HelpMessageRequest")
void HelpMessageRequest::run() 
{
	helpMsg = helpMsg
			+ "You will receive tweets of people you follow from this contact."
			+ "Following commands can be used to manage your Twitter account:\n\n"
			+ "#timeline [username]       ==> Retrieve <username>'s timeline; Default - own timeline\n"
			+ "#status <your status>      ==> Update your status\n"
			+ "@<username> <message>      ==> Send a directed message to the user <username>\n"
			+ "#retweet <unique_tweet_id> ==> Retweet the tweet having id <unique_tweet_id> \n"
			+ "#follow <username>         ==> Follow user <username>\n"
			+ "#unfollow <username>       ==> Stop Following user <username>\n"
			+ "#mode [01]                ==> Switch mode to 0(single), 1(multiple). See below\n"
			+ "#help                      ==> Print this help message\n"
			+ "************************************************************************\n"
			+ "There are 3 ways how to use the gateway:\n"
			+ "Mode 0 - There is twitter.com contact in your roster and all Twitter messages are forwaded using this contact.\n"
			+ "Mode 1 - Same as Mode 0, but the people you follow are displayed in your roster. You will receive/send directed messages using those contacts.\n"
			+ "Joining the #twitter@" + jid + " room - You can join mentioned room and see people you follow there as well as the tweets they post. Private messages in that room are mapped to Twitter direct messages.";
}

void HelpMessageRequest::finalize()
{
	callBack(user, helpMsg);
}
