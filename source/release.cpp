#include "release.h"

// 7z includes
#include "7z/7z.h"
#include "7z/7zAlloc.h"
#include "7z/7zCrc.h"
#include "7z/7zMemInStream.h"

// minizip includes
#include "minizip/ioapi_mem.h"
#include "minizip/unzip.h"

// jsmn includes
#include "jsmn.h"

// libmd5-rfc includes
#include "md5/md5.h"

// Internal includes
#include "http.h"
#include "utils.h"

#ifndef FAKEDL
static int jsoneq(const char *json, const jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
		std::strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}
#endif

static bool checkEtag(std::string etag, const u8* fileData, const u32 fileSize) {
	// Strip quotes from either side of the etag
	if (etag[0] == '"') {
		etag = etag.substr(1, etag.length() - 2);
	}

	// Get MD5 bytes from Etag header
	md5_byte_t expected[16];
	const char* etagchr = etag.c_str();
	for (u8 i = 0; i < 16; i++) {
		std::sscanf(etagchr + (i*2), "%02x", &expected[i]);
	}

	// Calculate MD5 hash of downloaded archive
	md5_state_t state;
	md5_byte_t result[16];
	md5_init(&state);
	md5_append(&state, (const md5_byte_t *)fileData, fileSize);
	md5_finish(&state, result);

	return memcmp(expected, result, 16) == 0;
}

ReleaseInfo releaseGetLatestStable() {
	ReleaseInfo release;

#ifdef FAKEDL
	// Citra doesn't support HTTPc right now, so just fake a successful request
	release.name = "5.2";
	release.description = "- Remade the chainloader to only try to load the right payload for the pressed button. Now the only buttons which have a matching payload will actually do something during boot\r\n- Got rid of the default payload (start now boots \"start_NAME.bin\")\r\n- sel_NAME.bin is now select_NAME.bin as there are no more SFN/8.3 limitations anymore\r\n\r\nRefer to [the wiki](https://github.com/AuroraWright/Luma3DS/wiki/Installation-and-Upgrade#upgrading-from-v531) for upgrade instructions.";
	release.versions.push_back(ReleaseVer{ "CITRA", "CITRA", "https://github.com/AuroraWright/Luma3DS/releases/download/v5.2/Luma3DSv5.2.7z", 143234 });
#else

	static const char* ReleaseURL = "https://api.github.com/repos/AuroraWright/Luma3DS/releases/latest";

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
	bool verHasName = false, verHasURL = false, verHasSize = false, isDev = false;
	ReleaseVer current;
	for (int i = 0; i < r; i++) {
		if (!namefound && jsoneq((const char*)apiReqData, &t[i], "name") == 0) {
			jsmntok_t val = t[i+1];
			// Strip the "v" in front of the version name
			if (apiReqData[val.start] == 'v') {
				val.start += 1;
			}
			release.name = std::string((const char*)apiReqData + val.start, val.end - val.start);
			std::printf("Release found: %s\n", release.name.c_str());
			namefound = true;
		}
		if (!bodyfound && jsoneq((const char*)apiReqData, &t[i], "body") == 0) {
			jsmntok_t val = t[i+1];
			release.description = unescape(std::string((const char*)apiReqData + val.start, val.end - val.start));
			std::printf("Release description found.\n");
			bodyfound = true;
		}
		if (!inassets && jsoneq((const char*)apiReqData, &t[i], "assets") == 0) {
			inassets = true;
		}
		if (inassets) {
			if (jsoneq((const char*)apiReqData, &t[i], "name") == 0) {
				jsmntok_t val = t[i+1];
				current.filename = std::string((const char*)apiReqData + val.start, val.end - val.start);
				isDev = current.filename.find("-dev.") < std::string::npos;
				current.friendlyName = isDev ? "developer version" : "normal version";
				verHasName = true;
			}
			if (jsoneq((const char*)apiReqData, &t[i], "browser_download_url") == 0) {
				jsmntok_t val = t[i+1];
				current.url = std::string((const char*)apiReqData + val.start, val.end - val.start);
				verHasURL = true;
			}
			if (jsoneq((const char*)apiReqData, &t[i], "size") == 0) {
				jsmntok_t val = t[i + 1];
				std::string sizeStr = std::string((const char*)apiReqData + val.start, val.end - val.start);
				current.fileSize = std::atoi(sizeStr.c_str());
				verHasSize = true;
			}
			if (verHasName && verHasURL) {
				std::printf("Found version: %s\n", current.filename.c_str());
				ReleaseVer version = ReleaseVer{ current.filename, current.friendlyName, current.url, current.fileSize };
				// Put normal version in front, dev on back
				if (!isDev) {
					release.versions.insert(release.versions.begin(), version);
				} else {
					release.versions.push_back(version);
				}
				verHasName = verHasURL = verHasSize = isDev = false;
			}
		}
	}

	gfxFlushBuffers();
	std::free(apiReqData);

#endif

	return release;
}

