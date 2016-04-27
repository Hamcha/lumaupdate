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
