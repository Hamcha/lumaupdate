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

// Internal includes
#include "http.h"
#include "utils.h"

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
		std::strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

ReleaseInfo releaseGetLatestStable() {
	static const char* ReleaseURL = "https://api.github.com/repos/AuroraWright/Luma3DS/releases/latest";
	ReleaseInfo release;

#ifdef FAKEDL
	// Citra doesn't support HTTPc right now, so just fake a successful request
	release.name = "5.2";
	release.versions.push_back(ReleaseVer{ "CITRA", "CITRA", "https://github.com/AuroraWright/Luma3DS/releases/download/v5.2/Luma3DSv5.2.7z" });
#else

	jsmn_parser p = {};
	jsmn_init(&p);

	u8* apiReqData = nullptr;
	u32 apiReqSize = 0;

	std::printf("Downloading %s...\n", ReleaseURL);

	httpGet(ReleaseURL, &apiReqData, &apiReqSize);

	std::printf("Downloaded %lu bytes\n", apiReqSize);
	gfxFlushBuffers();

	jsmntok_t t[512] = {};
	int r = jsmn_parse(&p, (const char*)apiReqData, apiReqSize, t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		throw formatErrMessage("Failed to parse JSON", r);
	}
	std::printf("JSON parsed successfully!\n");
	gfxFlushBuffers();

	bool namefound = false, inassets = false, verHasName = false, verHasURL = false, isDev = false;
	ReleaseVer current;
	for (int i = 0; i < r; i++) {
		if (!namefound && jsoneq((const char*)apiReqData, &t[i], "name") == 0) {
			jsmntok_t val = t[i + 1];
			// Strip the "v" in front of the version name
			if (apiReqData[val.start] == 'v') {
				val.start += 1;
			}
			release.name = std::string((const char*)apiReqData + val.start, val.end - val.start);
			std::printf("Release found: %s\n", release.name.c_str());
			namefound = true;
		}
		if (!inassets && jsoneq((const char*)apiReqData, &t[i], "assets") == 0) {
			inassets = true;
		}
		if (inassets) {
			if (jsoneq((const char*)apiReqData, &t[i], "name") == 0) {
				jsmntok_t val = t[i + 1];
				current.filename = std::string((const char*)apiReqData + val.start, val.end - val.start);
				isDev = current.filename.find("-dev.") < std::string::npos;
				current.friendlyName = isDev ? "developer version" : "normal version";
				verHasName = true;
			}
			if (jsoneq((const char*)apiReqData, &t[i], "browser_download_url") == 0) {
				jsmntok_t val = t[i + 1];
				current.url = std::string((const char*)apiReqData + val.start, val.end - val.start);
				verHasURL = true;
			}
			if (verHasName && verHasURL) {
				printf("Found version: %s\n", current.filename.c_str());
				ReleaseVer version = ReleaseVer{ current.filename, current.friendlyName, current.url };
				// Put normal version in front, dev on back
				if (!isDev) {
					release.versions.insert(release.versions.begin(), version);
				} else {
					release.versions.push_back(version);
				}
				verHasName = verHasURL = isDev = false;
			}
		}
	}

	gfxFlushBuffers();
	std::free(apiReqData);

#endif

	return release;
}

ReleaseInfo releaseGetLatestHourly() {
	static const char* LastCommitURL = "https://raw.githubusercontent.com/astronautlevel2/Luma3DS/gh-pages/lastCommit";
	ReleaseInfo hourly;

#ifdef FAKEDL
	// Citra doesn't support HTTPc right now, so just fake a successful request
	hourly.name = "aaaaaaa";
	hourly.versions.push_back(ReleaseVer{ "CITRA", "latest hourly (aaaaaaa)", "https://github.com/AuroraWright/Luma3DS/releases/download/v5.2/Luma3DSv5.2.7z" });
#else

	u8* apiReqData = nullptr;
	u32 apiReqSize = 0;

	std::printf("Downloading %s...\n", LastCommitURL);

	httpGet(LastCommitURL, &apiReqData, &apiReqSize);

	std::printf("Downloaded %lu bytes\n", apiReqSize);
	gfxFlushBuffers();

	hourly.name = std::string((const char*)apiReqData, apiReqSize);
	trim(hourly.name);
	std::string url = std::string("https://astronautlevel2.github.io/Luma3DS/builds/Luma-") + hourly.name + ".zip";

	hourly.versions.push_back(ReleaseVer{ hourly.name, "latest hourly (" + hourly.name + ")", std::string(url) });

	std::free(apiReqData);

#endif

	return hourly;
}

