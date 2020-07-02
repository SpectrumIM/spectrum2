#include "DestroyFriendRequest.h"
DEFINE_LOGGER(destroyFriendRequestLogger, "DestroyFriendRequest")

void DestroyFriendRequest::run()
{
	replyMsg = "";
	success = twitObj->friendshipDestroy(frnd, false);
	if(success) {
		twitObj->getLastWebResponse(replyMsg);

		LOG4CXX_INFO(destroyFriendRequestLogger, user << replyMsg);


	   	friendInfo = getUser(replyMsg);
		if(friendInfo.getScreenName() == "") LOG4CXX_INFO(destroyFriendRequestLogger, user << " - Was unable to fetch user info for " << frnd);
	}
}

void DestroyFriendRequest::finalize()
{
	Error error;
	if(!success) {
		std::string curlerror;
		twitObj->getLastCurlError(curlerror);
		error.setMessage(curlerror);
		LOG4CXX_ERROR(destroyFriendRequestLogger, user << " Curl error: " << curlerror);
		callBack(user, friendInfo, error);
	} else {
		error = getErrorMessage(replyMsg);
		if(error.getMessage().length()) LOG4CXX_ERROR(destroyFriendRequestLogger, user << " - " << error.getMessage());
		callBack(user, friendInfo, error);
	}
}
