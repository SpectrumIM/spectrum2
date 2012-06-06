#ifndef TWITTERRESPOSNSEPARSER_H
#define TWITTERRESPOSNSEPARSER_H

#include "Swiften/Parser/StringTreeParser.h"
#include <iostream>
#include <vector>

namespace TwitterReponseTypes
{
	const std::string id = "id";
	const std::string id_list = "id_list";
	const std::string ids = "ids";
	const std::string name = "name";
	const std::string screen_name = "screen_name";
	const std::string statuses_count = "statuses_count";
	const std::string created_at = "created_at";
	const std::string text = "text";
	const std::string truncated = "truncated";
	const std::string in_reply_to_status_id = "in_reply_to_user_id";
	const std::string in_reply_to_user_id = "in_reply_to_user_id";
	const std::string in_reply_to_screen_name = "in_reply_to_screen_name";
	const std::string retweet_count = "retweet_count";
	const std::string favorited = "favorited";
	const std::string retweeted = "retweeted";
	const std::string user = "user";
	const std::string users = "users";
	const std::string status = "status";
	const std::string error = "error";
};

//Class holding user data
class User
{
	std::string ID;
	std::string name;
	std::string screen_name;
	unsigned int statuses_count;

	public:
	User():ID(""),name(""),screen_name(""),statuses_count(0){}

	std::string getUserID() {return ID;}
	std::string getUserName() {return name;}
	std::string getScreenName() {return screen_name;}
	unsigned int getNumberOfTweets() {return statuses_count;}
	
	
	void setUserID(std::string _id) {ID = _id;}
	void setUserName(std::string _name) {name = _name;}
	void setScreenName(std::string _screen) {screen_name = _screen;}
	void setNumberOfTweets(unsigned int sc) {statuses_count  = sc;}
};

//Class representing a status (tweet)
class Status
{
	std::string created_at;
	std::string ID;
	std::string text;
	bool truncated;
	std::string in_reply_to_status_id;
	std::string in_reply_to_user_id;
	std::string in_reply_to_screen_name;
	User user;
	unsigned int retweet_count;
	bool favorited;
	bool retweeted;

	public:
	Status():created_at(""),ID(""),text(""),truncated(false),in_reply_to_status_id(""),
	         in_reply_to_user_id(""),in_reply_to_screen_name(""),user(User()),retweet_count(0),
	         favorited(false),retweeted(0){}
	
	std::string getCreationTime() {return created_at;}
	std::string getID() {return ID;}
	std::string getTweet() {return text;}
	bool isTruncated() {return truncated;}
	std::string getReplyToStatusID() {return in_reply_to_status_id;}
	std::string getReplyToUserID() {return in_reply_to_user_id;}
	std::string getReplyToScreenName() {return in_reply_to_screen_name;}
	User getUserData() {return user;}
	unsigned int getRetweetCount() {return retweet_count;}
	bool isFavorited() {return favorited;}
	bool isRetweeted() {return retweeted;}
	
	void setCreationTime(std::string _created) {created_at = _created;}
	void setID(std::string _id) {ID = _id;}
	void setTweet(std::string _text) {text = _text;}
	void setTruncated(bool val) {truncated = val;}
	void setReplyToStatusID(std::string _id) {in_reply_to_status_id = _id;}
	void setReplyToUserID(std::string _id) {in_reply_to_user_id = _id;}
	void setReplyToScreenName(std::string _name) {in_reply_to_screen_name = _name;}
	void setUserData(User u) {user = u;}
	void setRetweetCount(unsigned int rc) {retweet_count = rc;}
	void setFavorited(bool val) {favorited = val;}
	void setRetweeted(bool val) {retweeted = val;}
};

std::vector<Status> getTimeline(std::string &xml);
std::vector<std::string> getIDs(std::string &xml);
std::vector<User> getUsers(std::string &xml);
std::string getErrorMessage(std::string &xml);
Status getStatus(const Swift::ParserElement::ref &element, const std::string xmlns);
User getUser(const Swift::ParserElement::ref &element, const std::string xmlns);
#endif