bool extract7z(u8* fileData, size_t fileSize, u8** payloadData, size_t* offset, size_t* payloadSize) {
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
	if (res == SZ_OK) {
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
	}
	else {
		std::printf("Could not open archive (SzArEx_Open)\n");
		SzArEx_Free(&db, &allocImp);
		std::free(fileData);
		return false;
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

bool extractZip(u8* fileData, size_t fileSize, u8** payloadData, size_t* payloadSize) {
	zlib_filefunc_def filefunc32 = {};
	ourmemory_t unzmem = {};
	unz_file_info payloadInfo = {};
	bool success = false;

	unzmem.size = fileSize;
	unzmem.base = (char*)malloc(unzmem.size);
	memcpy(unzmem.base, fileData, unzmem.size);

	fill_memory_filefunc(&filefunc32, &unzmem);

	unzFile zipfile = unzOpen2("__notused__", &filefunc32);
	
	int res = unzLocateFile(zipfile, (std::string("out/") + PAYLOADPATH).c_str(), nullptr);
	if (res == UNZ_END_OF_LIST_OF_FILE) {
		printf("ERR Could not find %s in zip file\n", PAYLOADPATH);
		goto cleanup;
	}

	res = unzGetCurrentFileInfo(zipfile, &payloadInfo, nullptr, 0, nullptr, 0, nullptr, 0);
	if (res != UNZ_OK) {
		printf("ERR Could not read metadata for %s\n", PAYLOADPATH);
		goto cleanup;
	}
	*payloadSize = payloadInfo.uncompressed_size;

	res = unzOpenCurrentFile(zipfile);
	if (res != UNZ_OK) {
		printf("ERR Could not open %s for reading\n", PAYLOADPATH);
		goto cleanup;
	}

	*payloadData = (u8*)malloc(*payloadSize);
	res = unzReadCurrentFile(zipfile, *payloadData, *payloadSize);
	if (res < 0) {
		printf("ERR Could not read %s (%d)\n", PAYLOADPATH, res);
		goto cleanup;
	}
	if (res != (int)*payloadSize) {
		printf("ERR Extracted size does not match expected! (got %d expected %zu)", res, *payloadSize);
		goto cleanup;
	}

	// Close and cleanup
	success = true;
cleanup:
	unzCloseCurrentFile(zipfile);
	unzClose(zipfile);
	return success;
}

bool releaseGetPayload(ReleaseVer release, bool isHourly, u8** payloadData, size_t* offset, size_t* payloadSize) {
	u8* fileData = nullptr;
	u32 fileSize = 0;

	try {
#ifdef FAKEDL
		// Read predownloaded file
		std::ifstream predownloaded(release.filename + ".7z", std::ios::binary | std::ios::ate);
		fileSize = predownloaded.tellg();
		predownloaded.seekg(0, std::ios::beg);
		fileData = (u8*)malloc(fileSize);
		predownloaded.read((char*)fileData, fileSize);
#else
		httpGet(release.url.c_str(), &fileData, &fileSize);
#endif
	} catch (std::string& e) {
		std::printf("%s\n", e.c_str());
		return false;
	}
	std::printf("Download complete! Size: %lu\n", fileSize);
	std::printf("\nDecompressing archive in memory...\n");
	gfxFlushBuffers();

	if (isHourly) {
		bool ret = extractZip(fileData, fileSize, payloadData, payloadSize);
		offset = 0;
		if (!ret) {
			return false;
		}
	} else {
		bool ret = extract7z(fileData, fileSize, payloadData, offset, payloadSize);
		if (!ret) {
			return false;
		}
	}

	std::free(fileData);

	if (*payloadSize > 0x20000) {
		std::printf("File is too big to be a valid A9LH payload!\n");
		gfxFlushBuffers();
		return false;
	}

	return true;
}