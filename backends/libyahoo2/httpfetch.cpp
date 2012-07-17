
#include "httpfetch.h"
#include "transport/logging.h"

DEFINE_LOGGER(logger, "HTTPFetch");

static int url_to_host_port_path(const char *url,
	char *host, int *port, char *path, int *ssl)
{
	char *urlcopy = NULL;
	char *slash = NULL;
	char *colon = NULL;

	/*
	 * http://hostname
	 * http://hostname/
	 * http://hostname/path
	 * http://hostname/path:foo
	 * http://hostname:port
	 * http://hostname:port/
	 * http://hostname:port/path
	 * http://hostname:port/path:foo
	 * and https:// variants of the above
	 */

	if (strstr(url, "http://") == url) {
		urlcopy = strdup(url + 7);
	} else if (strstr(url, "https://") == url) {
		urlcopy = strdup(url + 8);
		*ssl = 1;
	} else {
		return 0;
	}

	slash = strchr(urlcopy, '/');
	colon = strchr(urlcopy, ':');

	if (!colon || (slash && slash < colon)) {
		if (*ssl)
			*port = 443;
		else
			*port = 80;
	} else {
		*colon = 0;
		*port = atoi(colon + 1);
	}

	if (!slash) {
		strcpy(path, "/");
	} else {
		strcpy(path, slash);
		*slash = 0;
	}

	strcpy(host, urlcopy);

	free(urlcopy);

	return 1;
}

HTTPFetch::HTTPFetch(Swift::BoostIOServiceThread *ioService, Swift::ConnectionFactory *factory) : m_ioService(ioService), m_factory(factory) {
	m_afterHeader = false;
}

HTTPFetch::~HTTPFetch() {
}

void HTTPFetch::_connected(boost::shared_ptr<Swift::Connection> conn, const std::string url, bool error) {
	if (error) {
		_disconnected(conn);
	}
	else {
		char host[255];
		int port = 80;
		char path[255];
		int ssl = 0;
		if (!url_to_host_port_path(url.c_str(), host, &port, path, &ssl))
			return;

		static char buff[2048];
		snprintf(buff, sizeof(buff),
			"GET %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"User-Agent: Mozilla/4.5 [en] (1/1)\r\n"
			"Accept: */*\r\n"
			"%s" "\r\n", path, host,
			"Connection: close\r\n");
		LOG4CXX_INFO(logger, "Sending " << buff << "\n");
		conn->write(Swift::createSafeByteArray(buff));
	}
}

void HTTPFetch::_disconnected(boost::shared_ptr<Swift::Connection> conn) {
	conn->onConnectFinished.disconnect_all_slots();
	conn->onDisconnected.disconnect_all_slots();
	conn->onDataRead.disconnect_all_slots();

	if (m_buffer.size() == 0) {
		onURLFetched("");
	}
	else {
		std::string img = m_buffer.substr(m_buffer.find("\r\n\r\n") + 4);
		onURLFetched(img);
	}
}

void HTTPFetch::_read(boost::shared_ptr<Swift::Connection> conn, boost::shared_ptr<Swift::SafeByteArray> data) {
	std::string d(data->begin(), data->end());
// 			std::cout << d << "\n";
	std::string img = d.substr(d.find("\r\n\r\n") + 4);
	if (d.find("Location: ") == std::string::npos) {
		m_buffer += d;
	}
	else {
		d = d.substr(d.find("Location: ") + 10);
		if (d.find("\r") == std::string::npos) {
			d = d.substr(0, d.find("\n"));
		}
		else {
			d = d.substr(0, d.find("\r"));
		}
		LOG4CXX_INFO(logger, "Next url is '" << d << "'");
		fetchURL(d);
		conn->onConnectFinished.disconnect_all_slots();
		conn->onDisconnected.disconnect_all_slots();
		conn->onDataRead.disconnect_all_slots();
	}
}

bool HTTPFetch::fetchURL(const std::string &url) {
	char host[255];
	int port = 80;
	char path[255];
	char buff[1024];
	int ssl = 0;
	if (!url_to_host_port_path(url.c_str(), host, &port, path, &ssl)) {
		LOG4CXX_ERROR(logger, "Invalid URL " << url);
		return false;
	}

	LOG4CXX_INFO(logger, "Connecting to " << host << ":" << port);

	boost::asio::ip::tcp::resolver resolver(*m_ioService->getIOService());
	boost::asio::ip::tcp::resolver::query query(host, "");
	boost::asio::ip::address address;
	for(boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query); i != boost::asio::ip::tcp::resolver::iterator(); ++i) {
		boost::asio::ip::tcp::endpoint end = *i;
		address = end.address();
		break;
	}

	boost::shared_ptr<Swift::Connection> conn = m_factory->createConnection();
	conn->onConnectFinished.connect(boost::bind(&HTTPFetch::_connected, this, conn, url, _1));
	conn->onDisconnected.connect(boost::bind(&HTTPFetch::_disconnected, this, conn));
	conn->onDataRead.connect(boost::bind(&HTTPFetch::_read, this, conn, _1));
	conn->connect(Swift::HostAddressPort(Swift::HostAddress(address), port));
	return true;
}
