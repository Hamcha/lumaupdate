#pragma once

#include "libs.h"

struct HTTPDownloadStatus {
	bool finished = false;
	u32 totalSize = 0;
	u32 currentSize = 0;
};

class HTTPRequest {
public:
	//! Handle redirects (3xx codes) by automatically requesting the redirect URL
	bool handleRedirect = true;

	//! How many redirects to follow before giving up
	u8 maxRedirects = 10;

	//! Use blocking download (download entire file in one call)
	bool blockingDownload = false;

	//! Maximum time (in nanoseconds) a download tick can take (default 10ms)
	u64 downloadTickDuration = 10000000;

	/*! \brief Create an HTTP Request
	 *
	 *  \param method Request method (GET/POST/..)
	 *  \param url    Request URL (including protocol and hostname)
	 *
	 *  \return HTTP Request object
	 */
	HTTPRequest(const HTTPC_RequestMethod method, const char* url);

	/*! \brief Uninitialize HTTP request, free buffers and context */
	~HTTPRequest();

	/*! \brief Start the request */
	int Start();

	/*! \brief Fill up the buffer with downloaded data and returns progress status
	 *
	 *  \return Download status
	 */
	HTTPDownloadStatus GetDownloadStatus();

private:
	httpcContext context;

	// Response data
	u32             responseBodySize = 0;
	u32             responseCurrentPosition = 0;
	std::vector<u8> responseData = {};
	Result          responseCurrentStatus = HTTPC_RESULTCODE_DOWNLOADPENDING;

	void Setup(const HTTPC_RequestMethod method, const char* url);
};
