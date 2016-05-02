#pragma once

#include <string>

/*! \brief Tries to detect currently installed Luma3DS/AuReiNand version
 *
 *  \param path Path to existing payload
 *
 *  \return Empty string if the version could not be determined, the version string otherwise
 */
std::string versionMemsearch(const std::string& path);