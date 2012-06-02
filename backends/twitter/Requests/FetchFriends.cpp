#include "FetchFriends.h"
DEFINE_LOGGER(logger, "FetchFriends")

void FetchFriends::run() 
{	
	replyMsg = ""; 
	if( twitObj.friendsIdsGet(twitObj.getTwitterUsername())) {
		
		while(replyMsg.length() == 0) {
			twitObj.getLastWebResponse( replyMsg );
		}

		LOG4CXX_INFO(logger, user << " - " << replyMsg.length() << " " << replyMsg << "\n" );

		std::vector<std::string> IDs = getIDs( replyMsg );
		
		twitObj.userLookup(IDs, true);
		twitObj.getLastWebResponse( replyMsg );

		LOG4CXX_INFO(logger, user << " - UserLookUp web response - " << replyMsg.length() << " " << replyMsg << "\n" );

		std::vector<User> users = getUsers( replyMsg );
		
		userlist = "\n***************USER LIST****************\n";
		for(int i=0 ; i < users.size() ; i++) {
			userlist += "*)" + users[i].getUserName() + " (" + users[i].getScreenName() + ")\n";
		}	
		userlist += "***************************************\n";	

	}
}

void FetchFriends::finalize()
{
	if(replyMsg != "" ) {
		np->handleMessage(user, "twitter-account", userlist);
	} else {
		twitObj.getLastCurlError( replyMsg );
		LOG4CXX_INFO(logger, user << " - friendsIdsGet error - " << replyMsg );
	}	
}
