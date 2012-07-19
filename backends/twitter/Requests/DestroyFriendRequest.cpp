#include "DestroyFriendRequest.h"
DEFINE_LOGGER(logger, "DestroyFriendRequest")

void DestroyFriendRequest::run()
{
	replyMsg = "";
	success = twitObj->friendshipDestroy(frnd, false);
	if(success) {
		twitObj->getLastWebResponse(replyMsg);
		
		LOG4CXX_INFO(logger, user << replyMsg)		
		

	   	friendInfo = getUser(replyMsg);
		if(friendInfo.getScreenName() == "") LOG4CXX_INFO(logger, user << " - Was unable to fetch user info for " << frnd);
	}
}

void DestroyFriendRequest::finalize()
{
	if(!success) {
		std::string error;
		twitObj->getLastCurlError(error);
		LOG4CXX_ERROR(logger, user << " Curl error: " << error)
		callBack(user, friendInfo, error);
	} else {
		std::string error;
		error = getErrorMessage(replyMsg);
		if(error.length()) LOG4CXX_ERROR(logger, user << " - " << error)
		callBack(user, friendInfo, error);
	}
}
