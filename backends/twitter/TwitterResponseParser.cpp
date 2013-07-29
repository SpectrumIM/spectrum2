#include "TwitterResponseParser.h"
#include "transport/logging.h"
#include "boost/algorithm/string.hpp"
#include <cctype>
#include "boost/date_time/local_time/local_time.hpp"
#include "boost/date_time/time_facet.hpp"

DEFINE_LOGGER(logger, "TwitterResponseParser")

static std::string tolowercase(std::string inp)
{
	std::string out = inp;
	for(int i=0 ; i<out.size() ; i++) out[i] = tolower(out[i]);
	return out;
}

template <class T> std::string stringOf(T object) {
	std::ostringstream os;
	os << object;
	return (os.str());
}

static std::string unescape(std::string data) {
	using boost::algorithm::replace_all;
	replace_all(data, "&amp;",  "&");
	replace_all(data, "&quot;", "\"");
	replace_all(data, "&apos;", "\'");
	replace_all(data, "&lt;",   "<");
	replace_all(data, "&gt;",   ">");
	return data;
}

static std::string toIsoTime(std::string in) {
	std::locale locale(std::locale::classic(), new boost::local_time::local_time_input_facet("%a %b %d %H:%M:%S +0000 %Y"));
	boost::local_time::local_time_facet* output_facet = new boost::local_time::local_time_facet();
	boost::local_time::local_date_time ldt(boost::local_time::not_a_date_time);
	std::stringstream ss(in);
	ss.imbue(locale);
	ss.imbue(std::locale(ss.getloc(), output_facet));
	output_facet->format("%Y%m%dT%H%M%S"); // boost::local_time::local_time_facet::iso_time_format_specifier ?
	ss >> ldt;
	ss.str("");
	ss << ldt;	
	return ss.str();
}

EmbeddedStatus getEmbeddedStatus(const rapidjson::Value &element)
{
	EmbeddedStatus status;
	if(!element.IsObject()) {
		LOG4CXX_ERROR(logger, "Not a status element!")
		return status;
	}
	status.setCreationTime( toIsoTime ( std::string( element[TwitterReponseTypes::created_at.c_str()].GetString() ) ) );
	status.setID( stringOf( element[TwitterReponseTypes::id.c_str()].GetInt64() ) );
	status.setTweet( unescape ( std::string( element[TwitterReponseTypes::text.c_str()].GetString() ) ) );
	status.setTruncated( element[TwitterReponseTypes::truncated.c_str()].GetBool());
	status.setReplyToStatusID( element[TwitterReponseTypes::in_reply_to_status_id.c_str()].IsNull() ?
"" : stringOf(element[TwitterReponseTypes::in_reply_to_status_id.c_str()].GetInt64()) );
	status.setReplyToUserID( element[TwitterReponseTypes::in_reply_to_user_id.c_str()].IsNull() ?
"" : stringOf(element[TwitterReponseTypes::in_reply_to_user_id.c_str()].GetInt64())  );
	status.setReplyToScreenName( element[TwitterReponseTypes::in_reply_to_screen_name.c_str()].IsNull() ?
"" : std::string(element[TwitterReponseTypes::in_reply_to_screen_name.c_str()].GetString()) );
	status.setRetweetCount( element[TwitterReponseTypes::retweet_count.c_str()].GetInt64() );
	status.setFavorited( element[TwitterReponseTypes::favorited.c_str()].GetBool() );
	status.setRetweeted( element[TwitterReponseTypes::retweeted.c_str()].GetBool());	
	return status;
}

User getUser(const rapidjson::Value &element) 
{
	User user;
	if(!element.IsObject()) {
		LOG4CXX_ERROR(logger, "Not a user element!")
		return user;
	}

	user.setUserID( stringOf( element[TwitterReponseTypes::id.c_str()].GetInt64() ) );
	user.setScreenName( tolowercase( std::string( element[TwitterReponseTypes::screen_name.c_str()].GetString() ) ) );
	user.setUserName( std::string( element[TwitterReponseTypes::name.c_str()].GetString() ) );
	user.setProfileImgURL( std::string( element[TwitterReponseTypes::profile_image_url.c_str()].GetString() ) );
	user.setNumberOfTweets( element[TwitterReponseTypes::statuses_count.c_str()].GetInt64() );
	if(element[TwitterReponseTypes::status.c_str()].IsObject()) 
		user.setLastStatus(getEmbeddedStatus(element[TwitterReponseTypes::status.c_str()]));
	return user;
}

