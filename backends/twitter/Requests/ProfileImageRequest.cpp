#include "ProfileImageRequest.h"
DEFINE_LOGGER(logger, "ProfileImageRequest")

int ProfileImageRequest::curlCallback(char* data, size_t size, size_t nmemb, ProfileImageRequest* obj)
{
    int writtenSize = 0;
    if(obj && data) {
       obj->callbackdata.append(data, size*nmemb);
	   writtenSize = (int)(size*nmemb);
    }
    return writtenSize;
}

bool ProfileImageRequest::fetchImage()
{
	LOG4CXX_INFO(logger, user << " - Fetching  profile image");

	curl_easy_setopt(curlhandle, CURLOPT_CUSTOMREQUEST, NULL);
    curl_easy_setopt(curlhandle, CURLOPT_ENCODING, "");
    
	img = "";
	callbackdata = "";
    memset(curl_errorbuffer, 0, 1024);
	
	curl_easy_setopt(curlhandle, CURLOPT_ERRORBUFFER, curl_errorbuffer);
	curl_easy_setopt(curlhandle, CURLOPT_WRITEFUNCTION, curlCallback);
	curl_easy_setopt(curlhandle, CURLOPT_WRITEDATA, this);
    	
	/* Set http request and url */
    curl_easy_setopt(curlhandle, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(curlhandle, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curlhandle, CURLOPT_URL, url.c_str());
    
    /* Send http request and return status*/
    return (CURLE_OK == curl_easy_perform(curlhandle));
}


void ProfileImageRequest::run() 
{	
	if(curlhandle == NULL) {
		error = "Failed to init CURL!";
		success = false;
		return;
	}
	success = fetchImage();
	if(!success) error.assign(curl_errorbuffer);
}

void ProfileImageRequest::finalize()
{
	if(!success) {
		LOG4CXX_ERROR(logger,  user << " - " << error)
		img = "";
		callBack(user, buddy, img, reqID, error);
	} else {
		//std::string error = getErrorMessage(replyMsg);
		//if(error.length()) LOG4CXX_ERROR(logger,  user << " - " << error)
		LOG4CXX_INFO(logger, user << " - " << callbackdata);
		img = callbackdata;
		callBack(user, buddy, img, reqID, error);	
	} 
}
