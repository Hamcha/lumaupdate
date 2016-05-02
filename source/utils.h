#pragma once

#include <string>

#include <3ds/types.h>

#define CHECK(val, msg) if (val != 0) { throw formatErrMessage(msg, val); }

/*! \brief Formats error messages so they are more readable as exceptions
 *
 *  \param msg Error message
 *  \param val Result from function call
 *
 *  \return Formatted error string for exceptions
 */
std::string formatErrMessage(const char* msg, const Result val);

/*! \brief Checks whether a file exists or not
 *
 *  \param path Full path to file
 *
 *  \return true if it exists and can be opened, false otherwise
 */
bool fileExists(const std::string path);

/*! \brief Trim whitespace around strings */
void trim(std::string &s);