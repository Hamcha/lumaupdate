#include "autoupdate.h"

// jsmn includes
#include "jsmn.h"

// Internal includes
#include "http.h"
#include "utils.h"

UpdaterInfo updaterGetInfo(const char* path) {
	HomebrewLocation location = HomebrewLocation::Unknown;
	HomebrewType type = HomebrewType::Unknown;

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

	return { type, location };
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
				if (url.find("release.zip") != std::string::npos) {
					latest.url = url;
				}
			}
		}
	}

	gfxFlushBuffers();
	std::free(apiReqData);

#ifdef GIT_VER
	latest.isNewer = latest.version < GIT_VER;
#else
	latest.isNewer = false;
#endif

	return latest;
#endif
}