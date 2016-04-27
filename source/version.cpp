#include "version.h"

#include <fstream>
#include <cstdlib>

std::string versionMemsearch(const std::string path) {
	const static char searchString[] = " configuration";
	const static size_t searchStringLen = sizeof(searchString)/sizeof(char) - 1;

	std::ifstream payloadFile(path, std::ios::binary | std::ios::ate);
	if (!payloadFile) {
		std::printf("Could not open existing payload, does it exists?\n");
		return "";
	}

	/* Load entire file into local buffer */
	size_t payloadSize = payloadFile.tellg();
	payloadFile.seekg(0, std::ios::beg);
	char* payloadData = (char*)std::malloc(payloadSize);
	payloadFile.read(payloadData, payloadSize);
	payloadFile.close();

	std::printf("Loaded existing payload in memory, searching for version number...\n");

	size_t curProposedOffset = 0;
	unsigned short curStringIndex = 0;
	bool found = false;
	std::string versionString = "";

	// Byte-by-byte search. (memcmp might be faster?)
	// Since " " (1st char) is only used once in the whole string we can search in O(n)
	for (size_t offset = 0; offset < payloadSize - searchStringLen; offset++) {
		if (payloadData[offset] == searchString[curStringIndex]) {
			if (curStringIndex == searchStringLen - 1) {
				found = true;
				break;
			}
			if (curStringIndex == 0) {
				curProposedOffset = offset;
			}
			curStringIndex++;
		}
		else {
			if (curStringIndex > 0) {
				curStringIndex = 0;
			}
		}
	}

	if (found) {
		// Version is what comes before "configuration" but after "v"
		size_t verOffset = curProposedOffset;
		for (; verOffset > 0; verOffset--) {
			char current = payloadData[verOffset];
			if (current == 'v') {
				break;
			}
		}
		// Get full version string
		versionString = std::string(payloadData+verOffset+1, curProposedOffset-verOffset-1);
	}

	std::free(payloadData);
	return versionString;
}