Status getStatus(const rapidjson::Value &element) 
{
	Status status;
	
	status.setCreationTime( toIsoTime ( std::string( element[TwitterReponseTypes::created_at.c_str()].GetString() ) ) );
	status.setID( stringOf( element[TwitterReponseTypes::id.c_str()].GetInt64()  ));
	status.setTweet( unescape ( std::string( element[TwitterReponseTypes::text.c_str()].GetString() ) ) );
	status.setTruncated( element[TwitterReponseTypes::truncated.c_str()].GetBool());
	status.setReplyToStatusID( element[TwitterReponseTypes::in_reply_to_status_id.c_str()].IsNull() ?
"" : stringOf(element[TwitterReponseTypes::in_reply_to_status_id.c_str()].GetInt64()) );
	status.setReplyToUserID( element[TwitterReponseTypes::in_reply_to_user_id.c_str()].IsNull() ?
"" : stringOf(element[TwitterReponseTypes::in_reply_to_user_id.c_str()].GetInt64())  );
	status.setReplyToScreenName( element[TwitterReponseTypes::in_reply_to_screen_name.c_str()].IsNull() ?
"" : std::string(element[TwitterReponseTypes::in_reply_to_screen_name.c_str()].GetString()) );
	status.setUserData( getUser(element[TwitterReponseTypes::user.c_str()]) );
	status.setRetweetCount( element[TwitterReponseTypes::retweet_count.c_str()].GetInt64() );
	status.setFavorited( element[TwitterReponseTypes::favorited.c_str()].GetBool() );
	status.setRetweeted( element[TwitterReponseTypes::retweeted.c_str()].GetBool());
	const rapidjson::Value &rt = element[TwitterReponseTypes::retweeted_status.c_str()];
	if (rt.IsObject()) {
		status.setTweet(unescape( std::string( rt[TwitterReponseTypes::text.c_str()].GetString()) + " (RT by @" + status.getUserData().getScreenName() + ")") );
		status.setRetweetID( stringOf(rt[TwitterReponseTypes::id.c_str()].GetInt64()) );
		status.setCreationTime( toIsoTime ( std::string (rt[TwitterReponseTypes::created_at.c_str()].GetString() ) ) );
		status.setUserData( getUser ( rt[TwitterReponseTypes::user.c_str()]) );
	}
	return status;
}

DirectMessage getDirectMessage(const rapidjson::Value &element) 
{
	DirectMessage DM;
	
	DM.setCreationTime( toIsoTime ( std::string( element[TwitterReponseTypes::created_at.c_str()].GetString() ) ) );
	DM.setID( stringOf( element[TwitterReponseTypes::id.c_str()].GetInt64() ) );
	DM.setMessage( unescape ( std::string( element[TwitterReponseTypes::text.c_str()].GetString() ) ) );
	DM.setSenderID( stringOf( element[TwitterReponseTypes::sender_id.c_str()].GetInt64())  );
	DM.setRecipientID( stringOf( element[TwitterReponseTypes::recipient_id.c_str()].GetInt64() ) );
	DM.setSenderScreenName( std::string( element[TwitterReponseTypes::sender_screen_name.c_str()].GetString() ) );
	DM.setRecipientScreenName( std::string( element[TwitterReponseTypes::recipient_screen_name.c_str()].GetString() ) );
	DM.setSenderData( getUser(element[TwitterReponseTypes::sender.c_str()] ));
	DM.setRecipientData( getUser(element[TwitterReponseTypes::recipient.c_str()]) );
	return DM;
}

