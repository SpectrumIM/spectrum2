#include "ProfileImageRequest.h"
#include "../HTTPRequest.h"
DEFINE_LOGGER(profileImageRequestLogger, "ProfileImageRequest")
void ProfileImageRequest::run()
{
	HTTPRequest req;
	req.init();
	req.setProxy(ip, port, puser, ppasswd);
	success = req.GET(url, callbackdata);
	if(!success) error.assign(req.getCurlError());
}

void ProfileImageRequest::finalize()
{
	Error errResponse;
	if(!success) {
		LOG4CXX_ERROR(profileImageRequestLogger,  user << " - " << error);
		img = "";
		errResponse.setMessage(error);
		callBack(user, buddy, img, reqID, errResponse);
	} else {
		LOG4CXX_INFO(profileImageRequestLogger, user << " - " << callbackdata);
		img = callbackdata;
		callBack(user, buddy, img, reqID, errResponse);
	}
}
