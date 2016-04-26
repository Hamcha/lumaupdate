#include "http.h"

#include <stdlib.h>
#include <cstring>

#include "utils.h"

#include "certs/cybertrust.h"
#include "certs/digicert.h"

int httpGet(const char* url, u8** buf, u32* size) {
	httpcContext context;
	CHECK(httpcOpenContext(&context, HTTPC_METHOD_GET, (char*)url, 0), "Could not open HTTP context");
	// Add User Agent field (required by Github API calls)
	CHECK(httpcAddRequestHeaderField(&context, (char*)"User-Agent", (char*)"ARN-UPDATER"), "Could not set User Agent");

	CHECK(httpcBeginRequest(&context), "Could not begin request");

	// Add root CA required for Github and AWS URLs
	CHECK(httpcAddTrustedRootCA(&context, digicert_cer, digicert_cer_len), "Could not add Digicert root CA");
	CHECK(httpcAddTrustedRootCA(&context, cybertrust_cer, cybertrust_cer_len), "Could not add Cybertrust root CA");

	u32 statuscode = 0;
	CHECK(httpcGetResponseStatusCode(&context, &statuscode, 0), "Could not get status code");
	if (statuscode != 200) {
		// Handle 3xx codes
		if (statuscode >= 300 && statuscode < 400) {
			char newUrl[1024];
			CHECK(httpcGetResponseHeader(&context, (char*)"Location", newUrl, 1024), "Could not get Location header for 3xx reply");
			CHECK(httpcCloseContext(&context), "Could not close HTTP context");
			return httpGet(newUrl, buf, size);
		}
		throw formatErrMessage("Non-200 status code", statuscode);
	}

	CHECK(httpcGetDownloadSizeState(&context, NULL, size), "Could not get file size");

	*buf = (u8*)malloc(*size);
	if (*buf == NULL) throw formatErrMessage("Could not allocate enough memory", *size);
	memset(*buf, 0, *size);

	CHECK(httpcDownloadData(&context, *buf, *size, NULL), "Could not download data");

	CHECK(httpcCloseContext(&context), "Could not close HTTP context");

	return 1;
}
