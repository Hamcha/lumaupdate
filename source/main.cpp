#include <string>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <3ds.h>

#include "libs.h"


#define PAYLOADPATH "arm9loaderhax.bin"
#define RELEASEURL  "https://api.github.com/repos/AuroraWright/AuReiNand/releases/latest"

#define WAIT_START while (aptMainLoop() && !(hidKeysDown() & KEY_START)) { gspWaitForVBlank(); hidScanInput(); }

bool redraw = false;
PrintConsole con;
Config config;

/* States */

enum UpdateState {
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

struct ReleaseInfo {
	std::string name;
	std::string url;
};

struct UpdateArgs {
	// Detected options
	std::string currentVersion;
	bool        migrateARN;

	// Configuration options
	std::string payloadPath;
	bool        backupExisting;
};

UpdateChoice drawConfirmationScreen(const ReleaseInfo release, const UpdateArgs args, const bool usingConfig) {
	static bool status = false;
	static bool partialredraw = false;

	bool haveLatest = args.currentVersion == release.name;

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
		menuPrintHeader(&con);

		if (!usingConfig){
			printf("  %sConfiguration not found, using default values%s\n\n", CONSOLE_MAGENTA, CONSOLE_RESET);
		}

		printf("  Payload path:   %s%s%s\n", CONSOLE_WHITE, args.payloadPath.c_str(), CONSOLE_RESET);
		printf("  Backup payload: %s%s%s\n\n",
			(args.backupExisting ? CONSOLE_GREEN : CONSOLE_RED),
			(args.backupExisting ? "Yes" : "No"),
			CONSOLE_RESET
		);

		if (args.currentVersion != "") {
			printf("  Current installed version:    %s%s%s\n", (haveLatest ? CONSOLE_GREEN : CONSOLE_RED), args.currentVersion.c_str(), CONSOLE_RESET);
		}
		else {
			printf("  %sCould not detect current version%s\n\n", CONSOLE_MAGENTA, CONSOLE_RESET);
		}
		printf("  Latest version (from Github): %s%s%s\n\n", CONSOLE_GREEN, release.name.c_str(), CONSOLE_RESET);

		if (haveLatest) {
			printf("  You seem to have the latest version already.\n");
		} else {
			printf("  A new version of Luma3DS is available.\n");
		}
		menuPrintFooter(&con);
	}

	con.cursorX = 4;
	con.cursorY = 10 + (usingConfig ? 0 : 3);
	printf("Do you %swant to install it? ", (haveLatest ? "still " : ""));
	printf(status ? "< YES >" : "< NO > ");

	redraw = false;
	partialredraw = false;
	return NoReply;
}

bool fileExists(std::string path) {
	std::ifstream file(path);
	return file.is_open();
}

bool backupA9LH(std::string payloadName) {
	std::ifstream original(payloadName, std::ifstream::binary);
	if (!original.good()) {
		printf("Could not open %s\n", payloadName.c_str());
		return false;
	}

	std::string backupName = payloadName + ".bak";
	std::ofstream target(backupName, std::ofstream::binary);
	if (!target.good()) {
		printf("Could not open %s\n", backupName.c_str());
		original.close();
		return false;
	}

	target << original.rdbuf();

	original.close();
	target.close();
	return true;
}

