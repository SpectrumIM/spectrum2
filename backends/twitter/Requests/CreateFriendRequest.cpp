#include "CreateFriendRequest.h"
DEFINE_LOGGER(logger, "CreateFriendRequest")

void CreateFriendRequest::run()
{
	LOG4CXX_INFO(logger, user << ": Sending follow request for " << frnd)
	replyMsg = "";
	success = twitObj->friendshipCreate(frnd, false);
	if(success) twitObj->getLastWebResponse(replyMsg);
}

void CreateFriendRequest::finalize()
{
	if(!success) {
		std::string error;
		twitObj->getLastCurlError(error);
		LOG4CXX_ERROR(logger, user << " " << error)
		callBack(user, frnd, error);
	} else {
		std::string error;
		error = getErrorMessage(replyMsg);
		if(error.length()) LOG4CXX_ERROR(logger, user << " " << error)
		else LOG4CXX_INFO(logger, user << ": Now following " << frnd)
		callBack(user, frnd, error);
	}
}
