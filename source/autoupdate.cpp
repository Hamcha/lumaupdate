#include "autoupdate.h"

// jsmn includes
#include "jsmn.h"

// Internal includes
#include "archive.h"
#include "http.h"
#include "utils.h"

UpdaterInfo updaterGetInfo(const char* path) {
	HomebrewLocation location = HomebrewLocation::Unknown;
	HomebrewType type = HomebrewType::Unknown;
	std::string sdmcLoc = "";

	if (path != nullptr) {
		std::string source(path);

		// Check for SDMC or 3DSLINK
		if (source.compare(0, 5, "sdmc:") == 0) {
			location = HomebrewLocation::SDMC;
		} else if (source.compare(0, 8, "3dslink:") == 0) {
			location = HomebrewLocation::Remote;
		}

		// Check for Homebrew
		if (source.find(".3dsx") != std::string::npos) {
			type = HomebrewType::Homebrew;
			size_t start = source.find_first_of(':') + 2;
			size_t end = source.find_last_of('/');
			sdmcLoc = source.substr(start, end - start);
		}
	}

#ifdef UNIQUE_ID
	// Check for CIA
	u32 aptid = envGetAptAppId();
	aptOpenSession();
	u64 appid = 0;
	if (APT_GetProgramID(&appid) == 0) {
		if (appid & UNIQUE_ID == UNIQUE_ID) {
			type = HomebrewType::CIA;
		}
	}
	aptCloseSession();
#endif

	return { type, location, sdmcLoc };
}

#ifndef FAKEDL
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
		std::strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}
#endif

LatestUpdaterInfo updaterGetLatest() {
#ifdef FAKEDL
	return {};
#else
	static const char* ReleaseURL = "https://api.github.com/repos/Hamcha/lumaupdate/releases/latest";

	jsmn_parser p = {};
	jsmn_init(&p);

	u8* apiReqData = nullptr;
	u32 apiReqSize = 0;

	std::printf("Downloading %s...\n", ReleaseURL);

	httpGet(ReleaseURL, &apiReqData, &apiReqSize, true);

	std::printf("Downloaded %lu bytes\n", apiReqSize);
	gfxFlushBuffers();

	jsmntok_t t[512] = {};
	int r = jsmn_parse(&p, (const char*)apiReqData, apiReqSize, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		throw formatErrMessage("Failed to parse JSON", r);
	}
	std::printf("JSON parsed successfully!\n");
	gfxFlushBuffers();

	bool namefound = false, bodyfound = false, inassets = false;
	LatestUpdaterInfo latest;
	for (int i = 0; i < r; i++) {
		if (!namefound && jsoneq((const char*)apiReqData, &t[i], "tag_name") == 0) {
			jsmntok_t val = t[i + 1];
			latest.version = std::string((const char*)apiReqData + val.start, val.end - val.start);
			std::printf("Release found: %s\n", latest.version.c_str());
			namefound = true;
		}
		if (!bodyfound && jsoneq((const char*)apiReqData, &t[i], "body") == 0) {
			jsmntok_t val = t[i + 1];
			latest.changelog = unescape(std::string((const char*)apiReqData + val.start, val.end - val.start));
			std::printf("Changelog found.\n");
			bodyfound = true;
		}
		if (!inassets && jsoneq((const char*)apiReqData, &t[i], "assets") == 0) {
			inassets = true;
		}
		if (inassets) {
			if (jsoneq((const char*)apiReqData, &t[i], "browser_download_url") == 0) {
				jsmntok_t val = t[i + 1];
				std::string url = std::string((const char*)apiReqData + val.start, val.end - val.start);
				if (url.find(".zip") != std::string::npos) {
					latest.url = url;
				}
			}
		}
	}

	gfxFlushBuffers();
	std::free(apiReqData);

#ifdef GIT_VER
	latest.isNewer = latest.version > GIT_VER;
#else
	latest.isNewer = false;
#endif

	return latest;
#endif
}

static void installCIA(const u8* ciaData, const size_t ciaSize) {
	Handle handle;
	AM_QueryAvailableExternalTitleDatabase(NULL);
	CHECK(AM_StartCiaInstall(MEDIATYPE_SD, &handle), "Cannot initialize CIA install");
	try {
		CHECK(FSFILE_Write(handle, NULL, 0, ciaData, (u32)ciaSize, NULL), "Cannot write CIA data to handle");
		CHECK(AM_FinishCiaInstall(handle), "Cannot finalize CIA install");
	} catch (const std::runtime_error& e) {
		// Abort CIA install and re-throw
		CHECK(AM_CancelCIAInstall(handle), "Cannot cancel CIA install");
		throw e;
	}
}

static void copyToFile(const std::string& path, const u8* fileData, const size_t fileSize) {
	std::ofstream fout(path, std::ios::binary | std::ios::out);
	fout.write((const char*)fileData, fileSize);
	fout.close();
}

void updaterDoUpdate(LatestUpdaterInfo latest, UpdaterInfo current) {
	//TODO Download payload
	u8* archiveData = nullptr;
	u32 archiveSize = 0;
	HTTPResponseInfo info;

	httpGet(latest.url.c_str(), &archiveData, &archiveSize, true, &info);
	std::printf("Download complete! Size: %lu\n", archiveSize);

	if (info.etag != "") {
		std::printf("Performing integrity check... ");
		if (!httpCheckETag(info.etag, archiveData, archiveSize)) {
			std::printf(" ERR\r\n");
			throw std::runtime_error("MD5 mismatch between server's and local file!\r\n");
		}
		std::printf(" OK\r\n");
	} else {
		std::printf("Skipping integrity check (no ETag found)\r\n");
	}

	ZipArchive archive(archiveData, archiveSize);
	
	switch (current.type) {
	case HomebrewType::CIA:
		// Extract CIA from archive, install it
		u8* ciaData;
		size_t ciaSize;
		archive.extractFile("lumaupdater.cia", &ciaData, &ciaSize);
		installCIA(ciaData, ciaSize);
		break;
	case HomebrewType::Homebrew:
		// Extract 3dsx/smdh from archive
		u8* hbData;
		size_t hbSize;
		archive.extractFile("3DS/lumaupdater/lumaupdater.3dsx", &hbData, &hbSize);
		copyToFile(current.sdmcLoc + "/lumaupdater.3dsx", hbData, hbSize);
		std::free(hbData);

		u8* smdhData;
		size_t smdhSize;
		archive.extractFile("3DS/lumaupdater/lumaupdater.smdh", &smdhData, &smdhSize);
		copyToFile(current.sdmcLoc + "/lumaupdater.smdh", hbData, hbSize);
		std::free(smdhData);
		break;
	default:
		throw std::domain_error("Trying to update an unknown install");
	}
}