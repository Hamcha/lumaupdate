#pragma once

#include <stdio.h>

#include <3ds.h>

/* \brief Prints the menu header
 * \param con     PrintConsole instance
 * \param version Version to print
 */
void menuPrintHeader(PrintConsole* con, const char* version);

/* \brief Prints the menu footer
 * \param con     PrintConsole instance
 */
void menuPrintFooter(PrintConsole* con);