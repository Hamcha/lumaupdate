#include "lumautils.h"

#include "utils.h"

static const std::string PayloadPath = "/luma/payloads/";

std::vector<std::string> listPayloads() {
	std::vector<std::string> files = {};
	FS_Archive sdmcArchive;

	// Open SD card
	if (FSUSER_OpenArchive(&sdmcArchive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, NULL)) != 0) {
		logPrintf("\nCould not access SD Card (?)\n\n");
		return files;
	}

	// Open source directory
	Handle directory = 0;
	if (FSUSER_OpenDirectory(&directory, sdmcArchive, fsMakePath(PATH_ASCII, "/luma/payloads")) != 0) {
		logPrintf("\nCould not open /luma/payloads\n\n");
		FSUSER_CloseArchive(sdmcArchive);
		return files;
	}

	u32 fileRead = 0;
	while (true) {
		FS_DirectoryEntry entry = {};
		FSDIR_Read(directory, &fileRead, 1, &entry);
		if (!fileRead) {
			break;
		}

		// Convert name to ASCII (just cut the other bytes) and compare with prefix
		char name8[262] = { 0 };
		for (size_t i = 0; i < 262; i++) {
			name8[i] = entry.name[i] % 0xff;
		}

		files.push_back(std::string(name8));
	}

	FSUSER_CloseArchive(sdmcArchive);
	return files;
}

bool hasPrefix(const std::string& name, const std::string& prefix) {
	if (name.length() < prefix.length()) {
		return false;
	}

	return name.substr(0, prefix.length()) == prefix;
}

bool findAndRename(const char* oldName, const char* newName) {
	const std::string oldPath = PayloadPath + oldName;
	const std::string newPath = PayloadPath + newName;

	if (fileExists(oldPath)) {
		return std::rename(oldPath.c_str(), newPath.c_str()) == 0;
	}
	return true;
}

bool findAndRenamePrefix(const std::vector<std::string>& files, const std::string& oldPrefix, const std::string& newPrefix) {
	const size_t oldPrefixLen = oldPrefix.length();
	for (std::string file : files) {
		logPrintf("Considering %s\n", file.c_str());
		if (hasPrefix(file, oldPrefix)) {
			std::string newName = PayloadPath + newPrefix + file.substr(oldPrefixLen);
			file = PayloadPath + file;
			logPrintf("%s -> %s", file.c_str(), newName.c_str());
			if (std::rename(file.c_str(), newName.c_str()) != 0) {
				std::perror((std::string("Could not rename ") + newName).c_str());
			}
		}
	}
	return true;
}

bool lumaMigratePayloads() {
	// Default is now def (5.1)
	if (!findAndRename("default.bin", "def.bin")) {
		std::perror("\nCould not rename default.bin");
		return false;
	}

	const std::vector<std::string> files = listPayloads();

	// Default is now Start (5.4)
	if (!findAndRenamePrefix(files, "def", "start")) {
		std::perror("\nCould not rename def_ to start_");
		return false;
	}

	// Sel is now Select (5.4)
	// Hacky thing: Match sel/sel_, but not select!
	if (!findAndRename("sel.bin", "select.bin")) {
		std::perror("\nCould not rename sel.bin");
		return false;
	}
	if (!findAndRenamePrefix(files, "sel_", "select_")) {
		std::perror("\nCould not rename sel_ to select_");
		return false;
	}

	return true;
}