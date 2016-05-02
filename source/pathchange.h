#pragma once

#include <3ds/types.h>

#include <string>

#define MAXPATHLEN 37

/*! \brief Apply the pathchanger patch to a ARN payload
 *
 *  \param buf     Buffer containing the payload data
 *  \param bufSize Size of the buffer
 *  \param path    Path to apply, must be 37 characters or less
 *
 *  \return true if the buffer is successfully changed without errors, false otherwise
 */
bool pathchange(u8* buf, const size_t bufSize, const std::string& path);