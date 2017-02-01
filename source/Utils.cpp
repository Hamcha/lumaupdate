#include "Utils.h"

int NAKED Utils::GetLumaVersion(SvcLumaVersion UNUSED *info) {
	asm volatile(
		"svc 0x2E\n"
		"bx lr"
	);
}

std::string Utils::FormatLumaVersion(const SvcLumaVersion& versionInfo) {
	std::stringstream versionStr;
	versionStr << (int)versionInfo.major << "." << (int)versionInfo.minor;
	if (versionInfo.build > 0) {
		versionStr << "." << (int)versionInfo.build;
	}

	// Is DEV? (Only applies < 6.3
	if (versionInfo.major < 7 && versionInfo.minor < 3 && versionInfo.flags & 0x1 == 0x1) {
		versionStr << "-dev";
	}

	if (versionInfo.commit > 0) {
		std::stringstream commitBuilder;
		versionStr << " (" << std::hex << versionInfo.commit << ")";
	}

	return versionStr.str();
}