bool update(const ReleaseInfo release, const UpdateArgs args) {
	consoleClear();

	// Back up local file if it exists
	if (!args.backupExisting) {
		printf("Payload backup is disabled in config, skipping...\n");
	} else if (!fileExists(args.payloadPath)) {
		printf("Original payload not found, skipping backup...\n");
	} else {
		printf("Copying %s to %s.bak...\n", args.payloadPath.c_str(), args.payloadPath.c_str());
		gfxFlushBuffers();
		if (!backupA9LH(args.payloadPath)) {
			printf("\nCould not backup %s (!!), aborting...\n", args.payloadPath.c_str());
			return false;
		}
	}

	printf("Downloading 7z file...\n");
	gfxFlushBuffers();

	u8* fileData = nullptr;
	u32 fileSize = 0;

	try {
#ifdef FAKEDL
		// Read predownloaded file
		std::ifstream predownloaded(release.name + ".7z", std::ios::binary | std::ios::ate);
		fileSize = predownloaded.tellg();
		predownloaded.seekg(0, std::ios::beg);
		fileData = (u8*)malloc(fileSize);
		predownloaded.read((char*)fileData, fileSize);
#else
		httpGet(release.url.c_str(), &fileData, &fileSize);
#endif
	} catch (std::string& e) {
		printf("%s\n", e.c_str());
		return false;
	}
	printf("Download complete! Size: %lu\n", fileSize);
	printf("\nDecompressing archive in memory...\n");
	gfxFlushBuffers();

	CMemInStream memStream;
	MemInStream_Init(&memStream, fileData, fileSize);
	printf("Created 7z InStream, opening as archive...\n");
	gfxFlushBuffers();

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
	u32 codeIndex = UINT32_MAX;
	if (res == SZ_OK) {
		printf("Archive opened in memory.\n\nSearching for %s: ", PAYLOADPATH);
		gfxFlushBuffers();
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
			for (size_t j = 0; j < len; j++) {
				name8[j] = name[j] % 0xff;
			}

			// Check if it's the A9LH payload
			int res = strncmp(name8, PAYLOADPATH, len - 1);
			if (res == 0) {
				codeIndex = i;
				printf("FOUND! (%lu)\n", codeIndex);
				break;
			}
		}
	} else {
		printf("Could not open archive (SzArEx_Open)\n");
		SzArEx_Free(&db, &allocImp);
		free(fileData);
		return false;
	}

	if (codeIndex == UINT32_MAX) {
		printf("ERR\nCould not find %s\n", PAYLOADPATH);
		SzArEx_Free(&db, &allocImp);
		free(fileData);
		return false;
	}

	printf("\nExtracting %s from archive...\n", PAYLOADPATH);
	gfxFlushBuffers();

	u8* fileBuf = nullptr;
	UInt32 blockIndex = UINT32_MAX;
	size_t fileBufSize = 0;
	size_t fileOutSize = 0;
	size_t offset = 0;

	res = SzArEx_Extract(
		&db,
		&memStream.s,
		codeIndex,
		&blockIndex,
		&fileBuf,
		&fileBufSize,
		&offset,
		&fileOutSize,
		&allocImp,
		&allocTempImp
	);
	if (res != SZ_OK) {
		printf("\nCould not extract %s\n", PAYLOADPATH);
		gfxFlushBuffers();
		SzArEx_Free(&db, &allocImp);
		free(fileData);
		return false;
	}

	if (fileOutSize > 0x20000) {
		printf("File is too big to be a valid A9LH payload!\n");
		gfxFlushBuffers();
		IAlloc_Free(&allocImp, fileBuf);
		SzArEx_Free(&db, &allocImp);
		free(fileData);
		return false;
	}

	printf("File extracted successfully (%zu bytes)\n", fileOutSize);
	gfxFlushBuffers();

	if (args.payloadPath != std::string("/") + PAYLOADPATH) {
		printf("Requested payload path is not %s, applying path patch...\n", PAYLOADPATH);
		bool res = pathchange(fileBuf, fileOutSize, args.payloadPath);
		if (!res) {
			IAlloc_Free(&allocImp, fileBuf);
			SzArEx_Free(&db, &allocImp);
			free(fileData);
			return false;
		}
	}

	printf("Saving %s to SD (as %s)...\n", PAYLOADPATH, args.payloadPath.c_str());
	std::ofstream a9lhfile("/" + args.payloadPath, std::ofstream::binary);
	a9lhfile.write((const char*)fileBuf, fileOutSize);
	a9lhfile.close();

	printf("All done, freeing resources and exiting...\n");
	IAlloc_Free(&allocImp, fileBuf);
	SzArEx_Free(&db, &allocImp);
	free(fileData);
	return true;
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
		strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

ReleaseInfo fetchLatestRelease() {
	ReleaseInfo release;

#ifdef FAKEDL
	// Citra doesn't support HTTPc right now, so just fake a successful request
	release.name = "5.2";
	release.url = "https://github.com/AuroraWright/Luma3DS/releases/download/v5.2/Luma3DSv5.2.7z";
#else

	jsmn_parser p;
	jsmn_init(&p);

	bool namefound = false, releasefound = false;
	u8* apiReqData = nullptr;
	u32 apiReqSize = 0;

	printf("Downloading %s...\n", RELEASEURL);

	httpGet(RELEASEURL, &apiReqData, &apiReqSize);

	printf("Downloaded %lu bytes\n", apiReqSize);
	gfxFlushBuffers();

	jsmntok_t t[128];
	int r = jsmn_parse(&p, (const char*)apiReqData, apiReqSize, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		throw formatErrMessage("Failed to parse JSON", r);
	}
	printf("JSON parsed successfully!\n");
	gfxFlushBuffers();

	for (int i = 0; i < r; i++) {
		if (!namefound && jsoneq((const char*)apiReqData, &t[i], "name") == 0) {
			jsmntok_t val = t[i+1];
			// Strip the "v" in front of the version name
			if (apiReqData[val.start] == 'v') {
				val.start += 1;
			}
			release.name = std::string((const char*)apiReqData + val.start, val.end - val.start);
			printf("Release found: %s\n", release.name.c_str());
			namefound = true;
		}
		if (!releasefound && jsoneq((const char*)apiReqData, &t[i], "browser_download_url") == 0) {
			jsmntok_t val = t[i+1];
			release.url = std::string((const char*)apiReqData + val.start, val.end - val.start);
			printf("Asset found: %s\n", release.url.c_str());
			releasefound = true;
		}
		if (namefound && releasefound) {
			break;
		}
	}
	gfxFlushBuffers();
	free(apiReqData);

#endif

	return release;
}

