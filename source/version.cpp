#include "version.h"

#include <fstream>
#include <stdlib.h>

std::string versionMemsearch(const std::string path) {
	const char searchString[] = "AuReiNand v";
	const size_t searchStringLen = sizeof(searchString)/sizeof(char) - 1;

	std::ifstream payloadFile(path, std::ios::binary | std::ios::ate);
	if (!payloadFile) {
		printf("Could not open existing payload, does it exists?\n");
		return "";
	}

	/* Load entire file into local buffer */
	size_t payloadSize = payloadFile.tellg();
	payloadFile.seekg(0, std::ios::beg);
	char* payloadData = new char[payloadSize];
	payloadFile.read(payloadData, payloadSize);
	payloadFile.close();

	printf("Loaded existing payload in memory, searching for version number...\n");

	size_t curProposedOffset = 0;
	unsigned short curStringIndex = 0;
	bool found = false;
	std::string versionString = "";

	// Byte-by-byte search. (memcmp might be faster?)
	// Since "A" (1st char) is only used once in the whole string we can search in O(n)
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
		// Version is what comes between "configuration"
		size_t verOffset = curProposedOffset + searchStringLen;
		size_t verLength = 0;
		size_t remaining = verOffset - payloadSize;
		for (; verLength < remaining; verLength++) {
			char current = payloadData[verOffset + verLength];
			if (current == ' ' || current == '\0') {
				break;
			}
		}
		// Get full version string
		versionString = std::string(payloadData + verOffset, verLength);
	}

	free(payloadData);
	return versionString;
}