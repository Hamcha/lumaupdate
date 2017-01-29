#include "HTTP.h"

#include <cybertrust_cer.h>
#include <digicert_cer.h>

constexpr void assert(const int returnValue, const char* errorMessage) {
	if (returnValue != 0) {
		//TODO Format error message a little
		throw std::runtime_error(errorMessage);
	}
}

HTTPRequest::HTTPRequest(const HTTPC_RequestMethod method, const char* url) {
	Setup(method, url);
}

void HTTPRequest::Setup(const HTTPC_RequestMethod method, const char* url) {
	// Open HTTPc context
	assert(httpcOpenContext(&context, method, url, 0), "Could not open HTTPc context");

	// Add User Agent field (required by Github API calls)
	assert(httpcAddRequestHeaderField(&context, (char*)"User-Agent", (char*)"LUMA-UPDATER"), "Could not set User Agent");

	// Add root CA required for Github and AWS URLs
	assert(httpcAddTrustedRootCA(&context, cybertrust_cer, cybertrust_cer_size), "Could not add Cybertrust root CA");
	assert(httpcAddTrustedRootCA(&context, digicert_cer, digicert_cer_size), "Could not add Digicert root CA");
}

int HTTPRequest::Start() {
	for (u8 currentRedirectIteration = 0; currentRedirectIteration < maxRedirects; ++currentRedirectIteration) {
		assert(httpcBeginRequest(&context), "Could not begin request");

		u32 statuscode = 0;
		assert(httpcGetResponseStatusCode(&context, &statuscode), "Could not get status code");
		if (statuscode != 200) {
			// Handle 3xx codes
			if (statuscode >= 300 && statuscode < 400) {
				char newUrl[1024];
				assert(httpcGetResponseHeader(&context, (char*)"Location", newUrl, 1024), "Could not get Location header for 3xx reply");
				assert(httpcCloseContext(&context), "Could not close HTTP context");
				continue;
			}
		}
		return statuscode;
	}

	throw std::runtime_error("Too many redirects");
}

HTTPDownloadStatus HTTPRequest::GetDownloadStatus() {
	// Check current position
	assert(httpcGetDownloadSizeState(&context, &responseCurrentPosition, &responseBodySize), "Could not get file size");
	if (responseData.size() < responseBodySize) {
		responseData.resize(responseBodySize);
	}

	// Get new data
	u32 remaining = responseBodySize - responseCurrentPosition;
	if (blockingDownload) {
		responseCurrentStatus = httpcReceiveData(&context, &responseData.at(responseCurrentPosition), remaining);
	} else {
		responseCurrentStatus = httpcReceiveDataTimeout(&context, &responseData.at(responseCurrentPosition), remaining, downloadTickDuration);
	}

	assert(httpcGetDownloadSizeState(&context, &responseCurrentPosition, &responseBodySize), "Could not get file size");
	HTTPDownloadStatus status;
	status.finished = responseCurrentStatus != (s32)HTTPC_RESULTCODE_DOWNLOADPENDING;
	status.currentSize = responseCurrentPosition;
	status.totalSize = responseBodySize;

	return status;
}

HTTPRequest::~HTTPRequest() {
	assert(httpcCloseContext(&context), "Could not close HTTP context");
}
