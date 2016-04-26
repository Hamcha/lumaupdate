#include "arnutil.h"

#include <3ds.h>

#include <stdio.h>
#include <stdlib.h>

bool arnVersionCheck(std::string versionString) {
	// Bound checking before trying to do naughty things
	size_t verLength = versionString.length();
	if (verLength < 1) {
		printf("Weird version string: it's empty (?)\n");
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
	int release = atoi(releaseStr.c_str());

	// Check major version if found and release is 5.x
	if (release == 5 && hasMajor) {
		// Parse major version number
		std::string majorStr = versionString;
		size_t minSeparator = versionString.find('.', relSeparator+1);

		// "." found, get only release part and set major version flag
		if (minSeparator != std::string::npos) {
			majorStr = versionString.substr(relSeparator, minSeparator);
		}

		int minor = atoi(majorStr.c_str());
		
		// Check if major is lower than 2
		return minor < 2;
	} else {
		// If version if not 5.x (or is 5.0) just check if it's <= 5
		return release <= 5;
	}
}

bool arnMigrate() {
	const static FS_Path arnDir = fsMakePath(PATH_ASCII, "/aurei");
	const static FS_Path lumaDir = fsMakePath(PATH_ASCII, "/luma");
	
	FS_Archive sdmcArchive = { 0x00000009,{ PATH_EMPTY, 1, (u8*) "" } };

	if (FSUSER_OpenArchive(&sdmcArchive) != 0) {
		printf("\nCould not access SD Card (?)\n\n");
		return false;
	}

	// If there is already a luma skip migration
	if (FSUSER_OpenDirectory(NULL, sdmcArchive, lumaDir) == 0) {
		printf("/luma directory found, no migration performed.\n\n");
		return true;
	}

	// If there is not /aurei AND no /luma we can't migrate!
	if (FSUSER_OpenDirectory(NULL, sdmcArchive, arnDir) != 0) {
		printf("\n/aurei folder not found, migration aborted.\n\n");
		return false;
	}

	// Try to rename the directory and check if it succeeds
	return FSUSER_RenameDirectory(sdmcArchive, arnDir, sdmcArchive, lumaDir) == 0;
}