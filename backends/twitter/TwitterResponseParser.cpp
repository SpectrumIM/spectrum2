#include "TwitterResponseParser.h"

User getUser(const Swift::ParserElement::ref &element, const std::string xmlns) 
{
	User user;
	if(element->getName() != "user") {
		std::cerr << "Not a user element!" << std::endl;
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
		std::cerr << "Not a status element!" << std::endl;
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

std::vector<Status> getTimeline(std::string &xml)
{
	std::vector<Status> statuses;
	Swift::ParserElement::ref rootElement = Swift::StringTreeParser::parse(xml);
	
	if(rootElement->getName() != "statuses") {
		std::cerr << "XML doesnt correspond to timline\n";
		return statuses;
	}

	const std::string xmlns = rootElement->getNamespace();
	const std::vector<Swift::ParserElement::ref> children = rootElement->getChildren(TwitterReponseTypes::status, xmlns);
//	const std::vector<Swift::ParserElement::ref>::iterator it;

	for(int i = 0; i <  children.size() ; i++) {
		const Swift::ParserElement::ref status = children[i];
		statuses.push_back(getStatus(status, xmlns));
	}
	return statuses;
}
