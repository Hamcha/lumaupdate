#pragma once

#include "libs.h"

#include "update.h"

//! Homebrew type (format)
enum class HomebrewType {
	Unknown,  //!< Can't detect what we are (?)
	Homebrew, //!< Running as .3dsx (hblauncher)
	CIA       //!< Running as .cia (Homemenu)
};

//! Homebrew location
enum class HomebrewLocation {
	Unknown, //!< Can't detect where we are running from
	SDMC,    //!< Loaded off the SD card (via sdmc)
	Remote   //!< Loaded via 3dslink
};

//! Luma3DS Updater install info
struct UpdaterInfo {
	HomebrewType     type;     //!< Homebrew type (3dsx, cia, etc)
	HomebrewLocation location; //!< Homebrew location (SDMC, 3DSLink, NAND etc)
	std::string      sdmcLoc;  //!< Folder on SDMC (if homebrew)
	std::string      sdmcName; //!< Name of the 3dsx file (if homebrew)
};

//! Latest Luma3DS Updater version info
struct LatestUpdaterInfo {
	std::string version;       //!< Version number
	std::string url;           //!< Download URL
	std::string changelog;     //!< Changelog
	bool        isNewer;       //!< Is version newer than the currently installed one
	size_t      fileSize;      //!< Archive size
};

/*! \brief Get currently installed homebrew info
 *
 *  \param path Path to currently executing homebrew, usually argv[0]
 *
 *  \return Detected install parameters (if any)
 */
UpdaterInfo updaterGetInfo(const char* path = nullptr);

/*! \brief Get latest available release of Luma3DS Updater
 *
 *  \return Latest release available on Github
 */
LatestUpdaterInfo updaterGetLatest();

/*! \brief Update to latest version 
 *
 *  \param latest Latest release info (for downloading)
 *  \param current Current updater install info (for installing)
 */
UpdateResult updaterDoUpdate(LatestUpdaterInfo latest, UpdaterInfo current);