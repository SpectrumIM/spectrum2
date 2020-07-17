#include "OAuthFlow.h"
DEFINE_LOGGER(oauthFlowRequestLogger, "OAuthFlow")
void OAuthFlow::run()
{
	success = twitObj->oAuthRequestToken( authUrl );
}

void OAuthFlow::finalize()
{
	if (!success) {
		LOG4CXX_ERROR(oauthFlowRequestLogger, "Error creating twitter authorization url!");
		np->handleMessage(user, "twitter.com", "Error creating twitter authorization url!");
		np->handleLogoutRequest(user, username);
	} else {
		np->handleMessage(user, "twitter.com", std::string("Please visit the following link and authorize this application: ") + authUrl);
		np->handleMessage(user, "twitter.com", std::string("Please reply with the PIN provided by twitter. Prefix the pin with '#pin'. Ex. '#pin 1234'"));
		np->OAuthFlowComplete(user, twitObj);
	}	
}