ReleaseInfo releaseGetLatestHourly() {
	ReleaseInfo hourly;

#ifdef FAKEDL
	// Citra doesn't support HTTPc right now, so just fake a successful request
	hourly.name = "aaaaaaa";
	hourly.versions.push_back(ReleaseVer{ "CITRA", "latest hourly (aaaaaaa)", "https://github.com/AuroraWright/Luma3DS/releases/download/v5.2/Luma3DSv5.2.7z", 143234 });
#else

	static const char* LastCommitURL = "https://raw.githubusercontent.com/astronautlevel2/Luma3DS/gh-pages/lastCommit";
	static const char* LastDevCommitURL = "https://raw.githubusercontent.com/astronautlevel2/Luma3DSDev/gh-pages/lastCommit";

	static const char* versions[] = { LastCommitURL, LastDevCommitURL };
	static const char* vertypes[] = { "hourly", "dev hourly" };
	static const std::string verurls[] = {
		"https://astronautlevel2.github.io/Luma3DS/builds/Luma-",
		"https://astronautlevel2.github.io/Luma3DSDev/builds/Luma-",
	};


	static const int versionCount = sizeof(versions) / sizeof(versions[0]);

	for (int i = 0; i < versionCount; i++) {
		u8* apiReqData = nullptr;
		u32 apiReqSize = 0;

		std::printf("Downloading %s...\n", versions[i]);

		try {
			httpGet(versions[i], &apiReqData, &apiReqSize, true);
		} catch (std::string& e) {
			std::printf("Could not download, skipping...");
			continue;
		}

		std::printf("Downloaded %lu bytes\n", apiReqSize);
		gfxFlushBuffers();

		std::string hourlyName = std::string((const char*)apiReqData, apiReqSize);
		trim(hourlyName);

		// Use stable's hourly id as "latest" since it gets updated more often
		if (i == 0) {
			hourly.name = hourlyName;
		}

		std::string url = verurls[i] + hourlyName + ".zip";

		hourly.versions.push_back(ReleaseVer { hourlyName, "latest " + std::string(vertypes[i]) + " (" + hourlyName + ")", std::string(url), 0 });
		hourly.commits[std::string(vertypes[i])] = hourlyName;

		std::free(apiReqData);
	}

#endif

	return hourly;
}

static bool extract7z(u8* fileData, const size_t fileSize, u8** payloadData, size_t* offset, size_t* payloadSize) {
	CMemInStream memStream;
	MemInStream_Init(&memStream, fileData, fileSize);
	std::printf("Created 7z InStream, opening as archive...\n");
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
	if (res != SZ_OK) {
		std::printf("Could not open archive (SzArEx_Open)\n");
		SzArEx_Free(&db, &allocImp);
		std::free(fileData);
		return false;
	}

	std::printf("Archive opened in memory.\n\nSearching for %s: ", PAYLOADPATH);
	gfxFlushBuffers();
	for (u32 i = 0; i < db.NumFiles; ++i) {
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
		for (size_t j = 0; j < len; ++j) {
			name8[j] = name[j] % 0xff;
		}

		// Check if it's the A9LH payload
		int res = strncmp(name8, PAYLOADPATH, len - 1);
		if (res == 0) {
			codeIndex = i;
			std::printf("FOUND! (%lu)\n", codeIndex);
			break;
		}
	}

	if (codeIndex == UINT32_MAX) {
		std::printf("ERR\nCould not find %s\n", PAYLOADPATH);
		SzArEx_Free(&db, &allocImp);
		std::free(fileData);
		return false;
	}

	std::printf("\nExtracting %s from archive...\n", PAYLOADPATH);
	gfxFlushBuffers();

	UInt32 blockIndex = UINT32_MAX;
	size_t fileBufSize = 0;

	res = SzArEx_Extract(
		&db,
		&memStream.s,
		codeIndex,
		&blockIndex,
		payloadData,
		&fileBufSize,
		offset,
		payloadSize,
		&allocImp,
		&allocTempImp
	);
	if (res != SZ_OK) {
		std::printf("\nCould not extract %s\n", PAYLOADPATH);
		gfxFlushBuffers();
		SzArEx_Free(&db, &allocImp);
		std::free(fileData);
		return false;
	}

	std::printf("File extracted successfully (%zu bytes)\n", *payloadSize);
	gfxFlushBuffers();

	SzArEx_Free(&db, &allocImp);
	return true;
}

