
#include "yahoolocalaccount.h"
#include "yahoohandler.h"

YahooLocalAccount::YahooLocalAccount(const std::string &user, const std::string &legacyName, const std::string &password) : user(user), id(0), conn_tag(1), handler_tag(1), status(YAHOO_STATUS_OFFLINE), msg(""), buffer("") {
	id = yahoo_init_with_attributes(legacyName.c_str(), password.c_str(), 
			"local_host", "",
			"pager_port", 5050,
			NULL);
}

YahooLocalAccount::~YahooLocalAccount() {
	// remove handlers
	for (std::map<int, YahooHandler *>::iterator it = handlers.begin(); it != handlers.end(); it++) {
		delete it->second;
	}

	// remove conns
	for (std::map<int, boost::shared_ptr<Swift::Connection> >::iterator it = conns.begin(); it != conns.end(); it++) {
		it->second->onConnectFinished.disconnect_all_slots();
		it->second->onDisconnected.disconnect_all_slots();
		it->second->onDataRead.disconnect_all_slots();
		it->second->onDataWritten.disconnect_all_slots();
	}
}

void YahooLocalAccount::login() {
	yahoo_login(id, YAHOO_STATUS_AVAILABLE);
}

void YahooLocalAccount::addHandler(YahooHandler *handler) {
	handlers[handler->handler_tag] = handler;
	handlers_per_conn[handler->conn_tag][handler->handler_tag] = handler;
}

void YahooLocalAccount::removeOldHandlers() {
	std::vector<int> handlers_to_remove;
	for (std::map<int, YahooHandler *>::iterator it = handlers.begin(); it != handlers.end(); it++) {
		if (it->second->remove_later) {
			handlers_to_remove.push_back(it->first);
		}
	}

	BOOST_FOREACH(int tag, handlers_to_remove) {
		YahooHandler *handler = handlers[tag];
		handlers.erase(tag);
		handlers_per_conn[handler->conn_tag].erase(tag);
		delete handler;
	}
}

void YahooLocalAccount::removeConn(int conn_tag) {
	for (std::map<int, YahooHandler *>::iterator it = handlers_per_conn[conn_tag].begin(); it != handlers_per_conn[conn_tag].end(); it++) {
		it->second->remove_later = true;
	}

	removeOldHandlers();

	conns.erase(conn_tag);
}
