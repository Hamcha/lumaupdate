#include "arnutil.h"

#include <3ds.h>

#include <cstdio>
#include <cstdlib>

bool renameRecursive(const FS_Archive& archive, const std::string& source, const std::string& target);

bool arnVersionCheck(const std::string& versionString) {
	// Bound checking before trying to do naughty things
	const size_t verLength = versionString.length();
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
	const static FS_Path aurei = fsMakePath(PATH_ASCII, "/aurei");
	const static FS_Path luma = fsMakePath(PATH_ASCII, "/luma");
	FS_Archive sdmcArchive = { 0x00000009,{ PATH_EMPTY, 1, (u8*) "" } };

	if (FSUSER_OpenArchive(&sdmcArchive) != 0) {
		std::printf("\nCould not access SD Card (?)\n\n");
		return false;
	}

	// Check if /luma already existsHandle directory = { 0 };
	if (FSUSER_OpenDirectory(NULL, sdmcArchive, luma) == 0) {
		std::printf("Luma directory already exists, skipping migration..\n");
		return true;
	}

	if (!renameRecursive(sdmcArchive, "/aurei", "/luma")) {
		FSUSER_CloseArchive(&sdmcArchive);
		return false;
	}

	// Delete the source directory and check if it succeeds
	Result res = FSUSER_DeleteDirectoryRecursively(sdmcArchive, aurei);
	if (res != 0) {
		std::printf("\nWARN: Could not delete original /aurei (%ld)!\n\n",  res);
	}
	
	FSUSER_CloseArchive(&sdmcArchive);
	return true;
}

bool renameRecursive(const FS_Archive& archive, const std::string& source, const std::string& target) {
	const FS_Path sourcePath = fsMakePath(PATH_ASCII, source.c_str());
	const FS_Path targetPath = fsMakePath(PATH_ASCII, target.c_str());

	// Open source directory
	Handle directory = NULL;
	if (FSUSER_OpenDirectory(&directory, archive, sourcePath) != 0) {
		std::printf("\nCould not open %s\n\n", source.c_str());
		return false;
	}

	// Make target directory
	if (FSUSER_CreateDirectory(archive, targetPath, FS_ATTRIBUTE_DIRECTORY) != 0) {
		std::printf("\nCould not create %s\n\n", target.c_str());
		return false;
	}

	u32 fileRead = 0;
	while (true) {
		FS_DirectoryEntry entry = {};
		FSDIR_Read(directory, &fileRead, 1, &entry);
		if (!fileRead) {
			break;
		}

		// Convert name to ASCII (just cut the other bytes)
		char name8[262] = { 0 };
		for (size_t i = 0; i < 262; i++) {
			name8[i] = entry.name[i] % 0xff;
		}
		std::string filePath = std::string("/") + name8;
		std::string from = source + filePath;
		std::string to = target + filePath;

		// Is a directory? Recurse rename
		if (entry.attributes & FS_ATTRIBUTE_DIRECTORY) {
			std::printf("  %s -> %s (DIR)\n", from.c_str(), to.c_str());
			if (!renameRecursive(archive, source + filePath, target + filePath)) {
				return false;
			}
		} else {
			FS_Path sourceFilePath = fsMakePath(PATH_ASCII, from.c_str());
			FS_Path targetFilePath = fsMakePath(PATH_ASCII, to.c_str());
			if (FSUSER_RenameFile(archive, sourceFilePath, archive, targetFilePath) != 0) {
				std::printf("\nCould not rename %s\n\n", (char*)sourceFilePath.data);
				return false;
			}
			std::printf("  %s -> %s\n", (char*)sourceFilePath.data, (char*)targetFilePath.data);
		}
	}

	return true;
}