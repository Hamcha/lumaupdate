#include "arnutil.h"

#include <3ds.h>

#include <cstdio>
#include <cstdlib>

bool renameRecursive(FS_Archive archive, std::wstring source, std::wstring target);

bool arnVersionCheck(std::string versionString) {
	// Bound checking before trying to do naughty things
	size_t verLength = versionString.length();
	if (verLength < 1) {
		std::printf("Weird version string: it's empty (?)\n");
		return false;
	}

	// Parse release number
	std::string releaseStr = versionString;
	bool hasMajor = false;
	size_t relSeparator = versionString.find('.');

	// "." found, get only release part and set major version flag
	if (relSeparator != std::string::npos) {
		releaseStr = versionString.substr(0, relSeparator);
		hasMajor = true;
	}
	int release = std::atoi(releaseStr.c_str());

	// Check major version if found and release is 5.x
	if (release == 5 && hasMajor) {
		// Parse major version number
		std::string majorStr = versionString.substr(relSeparator+1);
		size_t minSeparator = majorStr.find('.');

		// "." found, get only release part and set major version flag
		if (minSeparator != std::string::npos) {
			majorStr = majorStr.substr(0, minSeparator);
		}

		int major = std::atoi(majorStr.c_str());

		// Check if major is lower than 2
		return major < 2;
	} else {
		// If version if not 5.x (or is 5.0) just check if it's <= 5
		return release <= 5;
	}
}

bool arnMigrate() {
	const static FS_Path arnDir = fsMakePath(PATH_ASCII, "/aurei");
	
	FS_Archive sdmcArchive = { 0x00000009,{ PATH_EMPTY, 1, (u8*) "" } };

	if (FSUSER_OpenArchive(&sdmcArchive) != 0) {
		std::printf("\nCould not access SD Card (?)\n\n");
		return false;
	}

	renameRecursive(sdmcArchive, L"/aurei", L"/luma");
	
	// Try to rename the directory and check if it succeeds
	Result res = FSUSER_DeleteDirectory(sdmcArchive, arnDir);
	if (res != 0) {
		std::printf("\nCould not delete /aurei directory!\n\n");
		FSUSER_CloseArchive(&sdmcArchive);
		return false;
	}

	FSUSER_CloseArchive(&sdmcArchive);
	return true;
}

bool renameRecursive(FS_Archive archive, std::wstring source, std::wstring target) {
	const FS_Path sourcePath = fsMakePath(PATH_UTF16, source.c_str());
	const FS_Path targetPath = fsMakePath(PATH_UTF16, target.c_str());

	// Open source directory
	Handle directory = { 0 };
	if (FSUSER_OpenDirectory(&directory, archive, sourcePath) != 0) {
		std::wprintf(L"\nCould not open %ls\n\n", source.c_str());
		return false;
	}

	// Make target directory
	if (FSUSER_CreateDirectory(archive, targetPath, FS_ATTRIBUTE_DIRECTORY) != 0) {
		std::wprintf(L"\nCould not create %ls\n\n", target.c_str());
		return false;
	}

	u32 fileRead = 0;
	while (true) {
		FS_DirectoryEntry entry = { 0 };
		FSDIR_Read(directory, &fileRead, 1, &entry);
		if (!fileRead) {
			break;
		}

		std::wstring filePath = std::wstring(L"/") + (wchar_t*)entry.name;

		// Is a directory? Recurse rename
		if (entry.attributes & FS_ATTRIBUTE_DIRECTORY) {
			renameRecursive(archive, source + filePath, target + filePath);
		} else {
			FS_Path sourceFilePath = fsMakePath(PATH_UTF16, (source + filePath).c_str());
			FS_Path targetFilePath = fsMakePath(PATH_UTF16, (target + filePath).c_str());
			if (FSUSER_RenameFile(archive, sourceFilePath, archive, targetFilePath) != 0) {
				std::wprintf(L"\nCould not rename %ls\n\n", sourceFilePath);
				return false;
			}
			std::wprintf(L"  %ls -> %ls\n", sourceFilePath, targetFilePath);
		}
	}

	return true;
}