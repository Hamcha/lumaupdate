#include "update.h"

#include "arnutil.h"
#include "console.h"
#include "lumautils.h"
#include "utils.h"

static inline bool pathchange(u8* buf, const size_t bufSize, const std::string& path) {
	const static char original[] = "sdmc:/arm9loaderhax.bin";
	const static size_t prefixSize = 12; // S \0 D \0 M \0 C \0 : \0 / \0
	const static size_t originalSize = sizeof(original)/sizeof(char);
	u8 pathLength = path.length();

	if (pathLength > MAXPATHLEN) {
		std::printf("Cannot accept payload path: too long (max %d chars)\n", MAXPATHLEN);
		return false;
	}

	std::printf("Searching for \"%s\" in payload...\n", original);

	size_t curProposedOffset = 0;
	u8 curStringIndex = 0;
	bool found = false;

	// Byte-by-byte search. (memcmp might be faster?)
	// Since "s" (1st char) is only used once in the whole string we can search in O(n)
	for (size_t offset = 0; offset < bufSize-originalSize; ++offset) {
		if (buf[offset] == original[curStringIndex] && buf[offset+1] == 0) {
			if (curStringIndex == originalSize - 1) {
				found = true;
				break;
			}
			if (curStringIndex == 0) {
				curProposedOffset = offset;
			}
			curStringIndex++;
			offset++; // Skip one byte because Unicode
			continue;
		}

		if (curStringIndex > 0) {
			curStringIndex = 0;
		}
	}

	// Not found?
	if (!found) {
		std::printf("Could not find payload path, is this even a valid payload?\n");
		return false;
	}

	// Replace "arm9loaderhax.bin" with own payload path
	size_t offset = curProposedOffset + prefixSize;
	u8 i = 0;
	for (i = 0; i < pathLength; ++i) {
		buf[offset + i*2] = path[i];
	};
	// Replace remaining characters from original path with 0s
	for (i = pathLength; i < originalSize; ++i) {
		buf[offset + i*2] = 0;
	}

	return true;
}

static inline bool backupA9LH(const std::string& payloadName) {
	std::ifstream original(payloadName, std::ifstream::binary);
	if (!original.good()) {
		std::printf("Could not open %s\n", payloadName.c_str());
		return false;
	}

	std::string backupName = payloadName + ".bak";
	std::ofstream target(backupName, std::ofstream::binary);
	if (!target.good()) {
		std::printf("Could not open %s\n", backupName.c_str());
		original.close();
		return false;
	}

	target << original.rdbuf();

	original.close();
	target.close();
	return true;
}

UpdateResult update(const UpdateArgs& args) {
	consoleScreen(GFX_TOP);
	consoleInitProgress("Updating Luma3DS", "Performing preliminary operations", 0);

	consoleScreen(GFX_BOTTOM);
	consoleClear();

	// Back up local file if it exists
	if (!args.backupExisting) {
		std::printf("Payload backup is disabled in config, skipping...\n");
	} else if (!fileExists(args.payloadPath)) {
		std::printf("Original payload not found, skipping backup...\n");
	} else {
		consoleScreen(GFX_TOP);
		consoleSetProgressData("Backing up old payload", 0.1);
		consoleScreen(GFX_BOTTOM);

		std::printf("Copying %s to %s.bak...\n", args.payloadPath.c_str(), args.payloadPath.c_str());
		gfxFlushBuffers();
		if (!backupA9LH(args.payloadPath)) {
			std::printf("\nCould not backup %s (!!), aborting...\n", args.payloadPath.c_str());
			return { false, "BACKUP FAILED" };
		}
	}

	consoleScreen(GFX_TOP);
	consoleSetProgressData("Downloading payload", 0.3);
	consoleScreen(GFX_BOTTOM);

	std::printf("Downloading %s\n", args.chosenVersion.url.c_str());
	gfxFlushBuffers();

	u8* payloadData = nullptr;
	size_t offset = 0;
	size_t payloadSize = 0;
	if (!releaseGetPayload(args.chosenVersion, args.isHourly, &payloadData, &offset, &payloadSize)) {
		std::printf("FATAL\nCould not get A9LH payload...\n");
		std::free(payloadData);
		return { false, "DOWNLOAD FAILED" };
	}

	if (args.payloadPath != std::string("/") + PAYLOADPATH) {
		consoleScreen(GFX_TOP);
		consoleSetProgressData("Applying path changing", 0.6);
		consoleScreen(GFX_BOTTOM);

		std::printf("Requested payload path is not %s, applying path patch...\n", PAYLOADPATH);
		if (!pathchange(payloadData + offset, payloadSize, args.payloadPath)) {
			std::free(payloadData);
			return { false, "PATHCHANGE FAILED" };
		}
	}

	if (args.migrateARN) {
		consoleScreen(GFX_TOP);
		consoleSetProgressData("Migrating AuReiNand -> Luma3DS", 0.8);
		consoleScreen(GFX_BOTTOM);

		std::printf("Migrating AuReiNand install to Luma3DS...\n");
		if (!arnMigrate()) {
			std::printf("FATAL\nCould not migrate AuReiNand install (?)\n");
			return { false, "MIGRATION FAILED" };
		}
	}

	if (!lumaMigratePayloads()) {
		std::printf("WARN\nCould not migrate payloads\n\n");
	}

	consoleScreen(GFX_TOP);
	consoleSetProgressData("Saving payload to SD", 0.9);
	consoleScreen(GFX_BOTTOM);

	std::printf("Saving %s to SD (as %s)...\n", PAYLOADPATH, args.payloadPath.c_str());
	std::ofstream a9lhfile("/" + args.payloadPath, std::ofstream::binary);
	a9lhfile.write((const char*)(payloadData + offset), payloadSize);
	a9lhfile.close();

	std::printf("All done, freeing resources and exiting...\n");
	std::free(payloadData);

	consoleClear();
	consoleScreen(GFX_TOP);

	return { true };
}

UpdateResult restore(const UpdateArgs& args) {
	// Rename current payload to .broken
	if (std::rename(args.payloadPath.c_str(), (args.payloadPath + ".broken").c_str()) != 0) {
		std::perror("Can't rename current version");
		return { false, "RENAME1 FAILED" };
	}
	// Rename .bak to current
	if (std::rename((args.payloadPath + ".bak").c_str(), args.payloadPath.c_str()) != 0) {
		std::perror("Can't rename backup to current payload name");
		return { false, "RENAME2 FAILED" };
	}
	// Remove .broken
	if (std::remove((args.payloadPath + ".broken").c_str()) != 0) {
		std::perror("WARN: Could not remove current payload, please remove it manually");
	}
	return { true };
}
