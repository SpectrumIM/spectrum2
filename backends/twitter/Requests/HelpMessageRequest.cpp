#include "HelpMessageRequest.h"
DEFINE_LOGGER(logger, "HelpMessageRequest")
void HelpMessageRequest::run() 
{
	std::string helpMsg = "";
	helpMsg = helpMsg
			+ "\n******************************HELP************************************\n"
			+ "#status:<your status> ==> Update your status\n"
			+ "#timeline             ==> Retrieve your timeline\n"
			+ "@<username>:<message> ==> Send a directed message to the user <username>\n"
			+ "#help                 ==> Print this help message\n"
			+ "************************************************************************\n";

	np->handleMessage(user, "twitter-account", helpMsg);
}

void HelpMessageRequest::finalize()
{
}
