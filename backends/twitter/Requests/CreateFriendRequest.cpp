#include "CreateFriendRequest.h"
#include "../HTTPRequest.h"
DEFINE_LOGGER(logger, "CreateFriendRequest")

void CreateFriendRequest::run()
{
	LOG4CXX_INFO(logger, user << " - Sending follow request for " << frnd);
	replyMsg = "";
	success = twitObj->friendshipCreate(frnd, false);
	if(success) {
		twitObj->getLastWebResponse(replyMsg);

		LOG4CXX_INFO(logger, user << replyMsg);

	   	friendInfo = getUser(replyMsg);
		if(friendInfo.getScreenName() == "") LOG4CXX_INFO(logger, user << " - Was unable to fetch user info for " << frnd);

		HTTPRequest req;
		std::string img;

		req.init();
		req.setProxy(twitObj->getProxyServerIp(), twitObj->getProxyServerPort(), twitObj->getProxyUserName(), twitObj->getProxyPassword());

		profileImg = "";
		if(req.GET(friendInfo.getProfileImgURL(), img)) profileImg = img;
		else {
			LOG4CXX_INFO(logger, user << " - Was unable to fetch profile image of user " << frnd);
		}

	}
}

void CreateFriendRequest::finalize()
{
	Error error;
	if(!success) {
		std::string curlerror;
		twitObj->getLastCurlError(curlerror);
		error.setMessage(curlerror);
		LOG4CXX_ERROR(logger, user << " - Curl error: " << curlerror);
		callBack(user, friendInfo, profileImg, error);
	} else {
		error = getErrorMessage(replyMsg);
		if(error.getMessage().length()) {
			LOG4CXX_ERROR(logger, user << " - " << error.getMessage());
		}
		else LOG4CXX_INFO(logger, user << ": Now following " << frnd);
		callBack(user, friendInfo, profileImg, error);
	}
}
