#include "http.h"

#include "utils.h"

#include "certs/cybertrust.h"
#include "certs/digicert.h"

// libmd5-rfc includes
#include "md5/md5.h"

void httpGet(const char* url, u8** buf, u32* size, const bool verbose, HTTPResponseInfo* info) {
	httpcContext context;
	CHECK(httpcOpenContext(&context, HTTPC_METHOD_GET, (char*)url, 0), "Could not open HTTP context");
	// Add User Agent field (required by Github API calls)
	CHECK(httpcAddRequestHeaderField(&context, (char*)"User-Agent", (char*)"LUMA-UPDATER"), "Could not set User Agent");

	// Add root CA required for Github and AWS URLs
	CHECK(httpcAddTrustedRootCA(&context, cybertrust_cer, cybertrust_cer_len), "Could not add Cybertrust root CA");
	CHECK(httpcAddTrustedRootCA(&context, digicert_cer, digicert_cer_len), "Could not add Digicert root CA");

	CHECK(httpcBeginRequest(&context), "Could not begin request");

	u32 statuscode = 0;
	CHECK(httpcGetResponseStatusCode(&context, &statuscode, 0), "Could not get status code");
	if (statuscode != 200) {
		// Handle 3xx codes
		if (statuscode >= 300 && statuscode < 400) {
			char newUrl[1024];
			CHECK(httpcGetResponseHeader(&context, (char*)"Location", newUrl, 1024), "Could not get Location header for 3xx reply");
			CHECK(httpcCloseContext(&context), "Could not close HTTP context");
			httpGet(newUrl, buf, size, verbose, info);
			return;
		}
		throw formatErrMessage("Non-200 status code", statuscode);
	}

	// Retrieve extra info if required
	if (info != nullptr) {
		char etagChr[512] = { 0 };
		if (httpcGetResponseHeader(&context, (char*)"Etag", etagChr, 512) == 0) {
			info->etag = std::string(etagChr);
		}
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
			std::printf("Download progress: %lu / %lu", dlpos, *size);
			gfxFlushBuffers();
		}
	}
	
	if (verbose) {
		std::printf("\n");
	}

	CHECK(httpcCloseContext(&context), "Could not close HTTP context");
}


bool httpCheckETag(std::string etag, const u8* fileData, const u32 fileSize) {
	// Strip quotes from either side of the etag
	if (etag[0] == '"') {
		etag = etag.substr(1, etag.length() - 2);
	}

	// Get MD5 bytes from Etag header
	md5_byte_t expected[16];
	const char* etagchr = etag.c_str();
	for (u8 i = 0; i < 16; i++) {
		std::sscanf(etagchr + (i * 2), "%02x", &expected[i]);
	}

	// Calculate MD5 hash of downloaded archive
	md5_state_t state;
	md5_byte_t result[16];
	md5_init(&state);
	md5_append(&state, (const md5_byte_t *)fileData, fileSize);
	md5_finish(&state, result);

	return memcmp(expected, result, 16) == 0;
}