static bool extractZip(u8* fileData, const size_t fileSize, u8** payloadData, size_t* payloadSize) {
	zlib_filefunc_def filefunc32 = {};
	ourmemory_t unzmem = {};
	unz_file_info payloadInfo = {};
	bool success = false;

	unzmem.size = fileSize;
	unzmem.base = (char*)malloc(unzmem.size);
	std::memcpy(unzmem.base, fileData, unzmem.size);

	fill_memory_filefunc(&filefunc32, &unzmem);

	unzFile zipfile = unzOpen2("__notused__", &filefunc32);
	
	int res = unzLocateFile(zipfile, (std::string("out/") + PAYLOADPATH).c_str(), nullptr);
	if (res == UNZ_END_OF_LIST_OF_FILE) {
		std::printf("ERR Could not find %s in zip file\n", PAYLOADPATH);
		goto cleanup;
	}

	res = unzGetCurrentFileInfo(zipfile, &payloadInfo, nullptr, 0, nullptr, 0, nullptr, 0);
	if (res != UNZ_OK) {
		std::printf("ERR Could not read metadata for %s\n", PAYLOADPATH);
		goto cleanup;
	}
	*payloadSize = payloadInfo.uncompressed_size;

	res = unzOpenCurrentFile(zipfile);
	if (res != UNZ_OK) {
		std::printf("ERR Could not open %s for reading\n", PAYLOADPATH);
		goto cleanup;
	}

	*payloadData = (u8*)malloc(*payloadSize);
	res = unzReadCurrentFile(zipfile, *payloadData, *payloadSize);
	if (res < 0) {
		std::printf("ERR Could not read %s (%d)\n", PAYLOADPATH, res);
		goto cleanup;
	}
	if (res != (int)*payloadSize) {
		std::printf("ERR Extracted size does not match expected! (got %d expected %zu)", res, *payloadSize);
		goto cleanup;
	}

	// Close and cleanup
	success = true;
cleanup:
	unzCloseCurrentFile(zipfile);
	unzClose(zipfile);
	return success;
}

bool releaseGetPayload(const ReleaseVer& release, const bool isHourly, u8** payloadData, size_t* offset, size_t* payloadSize) {
	u8* fileData = nullptr;
	u32 fileSize = 0;
	HTTPResponseInfo info;

	try {
#ifdef FAKEDL
		// Read predownloaded file
		std::ifstream predownloaded(release.filename + ".7z", std::ios::binary | std::ios::ate);
		fileSize = predownloaded.tellg();
		predownloaded.seekg(0, std::ios::beg);
		fileData = (u8*)malloc(fileSize);
		predownloaded.read((char*)fileData, fileSize);
		info.etag = "\"0973d3d5fe62fccc30c8f663aec6918c\"";
#else
		httpGet(release.url.c_str(), &fileData, &fileSize, true, &info);
#endif
	} catch (std::string& e) {
		std::printf("%s\n", e.c_str());
		return false;
	}
	std::printf("Download complete! Size: %lu\n", fileSize);

	if (release.fileSize != 0) {
		std::printf("Integrity check #1");
		if (fileSize != release.fileSize) {
			std::printf(" [ERR]\r\nReceived file is a different size than expected!\r\n");
			gfxFlushBuffers();
			return false;
		}
		std::printf(" [OK]\r\n");
	} else {
		std::printf("Skipping integrity check #1 [No size]\r\n");
	}

	if (info.etag != "") {
		std::printf("Integrity check #2");
		if (!checkEtag(info.etag, fileData, fileSize)) {
			std::printf(" [ERR]\r\nMD5 mismatch between server's and local file!\r\n");
			gfxFlushBuffers();
			return false;
		}
		std::printf(" [OK]\r\n");
	} else {
		std::printf("Skipping integrity check #2 [No Etag]\r\n");
	}

	std::printf("\nDecompressing archive in memory...\n");
	gfxFlushBuffers();

	bool success;
	if (isHourly) {
		success = extractZip(fileData, fileSize, payloadData, payloadSize);
		offset = 0;
	} else {
		success = extract7z(fileData, fileSize, payloadData, offset, payloadSize);
	}
	if (!success) {
		return false;
	}

	std::free(fileData);
	return true;
}