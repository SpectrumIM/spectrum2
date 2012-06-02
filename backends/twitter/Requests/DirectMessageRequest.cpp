#include "DirectMessageRequest.h"
DEFINE_LOGGER(logger, "DirectMessageRequest")
void DirectMessageRequest::run() 
{
	if(twitObj.directMessageSend(username, data, false) == false) {
		LOG4CXX_ERROR(logger, user << ": Error while sending directed message to " << username );
		return;
	}
	twitObj.getLastWebResponse( replyMsg );
}

void DirectMessageRequest::finalize()
{
	LOG4CXX_INFO(logger, user << ": Sent " << data << " to " << username)
	LOG4CXX_INFO(logger, user << ": Twitter reponse - " << replyMsg)
}
