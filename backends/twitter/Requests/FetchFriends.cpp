#include "FetchFriends.h"
#include "../HTTPRequest.h"

DEFINE_LOGGER(fetchFriendsRequestLogger, "FetchFriends")

void FetchFriends::run()
{
	replyMsg = "";
	std::string next = "";
	success = twitObj->friendsIdsGet(next, twitObj->getTwitterUsername(), false);
	if(!success) return;

	twitObj->getLastWebResponse( replyMsg );
	std::vector<std::string> IDs = getIDs( replyMsg );

	success = twitObj->userLookup(IDs, true);
	if(!success) return;

	twitObj->getLastWebResponse( replyMsg );
	friends = getUsers( replyMsg );

	HTTPRequest req;
	req.init();
	req.setProxy(twitObj->getProxyServerIp(), twitObj->getProxyServerPort(), twitObj->getProxyUserName(), twitObj->getProxyPassword());

	for(unsigned i=0 ; i<friends.size() ; i++) {
		std::string img;
		friendAvatars.push_back("");
		if(req.GET(friends[i].getProfileImgURL(), img)) friendAvatars[i] = img;
		else {
			LOG4CXX_INFO(fetchFriendsRequestLogger, "Warning: Couldn't fetch Profile Image for " << user << "'s friend " << friends[i].getScreenName());
		}
	}
}

void FetchFriends::finalize()
{
	Error error;
	if(!success) {
		std::string curlerror;
		twitObj->getLastCurlError(curlerror);
		error.setMessage(curlerror);
		LOG4CXX_ERROR(fetchFriendsRequestLogger,  user << " - " << curlerror);
		callBack(user, friends, friendAvatars, error);
	} else {
		error = getErrorMessage(replyMsg);
		if(error.getMessage().length()) LOG4CXX_ERROR(fetchFriendsRequestLogger,  user << " - " << error.getMessage());
		callBack(user, friends, friendAvatars, error);
	}
}
