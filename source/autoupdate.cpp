#include "autoupdate.h"

// jsmn includes
#include "jsmn.h"

// Internal includes
#include "http.h"
#include "utils.h"

UpdaterInfo updaterGetInfo(const std::string& source) {
	HomebrewLocation location = HbLocUnknown;
	HomebrewType type = HbTypeUnknown;

	// Check for SDMC or 3DSLINK
	if (source.find("sdmc:") == 0) {
		location = HbLocSDMC;
	} else if (source.find("3dslink:") == 0) {
		location = HbLoc3DSLink;
	}

	// Check for Homebrew
	if (source.find(".3dsx") != std::string::npos) {
		type = HbType3DSX;
	}

	//TODO Check for CIA (build-time constant maybe?)

	return { type, location };
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
		std::strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

LatestUpdaterInfo updaterGetLatest() {
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

	return latest;
}