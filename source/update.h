#pragma once

#include "libs.h"

#include "release.h"

#define MAXPATHLEN 37

struct UpdateArgs {
	PayloadType  payloadType;    /*!< Type of payload to upgrade  */
	std::string  payloadPath;    /*!< Path to Luma3DS payload     */
	bool         backupExisting; /*!< Backup existing payload     */
	bool         migrateARN;     /*!< Migrate from AuReiNand      */
	ReleaseVer   chosenVersion;  /*!< Version to update to        */
	bool         isHourly;       /*!< Is chosen version a hourly? */
};

struct UpdateResult {
	bool        success; /*!< Wether the operation was a success */
	std::string errcode; /*!< Error code if success is false     */
};

UpdateResult update(const UpdateArgs& args);
UpdateResult restore(const UpdateArgs& args);