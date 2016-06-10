#pragma once

#include "libs.h"

#define PAYLOADPATH "arm9loaderhax.bin"

struct ReleaseVer {
	std::string filename;
	std::string friendlyName;
	std::string url;
	size_t      fileSize;
};

struct ReleaseInfo {
	std::string name = "";
	std::string description = "";
	std::vector<ReleaseVer> versions = {};
};

/* \brief Gets last official release (from Aurora's Github)
 *
 * \return ReleaseInfo containing the last release name and available versions
 */
ReleaseInfo releaseGetLatestStable();

/* \brief Gets the latest available hourly build (from astronautlevel2's website) 
 *
 * \return ReleaseInfo containing the last hourly
 */
ReleaseInfo releaseGetLatestHourly();

/* \brief Update to stable version
 * Gets the A9LH payload (arm9loaderhax.bin) file from either a stable release or a hourly
 * The buffer must be free'd after used. If the release is not an hourly, the `payloadData` pointer must
 * increased by `offset` to get the correct bytes
 *
 * \param release     Release data
 * \param isHourly    Wether the release is a hourly (.zip) or stable (.7z)
 * \param payloadData Pointer to fill with the payload bytes (should be nullptr when passing)
 * \param offset      Pointer to fill with the offset to the correct bytes (if 7z)
 * \param payloadSize Pointer to fill with size (in bytes) of the payload
 *
 * \return true if everything succeeds, false otherwise
 */
bool releaseGetPayload(ReleaseVer release, bool isHourly, u8** payloadData, size_t* offset, size_t* payloadSize);