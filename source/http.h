#pragma once

#include "libs.h"

/*! \brief Optional extra httpGet informations */
struct HTTPResponseInfo {
	std::string etag; //!< ETag (for AWS S3 requests)
};

/*! \brief Makes a GET HTTP request
 *  This function will throw an exception if it encounters any error
 *
 *  \param url     URL to download
 *  \param buf     Output buffer (will be allocated by the function)
 *  \param size    Output buffer size
 *  \param verbose OPTIONAL Write download progress to screen (via printf)
 *  \param info    OPTIONAL Pointer to HTTPResponseInfo struct to fill with extra data
 *
 *  \return 1 if the request completed successfully.
 */
int httpGet(const char* url, u8** buf, u32* size, const bool verbose = false, HTTPResponseInfo* info = nullptr);