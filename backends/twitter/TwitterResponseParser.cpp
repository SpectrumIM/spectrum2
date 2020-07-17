#include "TwitterResponseParser.h"
#include "transport/Logging.h"
#include "boost/algorithm/string.hpp"
#include <cctype>
#include "boost/date_time/local_time/local_time.hpp"
#include "boost/date_time/time_facet.hpp"

DEFINE_LOGGER(responseParserLogger, "TwitterResponseParser")

static std::string tolowercase(std::string inp)
{
	std::string out = inp;
	for (unsigned i=0 ; i<out.size() ; i++) out[i] = tolower(out[i]);
	return out;
}

template <class T> std::string stringOf(T object) {
	std::ostringstream os;
	os << object;
	return (os.str());
}

static std::string unescape(std::string data, std::vector<UrlEntity> urls) {
	using boost::algorithm::replace_all;
	replace_all(data, "&amp;",  "&");
	replace_all(data, "&quot;", "\"");
	replace_all(data, "&apos;", "\'");
	replace_all(data, "&lt;",   "<");
	replace_all(data, "&gt;",   ">");

	for (std::vector<UrlEntity>::size_type i = 0; i < urls.size(); i++) {
		replace_all(data, urls[i].getUrl(), urls[i].getExpandedUrl());
	}

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

EmbeddedStatus getEmbeddedStatus(const Json::Value &element)
{
	EmbeddedStatus status;
	if(!element.isObject()) {
		LOG4CXX_ERROR(responseParserLogger, "Not a status element!");
		return status;
	}
	status.setCreationTime( toIsoTime ( element[TwitterReponseTypes::created_at.c_str()].asString() ) );
	status.setID( stringOf( element[TwitterReponseTypes::id.c_str()].asInt64() ) );
	status.setTweet( unescape ( element[TwitterReponseTypes::text.c_str()].asString(), getUrlEntities(element) ) );
	status.setTruncated( element[TwitterReponseTypes::truncated.c_str()].asBool());
	status.setReplyToStatusID( element[TwitterReponseTypes::in_reply_to_status_id.c_str()].isNull() ?
"" : stringOf(element[TwitterReponseTypes::in_reply_to_status_id.c_str()].asInt64()) );
	status.setReplyToUserID( element[TwitterReponseTypes::in_reply_to_user_id.c_str()].isNull() ?
"" : stringOf(element[TwitterReponseTypes::in_reply_to_user_id.c_str()].asInt64())  );
	status.setReplyToScreenName( element[TwitterReponseTypes::in_reply_to_screen_name.c_str()].isNull() ?
"" : element[TwitterReponseTypes::in_reply_to_screen_name.c_str()].asString() );
	status.setRetweetCount( element[TwitterReponseTypes::retweet_count.c_str()].asInt64() );
	status.setFavorited( element[TwitterReponseTypes::favorited.c_str()].asBool() );
	status.setRetweeted( element[TwitterReponseTypes::retweeted.c_str()].asBool());
	return status;
}

User getUser(const Json::Value &element)
{
	User user;
	if(!element.isObject()) {
		LOG4CXX_ERROR(responseParserLogger, "Not a user element!");
		return user;
	}

	user.setUserID( stringOf( element[TwitterReponseTypes::id.c_str()].asInt64() ) );
	user.setScreenName( tolowercase( element[TwitterReponseTypes::screen_name.c_str()].asString() ) );
	user.setUserName( element[TwitterReponseTypes::name.c_str()].asString() );
	user.setProfileImgURL( element[TwitterReponseTypes::profile_image_url.c_str()].asString() );
	user.setNumberOfTweets( element[TwitterReponseTypes::statuses_count.c_str()].asInt64() );
	if(element[TwitterReponseTypes::status.c_str()].isObject())
		user.setLastStatus(getEmbeddedStatus(element[TwitterReponseTypes::status.c_str()]));
	return user;
}

Status getStatus(const Json::Value &element)
{
	Status status;

	status.setCreationTime( toIsoTime ( element[TwitterReponseTypes::created_at.c_str()].asString() ) );
	status.setID( stringOf( element[TwitterReponseTypes::id.c_str()].asInt64()  ));
	status.setTweet( unescape ( element[TwitterReponseTypes::text.c_str()].asString(), getUrlEntities(element) ) );
	status.setTruncated( element[TwitterReponseTypes::truncated.c_str()].asBool());
	status.setReplyToStatusID( element[TwitterReponseTypes::in_reply_to_status_id.c_str()].isNull() ?
"" : stringOf(element[TwitterReponseTypes::in_reply_to_status_id.c_str()].asInt64()) );
	status.setReplyToUserID( element[TwitterReponseTypes::in_reply_to_user_id.c_str()].isNull() ?
"" : stringOf(element[TwitterReponseTypes::in_reply_to_user_id.c_str()].asInt64())  );
	status.setReplyToScreenName( element[TwitterReponseTypes::in_reply_to_screen_name.c_str()].isNull() ?
"" : element[TwitterReponseTypes::in_reply_to_screen_name.c_str()].asString() );
	status.setUserData( getUser(element[TwitterReponseTypes::user.c_str()]) );
	status.setRetweetCount( element[TwitterReponseTypes::retweet_count.c_str()].asInt64() );
	status.setFavorited( element[TwitterReponseTypes::favorited.c_str()].asBool() );
	status.setRetweeted( element[TwitterReponseTypes::retweeted.c_str()].asBool());
	const Json::Value &rt = element[TwitterReponseTypes::retweeted_status.c_str()];
	if (rt.isObject()) {
		status.setTweet(unescape( rt[TwitterReponseTypes::text.c_str()].asString() + " (RT by @" + status.getUserData().getScreenName() + ")"
			, getUrlEntities(element)) );
		status.setRetweetID( stringOf(rt[TwitterReponseTypes::id.c_str()].asInt64()) );
		status.setCreationTime( toIsoTime ( rt[TwitterReponseTypes::created_at.c_str()].asString() ) );
		status.setUserData( getUser ( rt[TwitterReponseTypes::user.c_str()]) );
	}

	return status;
}

DirectMessage getDirectMessage(const Json::Value &element)
{
	DirectMessage DM;

	DM.setCreationTime( toIsoTime ( element[TwitterReponseTypes::created_at.c_str()].asString() ) );
	DM.setID( stringOf( element[TwitterReponseTypes::id.c_str()].asInt64() ) );
	DM.setMessage( unescape ( element[TwitterReponseTypes::text.c_str()].asString(), getUrlEntities(element) ) );
	DM.setSenderID( stringOf( element[TwitterReponseTypes::sender_id.c_str()].asInt64())  );
	DM.setRecipientID( stringOf( element[TwitterReponseTypes::recipient_id.c_str()].asInt64() ) );
	DM.setSenderScreenName( element[TwitterReponseTypes::sender_screen_name.c_str()].asString() );
	DM.setRecipientScreenName( element[TwitterReponseTypes::recipient_screen_name.c_str()].asString() );
	DM.setSenderData( getUser(element[TwitterReponseTypes::sender.c_str()] ));
	DM.setRecipientData( getUser(element[TwitterReponseTypes::recipient.c_str()]) );
	return DM;
}

std::vector<Status> getTimeline(std::string &json)
{
	std::vector<Status> statuses;
	Json::Value rootElement;
	Json::Reader reader;

	if(!reader.parse(json.c_str(), rootElement)){
		LOG4CXX_ERROR(responseParserLogger, "Error while parsing JSON");
		LOG4CXX_ERROR(responseParserLogger, json);
		return statuses;
	}

	if(!rootElement.isArray()) {
		LOG4CXX_ERROR(responseParserLogger, "JSON doesn't correspond to timeline:");
		LOG4CXX_ERROR(responseParserLogger, json);
		return statuses;
	}

	for (Json::Value::ArrayIndex i = 0; i < rootElement.size(); i++) {
		statuses.push_back(getStatus(rootElement[i]));
	}
	return statuses;
}

std::vector<DirectMessage> getDirectMessages(std::string &json)
{
	std::vector<DirectMessage> DMs;
	Json::Value rootElement;
	Json::Reader reader;

	if(!reader.parse(json.c_str(), rootElement)) {
		LOG4CXX_ERROR(responseParserLogger, "Error while parsing JSON");
		LOG4CXX_ERROR(responseParserLogger, json);
		return DMs;
	}

	if(!rootElement.isArray()) {
		LOG4CXX_ERROR(responseParserLogger, "JSON doesn't correspond to direct messages:");
		LOG4CXX_ERROR(responseParserLogger, json);
		return DMs;
	}

	for (Json::Value::ArrayIndex i = 0; i < rootElement.size(); i++) {
		DMs.push_back(getDirectMessage(rootElement[i]));
	}
	return DMs;
}

std::vector<User> getUsers(std::string &json)
{
	std::vector<User> users;
	Json::Value rootElement;
	Json::Reader reader;

	if(!reader.parse(json.c_str(), rootElement)) {
		LOG4CXX_ERROR(responseParserLogger, "Error while parsing JSON");
		LOG4CXX_ERROR(responseParserLogger, json);
		return users;
	}

	if(!rootElement.isArray()) {
		LOG4CXX_ERROR(responseParserLogger, "JSON doesn't correspond to user list:");
		LOG4CXX_ERROR(responseParserLogger, json);
		return users;
	}

	for (Json::Value::ArrayIndex i = 0; i < rootElement.size(); i++) {
		users.push_back(getUser(rootElement[i]));
	}
	return users;
}

User getUser(std::string &json)
{
	User user;
	Json::Value rootElement;
	Json::Reader reader;

	if(!reader.parse(json.c_str(), rootElement)) {
		LOG4CXX_ERROR(responseParserLogger, "Error while parsing JSON");
		LOG4CXX_ERROR(responseParserLogger, json);
		return user;
	}

	if(!rootElement.isObject()) {
		LOG4CXX_ERROR(responseParserLogger, "JSON doesn't correspond to user object");
		LOG4CXX_ERROR(responseParserLogger, json);
		return user;
	}

	return user = getUser(rootElement);
}

std::vector<std::string> getIDs(std::string &json)
{
	std::vector<std::string> IDs;
	Json::Value rootElement;
	Json::Reader reader;

	if(!reader.parse(json.c_str(), rootElement)) {
		LOG4CXX_ERROR(responseParserLogger, "Error while parsing JSON");
		LOG4CXX_ERROR(responseParserLogger, json);
		return IDs;
	}

	if(!rootElement.isObject()) {
		LOG4CXX_ERROR(responseParserLogger, "JSON doesn't correspond to id_list");
		LOG4CXX_ERROR(responseParserLogger, json);
		return IDs;
	}

	const Json::Value & ids = rootElement[TwitterReponseTypes::ids.c_str()];

	for (Json::Value::ArrayIndex i=0 ; i<ids.size() ; i++) {
		IDs.push_back(stringOf( ids[i].asInt64()) );
	}
	return IDs;
}

Error getErrorMessage(std::string &json)
{
	std::string error = "";
	std::string code = "0";
	Error resp;
	Json::Value rootElement;
	Json::Reader reader;

	if(!reader.parse(json.c_str(), rootElement)) {
		LOG4CXX_ERROR(responseParserLogger, "Error while parsing JSON");
		LOG4CXX_ERROR(responseParserLogger, json);
		return resp;
	}
	if (rootElement.isObject()) {
		if (!rootElement["errors"].isNull()) {
			const Json::Value &errorElement = rootElement["errors"][0u]; // first error
			error = errorElement["message"].asString();
			code = stringOf(errorElement["code"].asInt64());
			resp.setCode(code);
			resp.setMessage(error);
		}
	}

	return resp;
}

std::vector<UrlEntity> getUrlEntities(const Json::Value &element)
{

	std::vector<UrlEntity> url_entities;

	const Json::Value &entities = element["entities"];

	if (entities.isObject()) {
		const Json::Value &urls = entities["urls"];
		if (urls.isArray()) {
			for (Json::Value::ArrayIndex i = 0; i < urls.size(); i++) {
				UrlEntity entity(urls[i]["url"].asString(), urls[i]["expanded_url"].asString());
				url_entities.push_back(entity);
			}
		}
	}

	return url_entities;
}
