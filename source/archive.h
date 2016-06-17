#pragma once

#include "libs.h"

// 7z includes
#include "7z/7z.h"
#include "7z/7zAlloc.h"
#include "7z/7zCrc.h"
#include "7z/7zMemInStream.h"

// minizip includes
#include "minizip/ioapi_mem.h"
#include "minizip/unzip.h"

class ZipArchive {
private:
	ourmemory_t unzmem = {};
	zlib_filefunc_def filefunc32 = {};
	unzFile zipfile = nullptr;

public:
	ZipArchive(const u8* arcData, const u32 arcSize);
	~ZipArchive();

	void extractFile(std::string name, u8** fileData, size_t* fileSize);
};

class SzArchive {
private:
	CMemInStream memStream;
	CSzArEx db;
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	std::map<std::string, u32> files;
	void buildFileIndex();

public:
	SzArchive(const u8* arcData, const u32 arcSize);
	~SzArchive();

	void extractFile(std::string name, u8** fileData, size_t* fileSize, size_t* offset);
};