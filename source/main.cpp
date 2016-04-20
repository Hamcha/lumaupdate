#include <string>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <3ds.h>

#include "libs.h"

#define RELEASEURL "https://api.github.com/repos/AuroraWright/AuReiNand/releases/latest"

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

struct ARNRelease {
	std::string name;
	std::string url;
};

struct UpdateArgs {
	std::string payloadPath;
};

UpdateChoice drawConfirmationScreen(const ARNRelease release, const UpdateArgs args, bool configfile) {
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
		menuPrintHeader(&con);
		printf("  Payload path: %s\n\n", args.payloadPath.c_str());
		printf("  Latest version (from Github): %s%s%s\n\n", CONSOLE_WHITE, release.name.c_str(), CONSOLE_RESET);
		menuPrintFooter(&con);
	}

	con.cursorX = 4;
	con.cursorY = 7;
	
	if (configfile == false){
		printf("Configuration file NOT found!\n");
		printf("Assuming default values!\n\n");
	}
	printf("Do you want to install it? ");
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
	std::string originalName = "/" + payloadName;
	std::ifstream original(originalName, std::ifstream::binary);
	if (!original.good()) {
		printf("Could not open %s\n", originalName.c_str());
		return false;
	}

	std::string backupName = originalName + ".bak";
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

bool update(const ARNRelease release, const UpdateArgs args) {
	consoleClear();

	// Back up local file if it exists
	if (args.payloadPath == "arm9loaderhax.bin" || fileExists("/" + args.payloadPath)) {
		printf("Copying %s to %s.bak...\n", args.payloadPath.c_str(), args.payloadPath.c_str());
		gfxFlushBuffers();
		if (!backupA9LH(args.payloadPath)) {
			printf("\nCould not backup %s (!!), aborting...\n", args.payloadPath.c_str());
			return false;
		}
	}
	else {
		printf("Original payload not found, skipping backup...\n");
	}

	printf("Downloading 7z file...\n");
	gfxFlushBuffers();

	u8* fileData = nullptr;
	u32 fileSize = 0;

	try {
		httpGet(release.url.c_str(), &fileData, &fileSize);
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
		printf("Archive opened in memory.\n\nSearching for arm9loaderhax.bin: ");
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
			int res = strncmp(name8, "arm9loaderhax.bin", len - 1);
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
		printf("\nCould not find arm9loaderhax.bin\n");
		SzArEx_Free(&db, &allocImp);
		free(fileData);
		return false;
	}

	printf("\nExtracting arm9loaderhax.bin from archive...\n");
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
		printf("\nCould not extract arm9loaderhax.bin\n");
		gfxFlushBuffers();
		SzArEx_Free(&db, &allocImp);
		free(fileData);
		return false;
	}

	printf("File extracted successfully (%zu bytes)\n", fileOutSize);
	gfxFlushBuffers();

	printf("Saving arm9loaderhax.bin to SD (as %s)...\n", args.payloadPath.c_str());
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

ARNRelease fetchLatestRelease() {
	ARNRelease release;

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
			release.name = std::string((const char*)apiReqData + t[i + 1].start, t[i + 1].end - t[i + 1].start);
			printf("Release found: %s\n", release.name.c_str());
			namefound = true;
		}
		if (!releasefound && jsoneq((const char*)apiReqData, &t[i], "browser_download_url") == 0) {
			release.url = std::string((const char*)apiReqData + t[i + 1].start, t[i + 1].end - t[i + 1].start);
			printf("Asset found: %s\n", release.url.c_str());
			releasefound = true;
		}
		if (namefound && releasefound) {
			break;
		}
	}
	gfxFlushBuffers();
	free(apiReqData);

	return release;
}

int main() {
	UpdateState state = UpdateConfirmationScreen;
	ARNRelease release;
	UpdateArgs updateArgs;

	bool configfile = true;
	
	gfxInitDefault();
	httpcInit(0);

	consoleInit(GFX_TOP, &con);
	consoleDebugInit(debugDevice_CONSOLE);

	// Read config file
	if (std::ifstream("arnupdate.cfg")){
		bool loaded = config.LoadFile("arnupdate.cfg");
		if (!loaded) {
			printf("\n Config corrupted... Skipping. \n");
		}
		else{
			printf("Configuration file loaded successfully.\n");
		}
	}
	else{
		configfile = false;
	}
	

	// Check required values in config
	if (configfile == true){
		if (!config.Has("payload path")) {
			printf("Missing required config value: payload path\n");
			gfxFlushBuffers();
			WAIT_START
			goto cleanup;
		}
	}

	updateArgs.payloadPath = "arm9loaderhax.bin"; //default value

	if (configfile == true){
		updateArgs.payloadPath = config.Get("payload path");
	}

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
			switch (drawConfirmationScreen(release, updateArgs, configfile)) {
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
				con.cursorX = 2;
				con.cursorY = 7;
				printf("Update aborted. Press START to exit.\n");
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

