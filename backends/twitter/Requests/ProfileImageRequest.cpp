#include "ProfileImageRequest.h"
#include "../HTTPRequest.h"
DEFINE_LOGGER(logger, "ProfileImageRequest")
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
		LOG4CXX_ERROR(logger,  user << " - " << error)
		img = "";
		errResponse.setMessage(error);
		callBack(user, buddy, img, reqID, errResponse);
	} else {
		LOG4CXX_INFO(logger, user << " - " << callbackdata);
		img = callbackdata;
		callBack(user, buddy, img, reqID, errResponse);	
	} 
}
