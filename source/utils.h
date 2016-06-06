#pragma once

#include "libs.h"

#define CHECK(val, msg) if (val != 0) { throw formatErrMessage(msg, val); }

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
 *  \param indent What to indent lines with
 *  \param cols   Number of columns (for word wrapping)
 *
 *  \return Text indented and wrapped
 */
std::string indent(std::string text, const std::string& indent, const size_t cols);

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