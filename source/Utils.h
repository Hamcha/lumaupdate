#pragma once

#include "libs.h"

#ifdef __GNUC__
#define NAKED __attribute__((naked))
#define UNUSED __attribute__((unused))
#else
// Visual studio really hates GCC-specific syntax (I mean, it's right)
#undef PACKED
#define PACKED
#define NAKED
#define UNUSED
#endif

/* Luma3DS 0x2e svc version struct */
struct PACKED SvcLumaVersion {
	char magic[4];
	uint8_t major;
	uint8_t minor;
	uint8_t build;
	uint8_t flags;
	uint32_t commit;
	uint32_t config;
};

namespace Utils {

	/*! \brief Alternative to_string implementation (workaround for mingw)
	*
	*  \param n Input parameter
	*
	*  \return String representation of the input parameter
	*/
	template<typename T> std::string tostr(const T& n) {
		std::ostringstream stm;
		stm << n;
		return stm.str();
	}

	/*! \brief Get Luma3DS version using SVC 0x2F
	 *
	 *  \param info Pointer to struct to fill with version info
	 */
	int GetLumaVersion(SvcLumaVersion UNUSED * info);

	/*! \brief Format Luma3DS version struct to a readable format
	 *
	 *  \param versionInfo Struct to read version data from
	 *
	 *  \return Human-readable version as string
	 */
	std::string FormatLumaVersion(const SvcLumaVersion& versionInfo);
}