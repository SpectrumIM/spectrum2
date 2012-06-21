#include "HelpMessageRequest.h"
DEFINE_LOGGER(logger, "HelpMessageRequest")
void HelpMessageRequest::run() 
{
	helpMsg = helpMsg
			+ "\n******************************HELP************************************\n"
			+ "#status <your status>      ==> Update your status\n"
			+ "#timeline [username]       ==> Retrieve <username>'s timeline; Default - own timeline\n"
			+ "@<username> <message>      ==> Send a directed message to the user <username>\n"
			+ "#retweet <unique_tweet_id> ==> Retweet the tweet having id <unique_tweet_id> \n"
			+ "#follow <username>         ==> Follow user <username>\n"
			+ "#unfollow <username>       ==> Stop Following user <username>\n"
			+ "#help                      ==> Print this help message\n"
			+ "************************************************************************\n";
}

void HelpMessageRequest::finalize()
{
	callBack(user, helpMsg);
}
