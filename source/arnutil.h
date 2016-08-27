#pragma once

#include "libs.h"
#include "version.h"

/*! \brief Check if the specified version is before the name change
 *  arnVersionCheck checks wether the version is lower than 5.2
 *  (ie. before the name change from AuReiNand to Luma3DS)
 *
 *  \param versionString Version string found in payload
 *
 *  \return true if the payload is AuReiNand (<5.2), false otherwsie
 */
bool arnVersionCheck(const LumaVersion& versionString);

/*! \brief Migrate AuReiNand install to Luma3DS
 *  Migrate an AuReiNand install to a Luma3DS by renaming the aurei/
 *  folder to luma/
 *
 *  \return true if the migration was successful, false otherwise
 */
bool arnMigrate();