#pragma once

#include "libs.h"

/*! \brief Tries to detect currently installed Luma3DS/AuReiNand version
 *
 *  \param path Path to existing payload
 *
 *  \return Empty string if the version could not be determined, the version string otherwise
 */
std::string versionMemsearch(const std::string& path);

/*! \brief Get stable part from a version
 *  Example: "5.4-abcdef0" will return "5.4"
 *
 *  \param version Full version string
 *
 *  \return String containing the stable version part only
*/
std::string versionGetStable(const std::string& version);

/*! \brief Get commit from a version
*  Example: "5.4-abcdef0" will return "abcdef0"
*
*  \param version Full version string
*
*  \return String containing the commit only
*/
std::string versionGetCommit(const std::string& version);