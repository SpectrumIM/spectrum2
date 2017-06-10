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
	Error error;
	if(!success) {
		std::string curlerror;
		twitObj->getLastCurlError(curlerror);
		error.setMessage(curlerror);
		LOG4CXX_ERROR(logger, user << " Curl error: " << curlerror);
		callBack(user, friendInfo, error);
	} else {
		error = getErrorMessage(replyMsg);
		if(error.getMessage().length()) LOG4CXX_ERROR(logger, user << " - " << error.getMessage());
		callBack(user, friendInfo, error);
	}
}
