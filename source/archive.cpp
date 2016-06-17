#include "archive.h"
#include "utils.h"

ZipArchive::ZipArchive(const u8* arcData, const u32 arcSize) {
	unzmem.size = arcSize;
	unzmem.base = (char*)malloc(unzmem.size);
	std::memcpy(unzmem.base, arcData, unzmem.size);
	fill_memory_filefunc(&filefunc32, &unzmem);
	zipfile = unzOpen2("__notused__", &filefunc32);
}

ZipArchive::~ZipArchive() {
	unzCloseCurrentFile(zipfile);
	unzClose(zipfile);
}

void ZipArchive::extractFile(std::string name, u8** fileData, size_t* fileSize) {
	int res = unzLocateFile(zipfile, name.c_str(), nullptr);
	if (res == UNZ_END_OF_LIST_OF_FILE) {
		throw std::runtime_error("Could not find " + name + " in zip file");
	}

	unz_file_info payloadInfo = {};
	res = unzGetCurrentFileInfo(zipfile, &payloadInfo, nullptr, 0, nullptr, 0, nullptr, 0);
	if (res != UNZ_OK) {
		throw std::runtime_error("Could not read metadata for " + name);
	}
	*fileSize = payloadInfo.uncompressed_size;

	res = unzOpenCurrentFile(zipfile);
	if (res != UNZ_OK) {
		throw std::runtime_error("Could not open " + name + " for reading");
	}

	*fileData = (u8*)malloc(*fileSize);
	res = unzReadCurrentFile(zipfile, *fileData, *fileSize);
	if (res < 0) {
		throw std::runtime_error("Could not read " + name + " (" + tostr(res) + ")");
	}

	if (res != (int)*fileSize) {
		throw std::runtime_error("Extracted size does not match expected! (got " + tostr(res) + " expected " + tostr(*fileSize) + ")");
	}
}

SzArchive::SzArchive(const u8* arcData, const u32 arcSize) {
	MemInStream_Init(&memStream, arcData, arcSize);

	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;
	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;

	CrcGenerateTable();
	SzArEx_Init(&db);

	SRes res = SzArEx_Open(&db, &memStream.s, &allocImp, &allocTempImp);
	if (res != SZ_OK) {
		throw std::runtime_error("Could not open archive (SzArEx_Open)\n");
	}

	buildFileIndex();
}

void SzArchive::buildFileIndex() {
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

		std::string nameStr(name8);
		files[nameStr] = i;
	}
}

SzArchive::~SzArchive() {
	SzArEx_Free(&db, &allocImp);
}

void SzArchive::extractFile(std::string name, u8** fileData, size_t* fileSize, size_t* offset) {
	auto it = files.find(name);
	if (it == files.end()) {
		throw std::runtime_error("Could not find " + name);
	}

	UInt32 blockIndex = UINT32_MAX;
	size_t fileBufSize = 0;

	SRes res = SzArEx_Extract(
		&db,
		&memStream.s,
		it->second,
		&blockIndex,
		fileData,
		&fileBufSize,
		offset,
		fileSize,
		&allocImp,
		&allocTempImp
	);
	if (res != SZ_OK) {
		throw std::runtime_error("Could not extract " + name);
	}
}