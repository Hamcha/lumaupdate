#include "pathchange.h"

bool pathchange(u8* buf, const size_t bufSize, const std::string& path) {
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
		} else {
			if (curStringIndex > 0) {
				curStringIndex = 0;
			}
		}
	}

	// Not found?
	if (!found) {
		std::printf("Could not find payload path, is this even a valid payload?\n");
		return false;
	}

	// Replace "arm9loaderhax.bin" with own payload path
	size_t offset = curProposedOffset + prefixSize;
	for (u8 i = 0; i < pathLength; ++i) {
		buf[offset + i*2] = path[i];
	};
	// Replace remaining characters from original path with 0s
	for (u8 i = pathLength; i < originalSize; ++i) {
		buf[offset + i*2] = 0;
	}

	return true;
}