#include "version.h"

#include "utils.h"

const std::string LumaVersion::toString(bool printBranch) const {
	std::string currentVersionStr = release;
	if (!commit.empty()) {
		currentVersionStr += "-" + commit;
	}
	if (printBranch && isDev) {
		currentVersionStr += " (dev)";
	}
	return currentVersionStr;
}

/* Luma3DS 0x2e svc version struct */
struct PACKED SvcLumaVersion {
	char magic[4];
	uint8_t major;
	uint8_t minor;
	uint8_t build;
	uint8_t flags;
	uint32_t commit;
	uint32_t unused;
};


int NAKED svcGetLumaVersion(SvcLumaVersion UNUSED *info) {
	asm volatile(
		"svc 0x2E\n"
		"bx lr"
	);
}


LumaVersion versionSvc() {
	SvcLumaVersion info;
	if (svcGetLumaVersion(&info) != 0) {
		return LumaVersion{};
	}

	LumaVersion version;

	std::stringstream releaseBuilder;
	releaseBuilder << (int)info.major << "." << (int)info.minor;
	if (info.build > 0) {
		releaseBuilder << "." << (int)info.build;
	}
	version.release = releaseBuilder.str();

	if (info.commit > 0) {
		std::stringstream commitBuilder;
		commitBuilder << std::hex << info.commit;
		version.commit = commitBuilder.str();
	}

	version.isDev = (info.flags & 0x1) == 0x1;

	logPrintf("%s\n", version.toString().c_str());
	return version;
}

LumaVersion versionMemsearch(const std::string& path) {
	std::ifstream payloadFile(path, std::ios::binary);
	if (!payloadFile) {
		logPrintf("Could not open existing payload, does it exists?\n");
		return LumaVersion{ "", "", false };
	}

	logPrintf("Loaded existing payload in memory, searching for version number...\n");
	std::string str((std::istreambuf_iterator<char>(payloadFile)), std::istreambuf_iterator<char>()); 
	size_t pos = str.find("Luma3DS v"); 
	if (pos == std::string::npos) { 
		logPrintf("Could not find version number in payload.\n");
		return LumaVersion{ "", "", false };
	}
	
	size_t confPos = str.find("configuration", pos);
	if (confPos == std::string::npos) {
		logPrintf("Could not find version number in payload.\n");
		return LumaVersion{ "", "", false };
	}
	
	// Increment position to get past the 'Luma3DS v' string
	pos += 9;

	const std::string versionString = str.substr(pos, confPos - pos);
	logPrintf("Found version string: %s\n", versionString);

	LumaVersion version;
	version.release = versionString.substr(0, separator);

	const size_t end = versionString.find(" ", separator);
	if (end == std::string::npos) {
		version.commit = versionString.substr(separator + 1);
		version.isDev = false;
	} else {
		version.commit = versionString.substr(separator + 1, end - separator - 1);
		version.isDev = versionString.find("(dev)", separator) != std::string::npos;
	}

	return version;
}