std::vector<Status> getTimeline(std::string &json)
{
	std::vector<Status> statuses;
	rapidjson::Document rootElement;
	
	if(rootElement.Parse<0>(json.c_str()).HasParseError()) {
		LOG4CXX_ERROR(logger, "Error while parsing JSON")
        LOG4CXX_ERROR(logger, json)
		return statuses;
	}

	if(!rootElement.IsArray()) {
		LOG4CXX_ERROR(logger, "JSON doesn't correspond to timeline:")
        LOG4CXX_ERROR(logger, json)
		return statuses;
	}

	for(rapidjson::SizeType i = 0; i < rootElement.Size(); i++) {
		statuses.push_back(getStatus(rootElement[i]));
	}
	return statuses;
}

std::vector<DirectMessage> getDirectMessages(std::string &json)
{
	std::vector<DirectMessage> DMs;
	rapidjson::Document rootElement;
	
	
	if(rootElement.Parse<0>(json.c_str()).HasParseError()) {
		LOG4CXX_ERROR(logger, "Error while parsing JSON")
        LOG4CXX_ERROR(logger, json)
		return DMs;
	}

	if(!rootElement.IsArray()) {
		LOG4CXX_ERROR(logger, "JSON doesn't correspond to direct messages:")
        LOG4CXX_ERROR(logger, json)
		return DMs;
	}

	for(rapidjson::SizeType i = 0; i < rootElement.Size(); i++) {
		DMs.push_back(getDirectMessage(rootElement[i]));
	}
	return DMs;
}

std::vector<User> getUsers(std::string &json)
{
	std::vector<User> users;
	rapidjson::Document rootElement;
	
	
	if(rootElement.Parse<0>(json.c_str()).HasParseError()) {
		LOG4CXX_ERROR(logger, "Error while parsing JSON")
        LOG4CXX_ERROR(logger, json)
		return users;
	}

	if(!rootElement.IsArray()) {
		LOG4CXX_ERROR(logger, "JSON doesn't correspond to user list:")
        LOG4CXX_ERROR(logger, json)
		return users;
	}

	for(rapidjson::SizeType i = 0; i < rootElement.Size(); i++) {
		users.push_back(getUser(rootElement[i]));
	}
	return users;	
}

User getUser(std::string &json)
{
	User user;
	rapidjson::Document rootElement;
	
	if(rootElement.Parse<0>(json.c_str()).HasParseError()) {
		LOG4CXX_ERROR(logger, "Error while parsing JSON")
        LOG4CXX_ERROR(logger, json)
		return user;
	}

	if(!rootElement.IsObject()) {
		LOG4CXX_ERROR(logger, "JSON doesn't correspond to user object")
        LOG4CXX_ERROR(logger, json)
		return user;
	}

	return user = getUser(rootElement);
}

std::vector<std::string> getIDs(std::string &json)
{
	std::vector<std::string> IDs;
	rapidjson::Document rootElement;
	
	if(rootElement.Parse<0>(json.c_str()).HasParseError()) {
		LOG4CXX_ERROR(logger, "Error while parsing JSON")
        LOG4CXX_ERROR(logger, json)
		return IDs;
	}

	if(!rootElement.IsObject()) {
		LOG4CXX_ERROR(logger, "JSON doesn't correspond to id_list");
        LOG4CXX_ERROR(logger, json)
		return IDs;
	}

	const rapidjson::Value & ids = rootElement[TwitterReponseTypes::ids.c_str()];
	
	for(int i=0 ; i<ids.Size() ; i++) {
		IDs.push_back(stringOf( ids[i].GetInt64()) );
	}
	return IDs;
}

Error getErrorMessage(std::string &json)
{
	std::string error = "";
	std::string code = "0";
	Error resp;
	rapidjson::Document rootElement;
	
	if(rootElement.Parse<0>(json.c_str()).HasParseError()) {
		LOG4CXX_ERROR(logger, "Error while parsing JSON")
        LOG4CXX_ERROR(logger, json)
		return resp;
	}
	if (rootElement.IsObject()) {
		if (!rootElement["errors"].IsNull()) {			
			const rapidjson::Value &errorElement = rootElement["errors"][0u]; // first error
			error = std::string(errorElement["message"].GetString());
			code = stringOf(errorElement["code"].GetInt64());
			resp.setCode(code);
			resp.setMessage(error);
		}
	}

	return resp;
}
