#pragma once

#include "libs.h"

//! Homebrew type (format)
enum HomebrewType {
	HbTypeUnknown, //!< Can't detect what we are (?)
	HbType3DSX,    //!< Running as .3dsx (hblauncher)
	HbTypeCIA      //!< Running as .cia (Homemenu)
};

//! Homebrew location
enum HomebrewLocation {
	HbLocUnknown, //!< Can't detect where we are running from
	HbLocSDMC,    //!< Loaded off the SD card (via sdmc)
	HbLoc3DSLink  //!< Loaded via 3dslink
};

//! Luma3DS Updater install info
struct UpdaterInfo {
	HomebrewType     type;     //!< Homebrew type (3dsx, cia, etc)
	HomebrewLocation location; //!< Homebrew location (SDMC, 3DSLink, NAND etc)
};

//! Latest Luma3DS Updater version info
struct LatestUpdaterInfo {
	std::string version;
	std::string url;
	std::string changelog;
};

/*! \brief Get currently installed homebrew info
 *
 *  \param source Path to currently executing homebrew, usually argv[0]
 *
 *  \return Detected install parameters (if any)
 */
UpdaterInfo updaterGetInfo(const std::string& source);

/*! \brief Get latest available release of Luma3DS Updater
 *
 *  \return Latest release available on Github
 */
LatestUpdaterInfo updaterGetLatest();