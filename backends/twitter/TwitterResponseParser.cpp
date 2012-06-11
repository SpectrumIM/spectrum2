#include "TwitterResponseParser.h"
#include "transport/logging.h"

DEFINE_LOGGER(logger, "TwitterResponseParser")

User getUser(const Swift::ParserElement::ref &element, const std::string xmlns) 
{
	User user;
	if(element->getName() != TwitterReponseTypes::user 
	   && element->getName() != TwitterReponseTypes::sender
	   && element->getName() != TwitterReponseTypes::recipient) {
		LOG4CXX_ERROR(logger, "Not a user element!")
		return user;
	}

	user.setUserID( std::string( element->getChild(TwitterReponseTypes::id, xmlns)->getText() ) );
	user.setScreenName( std::string( element->getChild(TwitterReponseTypes::screen_name, xmlns)->getText() ) );
	user.setUserName( std::string( element->getChild(TwitterReponseTypes::name, xmlns)->getText() ) );
	user.setNumberOfTweets( atoi(element->getChild(TwitterReponseTypes::statuses_count, xmlns)->getText().c_str()) );
	return user;
}

Status getStatus(const Swift::ParserElement::ref &element, const std::string xmlns) 
{
	Status status;
	if(element->getName() != "status") {
		LOG4CXX_ERROR(logger, "Not a status element!")
		return status;
	}

	status.setCreationTime( std::string( element->getChild(TwitterReponseTypes::created_at, xmlns)->getText() ) );
	status.setID( std::string( element->getChild(TwitterReponseTypes::id, xmlns)->getText() ) );
	status.setTweet( std::string( element->getChild(TwitterReponseTypes::text, xmlns)->getText() ) );
	status.setTruncated( std::string( element->getChild(TwitterReponseTypes::truncated, xmlns)->getText() )=="true" );
	status.setReplyToStatusID( std::string( element->getChild(TwitterReponseTypes::in_reply_to_status_id, xmlns)->getText() ) );
	status.setReplyToUserID( std::string( element->getChild(TwitterReponseTypes::in_reply_to_user_id, xmlns)->getText() ) );
	status.setReplyToScreenName( std::string( element->getChild(TwitterReponseTypes::in_reply_to_screen_name, xmlns)->getText() ) );
	status.setUserData( getUser(element->getChild(TwitterReponseTypes::user, xmlns), xmlns) );
	status.setRetweetCount( atoi( element->getChild(TwitterReponseTypes::retweet_count, xmlns)->getText().c_str() ) );
	status.setFavorited( std::string( element->getChild(TwitterReponseTypes::favorited, xmlns)->getText() )=="true" );
	status.setRetweeted( std::string( element->getChild(TwitterReponseTypes::retweeted, xmlns)->getText() )=="true" );
	return status;
}

DirectMessage getDirectMessage(const Swift::ParserElement::ref &element, const std::string xmlns) 
{
	DirectMessage DM;
	if(element->getName() != TwitterReponseTypes::direct_message) {
		LOG4CXX_ERROR(logger, "Not a direct_message element!")
		return DM;
	}

	DM.setCreationTime( std::string( element->getChild(TwitterReponseTypes::created_at, xmlns)->getText() ) );
	DM.setID( std::string( element->getChild(TwitterReponseTypes::id, xmlns)->getText() ) );
	DM.setMessage( std::string( element->getChild(TwitterReponseTypes::text, xmlns)->getText() ) );
	DM.setSenderID( std::string( element->getChild(TwitterReponseTypes::sender_id, xmlns)->getText() ) );
	DM.setRecipientID( std::string( element->getChild(TwitterReponseTypes::recipient_id, xmlns)->getText() ) );
	DM.setSenderScreenName( std::string( element->getChild(TwitterReponseTypes::sender_screen_name, xmlns)->getText() ) );
	DM.setRecipientScreenName( std::string( element->getChild(TwitterReponseTypes::recipient_screen_name, xmlns)->getText() ) );
	DM.setSenderData( getUser(element->getChild(TwitterReponseTypes::sender, xmlns), xmlns) );
	DM.setRecipientData( getUser(element->getChild(TwitterReponseTypes::recipient, xmlns), xmlns) );
	return DM;
}

std::vector<Status> getTimeline(std::string &xml)
{
	std::vector<Status> statuses;
	Swift::ParserElement::ref rootElement = Swift::StringTreeParser::parse(xml);
	
	if(rootElement->getName() != "statuses") {
		LOG4CXX_ERROR(logger, "XML doesn't correspond to timeline")
		return statuses;
	}

	const std::string xmlns = rootElement->getNamespace();
	const std::vector<Swift::ParserElement::ref> children = rootElement->getChildren(TwitterReponseTypes::status, xmlns);

	for(int i = 0; i <  children.size() ; i++) {
		const Swift::ParserElement::ref status = children[i];
		statuses.push_back(getStatus(status, xmlns));
	}
	return statuses;
}

std::vector<DirectMessage> getDirectMessages(std::string &xml)
{
	std::vector<DirectMessage> DMs;
	Swift::ParserElement::ref rootElement = Swift::StringTreeParser::parse(xml);
	
	if(rootElement->getName() != TwitterReponseTypes::directmessages) {
		LOG4CXX_ERROR(logger, "XML doesn't correspond to direct-messages")
		return DMs;
	}

	const std::string xmlns = rootElement->getNamespace();
	const std::vector<Swift::ParserElement::ref> children = rootElement->getChildren(TwitterReponseTypes::direct_message, xmlns);

	for(int i = 0; i <  children.size() ; i++) {
		const Swift::ParserElement::ref dm = children[i];
		DMs.push_back(getDirectMessage(dm, xmlns));
	}
	return DMs;
}

std::vector<User> getUsers(std::string &xml)
{
	std::vector<User> users;
	Swift::ParserElement::ref rootElement = Swift::StringTreeParser::parse(xml);
	
	if(rootElement->getName() != TwitterReponseTypes::users) {
		LOG4CXX_ERROR(logger, "XML doesn't correspond to user list")
		return users;
	}

	const std::string xmlns = rootElement->getNamespace();
	const std::vector<Swift::ParserElement::ref> children = rootElement->getChildren(TwitterReponseTypes::user, xmlns);

	for(int i = 0 ; i < children.size() ; i++) {
		const Swift::ParserElement::ref user = children[i];
		users.push_back(getUser(user, xmlns));
	}
	return users;
}

std::vector<std::string> getIDs(std::string &xml)
{
	std::vector<std::string> IDs;
	Swift::ParserElement::ref rootElement = Swift::StringTreeParser::parse(xml);

	if(rootElement->getName() != TwitterReponseTypes::id_list) {
		LOG4CXX_ERROR(logger, "XML doesn't correspond to id_list");
		return IDs;
	}

	const std::string xmlns = rootElement->getNamespace();
	const std::vector<Swift::ParserElement::ref> ids = rootElement->getChild(TwitterReponseTypes::ids, xmlns)->getChildren(TwitterReponseTypes::id, xmlns);
	
	for(int i=0 ; i<ids.size() ; i++) {
		IDs.push_back(std::string( ids[i]->getText() ));
	}
	return IDs;
}

std::string getErrorMessage(std::string &xml)
{
	std::string error;
	Swift::ParserElement::ref rootElement = Swift::StringTreeParser::parse(xml);

	const std::string xmlns = rootElement->getNamespace();
	const Swift::ParserElement::ref errorElement = rootElement->getChild(TwitterReponseTypes::error, xmlns);
	
	if(errorElement != NULL) error = errorElement->getText();
	return error;
}
