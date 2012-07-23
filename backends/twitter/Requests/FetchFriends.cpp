#include "FetchFriends.h"
#include "../HTTPRequest.h"

DEFINE_LOGGER(logger, "FetchFriends")

void FetchFriends::run() 
{	
	replyMsg = ""; 

	success = twitObj->friendsIdsGet(twitObj->getTwitterUsername());
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
	
	for(int i=0 ; i<friends.size() ; i++) {
		std::string img;
		friendAvatars.push_back("");
		if(req.GET(friends[i].getProfileImgURL(), img)) friendAvatars[i] = img;
		else {
			LOG4CXX_INFO(logger, "Warning: Couldn't fetch Profile Image for " << user << "'s friend " << friends[i].getScreenName())
		}
	}
}

void FetchFriends::finalize()
{
	if(!success) {
		twitObj->getLastCurlError( replyMsg );
		LOG4CXX_ERROR(logger,  user << " - " << replyMsg)
		callBack(user, friends, friendAvatars, replyMsg);
	} else {
		std::string error = getErrorMessage(replyMsg);
		if(error.length()) LOG4CXX_ERROR(logger,  user << " - " << error)
		callBack(user, friends, friendAvatars, error);	
	} 
}
