#include "FetchFriends.h"
DEFINE_LOGGER(logger, "FetchFriends")

void FetchFriends::run() 
{	
	replyMsg = ""; 

	success = twitObj->friendsIdsGet(twitObj->getTwitterUsername());
	if(!success) return;	

	twitObj->getLastWebResponse( replyMsg );
	//LOG4CXX_INFO(logger, user << " - " << replyMsg.length() << " " << replyMsg << "\n" );
	std::vector<std::string> IDs = getIDs( replyMsg );
	
	success = twitObj->userLookup(IDs, true);
	if(!success) return;

	twitObj->getLastWebResponse( replyMsg );
	//LOG4CXX_INFO(logger, user << " - UserLookUp web response - " << replyMsg.length() << " " << replyMsg << "\n" );
	friends = getUsers( replyMsg );
}

void FetchFriends::finalize()
{
	if(!success) {
		twitObj->getLastCurlError( replyMsg );
		LOG4CXX_ERROR(logger,  user << " - " << replyMsg)
		callBack(user, friends, replyMsg);
	} else {
		std::string error = getErrorMessage(replyMsg);
		if(error.length()) LOG4CXX_ERROR(logger,  user << " - " << error)
		callBack(user, friends, error);	
	} 
}
