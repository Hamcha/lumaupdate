#pragma once

#include "libs.h"

#ifdef __GNUC__
#define NAKED __attribute__((naked))
#define UNUSED __attribute__((unused))
#else
// Visual studio really hates GCC-specific syntax (I mean, it's right)
#undef PACKED
#define PACKED
#define NAKED
#define UNUSED
#endif

#define CHECK(val, msg) if (val != 0) { throw std::runtime_error(formatErrMessage(msg, val)); }

/*! \brief Formats error messages so they are more readable as exceptions
 *
 *  \param msg Error message
 *  \param val Result from function call
 *
 *  \return Formatted error string for exceptions
 */
std::string formatErrMessage(const char* msg, const Result& val);

/*! \brief Checks whether a file exists or not
 *
 *  \param path Full path to file
 *
 *  \return true if it exists and can be opened, false otherwise
 */
bool fileExists(const std::string& path);

/*! \brief Trim whitespace around strings (in-place)
 *
 *  \param s String to trim whitespace from
 */
void trim(std::string& s);

/*! \brief Unescape string sequences
 *
 *  \param s String to unescape sequences from
 *
 *  \return Unescaped string
 */
std::string unescape(const std::string& s);

/*! \brief Strip Markdown formatting
 *
 *  \param text Text to strip markdown from
 *
 *  \return Text without markdown links and formatting
 */
std::string stripMarkdown(std::string text);

/*! \brief Indent multiline text
 *
 *  \param text   Text to indent
 *  \param cols   Number of columns (for word wrapping)
 *
 *  \return Text indented and wrapped
 */
std::string indent(const std::string& text, const size_t cols);

/*! \brief Get how many pages a multiline text makes 
 *
 *  \param text Text to count pages from
 *  \param rows Number of lines per page
 *
 *  \return Number of pages the text forms
 */
int getPageCount(const std::string& text, const int rows);

/*! \brief Get one specific page of a multiline text
 *
 *  \param text Text to extract specific page from
 *  \param num  Page number
 *  \param rows Number of lines per page
 *
 *  \return Requested page, or blank string if it's an inexistant page
 */
std::string getPage(const std::string& text, const int num, const int rows);

/*! \brief Initialize log file 
 *  \param path Log file path
 */
void logInit(const char* path);

/*! \brief Closes log file and cleans up resources */
void logExit();

/*! \brief Print information on both screen and logfile */
void logPrintf(const char* format, ...);

/*! \brief Alternative to_string implementation (workaround for mingw)
 *
 *  \param n Input parameter
 *
 *  \return String representation of the input parameter
 */
template<typename T> std::string tostr(const T& n) {
	std::ostringstream stm;
	stm << n;
	return stm.str();
}