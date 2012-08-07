#include "OAuthFlow.h"
DEFINE_LOGGER(logger, "OAuthFlow")
void OAuthFlow::run()
{
	success = twitObj->oAuthRequestToken( authUrl );
}

void OAuthFlow::finalize()
{
	if (!success) {
		LOG4CXX_ERROR(logger, "Error creating twitter authorization url!");
		np->handleMessage(user, "twitter-account", "Error creating twitter authorization url!");
		np->handleLogoutRequest(user, username);
	} else {
		np->handleMessage(user, "twitter-account", std::string("Please visit the following link and authorize this application: ") + authUrl);
		np->handleMessage(user, "twitter-account", std::string("Please reply with the PIN provided by twitter. Prefix the pin with '#pin'. Ex. '#pin 1234'"));
		np->OAuthFlowComplete(user, twitObj);
	}	
}
