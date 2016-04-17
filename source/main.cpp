/* stdlib includes */
#include <string>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ctrulib includes */
#include <3ds.h>

/* 7z includes */
#include "7z/7z.h"
#include "7z/7zAlloc.h"
#include "7z/7zCrc.h"
#include "7z/7zMemInStream.h"

/* Local includes */
#include "jsmn.h"
#include "digicert.h"
#include "cybertrust.h"

#define RELEASEURL "https://api.github.com/repos/AuroraWright/AuReiNand/releases/latest"
#define VERSION "0.1.0"

/* Console utils */

bool redraw = false;
PrintConsole con;

void printHeader() {
	printf("\n  ARN Updater v%s\n\n", VERSION);
}

void printFooter() {
	con.cursorX = 2;
	con.cursorY = con.consoleHeight - 1;
	printf("< > select options   A choose   START quit");
}

std::string formatErrMessage(std::string msg, Result val) {
	std::ostringstream os;
	os << msg << "\nRet code: " << val;
	return os.str();
}

/* HTTP util */

#define CHECK(val, msg) if (val != 0) { throw formatErrMessage(msg, val); }

int HTTPGet(const std::string url, u8** buf, u32* size) {
	httpcContext context;
	std::string out;
	CHECK(httpcOpenContext(&context, HTTPC_METHOD_GET, (char*)url.c_str(), 0), "Could not open HTTP context");
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
			return HTTPGet(std::string(newUrl), buf, size);
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

/* States */

enum UpdateState {
	ObtainingRequiredInfo,
	UpdateConfirmationScreen,
	Updating,
	UpdateComplete,
	UpdateFailed,
	UpdateAborted
};

enum UpdateChoice {
	NoReply,
	Yes,
	No
};

UpdateChoice drawConfirmationScreen(std::string name, std::string url) {
	static bool status = false;
	static bool partialredraw = false;

	u32 keydown = hidKeysDown();
	if (keydown & (KEY_RIGHT | KEY_LEFT)) {
		partialredraw = true;
		status = !status;
	}
	if (keydown & KEY_A) {
		return status ? Yes : No;
	}

	if (!redraw && !partialredraw) {
		return NoReply;
	}

	if (redraw) {
		consoleClear();
		printHeader();
		printf("  Latest version (from Github): %s\n\n", name.c_str());
		printFooter();
	}

	con.cursorX = 4;
	con.cursorY = 5;
	printf("Do you want to install it? ");
	printf(status ? "< YES >" : "< NO > ");

	redraw = false;
	partialredraw = false;
	return NoReply;
};

bool update(std::string name, std::string url) {
	consoleClear();

	printf("Downloading 7z file...\n");

	u8* fileData = nullptr;
	u32 fileSize = 0;

	try {
		HTTPGet(url, &fileData, &fileSize);
	} catch (std::string e) {
		printf("%s\n", e.c_str());
		return false;
	}
	printf("Download complete! Size: %lu\n", fileSize);

	printf("\nDecompressing archive in memory...\n");

	CMemInStream memStream;
	MemInStream_Init(&memStream, fileData, fileSize);
	printf("Created 7z InStream, opening as archive...\n");

	CSzArEx db;
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;
	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;
	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;

	CrcGenerateTable();
	SzArEx_Init(&db);

	SRes res = SzArEx_Open(&db, &memStream.s, &allocImp, &allocTempImp);
	int codeIndex = -1;
	if (res == SZ_OK) {
		printf("Archive opened in memory.\n\nFiles:\n");
		for (u32 i = 0; i < db.NumFiles; i++) {
			// Skip directories
			unsigned isDir = SzArEx_IsDir(&db, i);
			if (isDir) {
				continue;
			}

			// Get name
			size_t len;
			len = SzArEx_GetFileNameUtf16(&db, i, NULL);
			// Super long filename? Just skip it..
			if (len >= 256) {
				continue;
			}
			u16 name[256] = { 0 };
			SzArEx_GetFileNameUtf16(&db, i, name);

			// Convert name to ASCII (just cut the other bytes)
			char name8[256] = { 0 };
			for (int j = 0; j < len; j++) {
				name8[j] = name[j] % 0xff;
			}

			// Check if it's the A9LH payload
			int res = strncmp(name8, "arm9loaderhax.bin", len - 1);
			if (res == 0) {
				codeIndex = i;
			}

			wprintf(L"  - %ls (%d)\n", (wchar_t*)name, res, sizeof(wchar_t));
		}
	} else {
		printf("Could not open archive (SzArEx_Open)\n");
		return false;
	}

	if (codeIndex < 0) {
		printf("\nCould not find arm9loaderhax.bin\n");
		return false;
	}

	printf("\nFound arm9loaderhax.bin (index %d)\n", codeIndex);
	return true;
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
		strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

int main() {
	jsmn_parser p;
	jsmn_init(&p);

	gfxInitDefault();
	httpcInit(0);

	consoleInit(GFX_TOP, &con);
	consoleDebugInit(debugDevice_CONSOLE);

	u8* apiReqData = nullptr;
	u32 apiReqSize = 0;
	int ret = 0;

	UpdateState state = ObtainingRequiredInfo;

	bool namefound = false, releasefound = false;
	std::string name, url;

	try {
		printf("Downloading %s...\n", RELEASEURL);

		ret = HTTPGet(RELEASEURL, &apiReqData, &apiReqSize);

		printf("Downloaded %lu bytes\n", apiReqSize);
		gfxFlushBuffers();

		jsmntok_t t[128];
		int r = jsmn_parse(&p, (const char*)apiReqData, apiReqSize, t, sizeof(t) / sizeof(t[0]));
		if (r < 0) {
			throw formatErrMessage("Failed to parse JSON", r);
		}
		printf("JSON parsed successfully!\n");

		for (int i = 0; i < r; i++) {
			if (!namefound && jsoneq((const char*)apiReqData, &t[i], "name") == 0) {
				name = std::string((const char*)apiReqData + t[i + 1].start, t[i + 1].end - t[i + 1].start);
				printf("Release found: %s\n", name.c_str());
				namefound = true;
			}
			if (!releasefound && jsoneq((const char*)apiReqData, &t[i], "browser_download_url") == 0) {
				url = std::string((const char*)apiReqData + t[i + 1].start, t[i + 1].end - t[i + 1].start);
				printf("Asset found: %s\n", url.c_str());
				releasefound = true;
			}
			if (namefound && releasefound) {
				break;
			}
		}

		state = UpdateConfirmationScreen;
		redraw = true;
	}
	catch (std::string e) {
		printf("%s\n", e.c_str());
		gfxFlushBuffers();
		ret = 0;
	}

	free(apiReqData);

	if (!ret) {
		printf("\nFailed to obtain required data. Press START to exit.\n");
		gfxFlushBuffers();
		while (aptMainLoop() && !(hidKeysDown() & KEY_START))
		{
			gspWaitForVBlank();
			hidScanInput();
		}
		goto cleanup;
	}

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		switch (state) {
		case UpdateConfirmationScreen:
			switch (drawConfirmationScreen(name, url)) {
			case Yes:
				state = Updating;
				redraw = true;
				break;
			case No:
				state = UpdateAborted;
				redraw = true;
				break;
			case NoReply:
				break;
			}
			break;
		case Updating:
			if (update(name, url)) {
				state = UpdateComplete;
			} else {
				state = UpdateFailed;
			}
			redraw = true;
			break;
		case UpdateFailed:
			if (redraw) {
				printf("\n  Update failed. Press START to exit.\n");
				redraw = false;
			}
			break;
		case UpdateAborted:
			if (redraw) {
				con.cursorX = 2;
				con.cursorY = 7;
				printf("Update aborted. Press START to exit.\n");
				redraw = false;
			}
			break;
		}


		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

cleanup:
	// Exit services
	httpcExit();
	gfxExit();
	return 0;
}

