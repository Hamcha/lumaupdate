#pragma once

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

