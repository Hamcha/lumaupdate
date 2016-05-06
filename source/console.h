#pragma once

#include "libs.h"

/*! \brief Initializes top and bottom consoles */
void consoleInitEx();

/*! \brief Select what screen to use for console operations */
void consoleScreen(gfxScreen_t screen);

/*! \brief Prints the menu header */
void consolePrintHeader();

/*! \brief Prints the menu footer */
void consolePrintFooter();

/*! \brief Move console's cursor to a specified position 
 *
 *  \param x X position to move the cursor to (column)
 *  \param y Y position to move the cursor to (row)
*/
void consoleMoveTo(int x, int y);