int main() {
	const char* cfgPaths[] = {
		"/lumaupdater.cfg",
		"/3DS/lumaupdater.cfg",
		"/luma/lumaupdater.cfg",
	};
	const size_t cfgPathsLen = sizeof(cfgPaths) / sizeof(cfgPaths[0]);

	UpdateState state = UpdateConfirmationScreen;
	ReleaseInfo release;
	UpdateArgs updateArgs;

	gfxInitDefault();
	httpcInit(0);

	consoleInit(GFX_TOP, &con);
	consoleDebugInit(debugDevice_CONSOLE);

	// Read config file
	bool usingConfig = false;
	for (size_t cfgIndex = 0; !usingConfig && (cfgIndex < cfgPathsLen); cfgIndex++) {
		LoadConfigError confStatus = config.LoadFile(cfgPaths[cfgIndex]);
		switch (confStatus) {
		case CFGE_NOTEXISTS:
			break;
		case CFGE_UNREADABLE:
			printf("FATAL\nConfiguration file is unreadable!\n\nPress START to quit.\n");
			gfxFlushBuffers();
			WAIT_START
			goto cleanup;
		case CFGE_MALFORMED:
			printf("FATAL\nConfiguration file is malformed!\n\nPress START to quit.\n");
			gfxFlushBuffers();
			WAIT_START
			goto cleanup;
		case CFGE_NONE:
			printf("Configuration file loaded successfully.\n");
			usingConfig = true;
			break;
		}
	}

	// Check required values in config, if existing
	if (usingConfig && !config.Has("payload path")) {
		printf("Missing required config value: payload path\n");
		gfxFlushBuffers();
		WAIT_START
		goto cleanup;
	}
	
	if (!usingConfig)
	{
		printf("The configuration file could not be found, skipping...\n");
	}

	// Load config values
	updateArgs.payloadPath = config.Get("payload path", PAYLOADPATH);
	updateArgs.backupExisting = tolower(config.Get("backup", "y")[0]) == 'y';

	// Add initial slash to payload path, if missing
	if (updateArgs.payloadPath[0] != '/') {
		updateArgs.payloadPath = "/" + updateArgs.payloadPath;
	}

	// Check that the payload path is valid
	if (updateArgs.payloadPath.length() > MAXPATHLEN) {
		printf("\nFATAL\nPayload path is too long!\nIt can contain at most %d characters!\n\nPress START to quit.\n", MAXPATHLEN);
		gfxFlushBuffers();
		WAIT_START
		goto cleanup;
	}

	// Try to detect current version
	printf("Trying detection of current payload version...\n");
	updateArgs.currentVersion = versionMemsearch(updateArgs.payloadPath);

	try {
		release = fetchLatestRelease();
	}
	catch (std::string& e) {
		printf("%s\n", e.c_str());
		printf("\nFATAL ERROR\nFailed to obtain required data.\n\nPress START to exit.\n");
		gfxFlushBuffers();
		WAIT_START
		goto cleanup;
	}

	redraw = true;

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();
		u32 kDown = hidKeysDown();

		switch (state) {
		case UpdateConfirmationScreen:
			switch (drawConfirmationScreen(release, updateArgs, usingConfig)) {
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
			if (update(release, updateArgs)) {
				state = UpdateComplete;
			} else {
				state = UpdateFailed;
			}
			redraw = true;
			break;
		case UpdateFailed:
			if (redraw) {
				printf("\n  %sUpdate failed%s. Press START to exit.\n", CONSOLE_RED, CONSOLE_RESET);
				redraw = false;
			}
			break;
		case UpdateComplete:
			if (redraw) {
				consoleClear();
				menuPrintHeader(&con);
				printf("\n  %sUpdate complete.%s", CONSOLE_GREEN, CONSOLE_RESET);
				printf("\n\n  In case something goes wrong you can restore\n  the old payload from %s.bak\n", updateArgs.payloadPath.c_str());
				printf("\n  Press START to reboot.");
				redraw = false;
			}
			if ((kDown & KEY_START) != 0) {
				// Reboot!
				aptOpenSession();
				APT_HardwareResetAsync();
				aptCloseSession();
			}
			break;
		case UpdateAborted:
			if (redraw) {
				printf("\n\n  Update aborted. Press START to exit.");
				redraw = false;
			}
			break;
		}


		if (kDown & KEY_START) {
			break; // Exit to HBMenu
		}

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

