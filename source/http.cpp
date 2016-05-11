#include "http.h"

#include "utils.h"

#include "certs/cybertrust.h"
#include "certs/digicert.h"

int httpGet(const char* url, u8** buf, u32* size, bool verbose) {
	httpcContext context;
	CHECK(httpcOpenContext(&context, HTTPC_METHOD_GET, (char*)url, 0), "Could not open HTTP context");
	// Add User Agent field (required by Github API calls)
	CHECK(httpcAddRequestHeaderField(&context, (char*)"User-Agent", (char*)"LUMA-UPDATER"), "Could not set User Agent");

	CHECK(httpcBeginRequest(&context), "Could not begin request");

	// Add root CA required for Github and AWS URLs
	CHECK(httpcAddTrustedRootCA(&context, cybertrust_cer, cybertrust_cer_len), "Could not add Cybertrust root CA");
	CHECK(httpcAddTrustedRootCA(&context, digicert_cer, digicert_cer_len), "Could not add Digicert root CA");

	u32 statuscode = 0;
	CHECK(httpcGetResponseStatusCode(&context, &statuscode, 0), "Could not get status code");
	if (statuscode != 200) {
		// Handle 3xx codes
		if (statuscode >= 300 && statuscode < 400) {
			char newUrl[1024];
			CHECK(httpcGetResponseHeader(&context, (char*)"Location", newUrl, 1024), "Could not get Location header for 3xx reply");
			CHECK(httpcCloseContext(&context), "Could not close HTTP context");
			return httpGet(newUrl, buf, size, verbose );
		}
		throw formatErrMessage("Non-200 status code", statuscode);
	}

	u32 pos = 0;
	u32 dlstartpos = 0;
	u32 dlpos = 0;
	Result dlret = HTTPC_RESULTCODE_DOWNLOADPENDING;

	CHECK(httpcGetDownloadSizeState(&context, &dlstartpos, size), "Could not get file size");

	*buf = (u8*)std::malloc(*size);
	if (*buf == NULL) throw formatErrMessage("Could not allocate enough memory", *size);
	std::memset(*buf, 0, *size);

	while (pos < *size && dlret == HTTPC_RESULTCODE_DOWNLOADPENDING)
	{
		u32 sz = *size - pos;
		dlret = httpcReceiveData(&context, *buf + pos, sz);
		CHECK(httpcGetDownloadSizeState(&context, &dlpos, NULL), "Could not get file size");
		pos = dlpos - dlstartpos;
		if (verbose) {
			printf("Download progress: %lu / %lu", dlpos, *size);
			gfxFlushBuffers();
		}
	}
	
	if (verbose) {
		printf("\n");
	}

	CHECK(httpcCloseContext(&context), "Could not close HTTP context");

	return 1;
}
