#pragma once

#include "libs.h"

/*! \brief Initializes top and bottom consoles */
void consoleInitEx();

/*! \brief Select what screen to use for console operations */
void consoleScreen(const gfxScreen_t screen);

/*! \brief Prints the menu header */
void consolePrintHeader();

/*! \brief Prints the menu footer */
void consolePrintFooter();

/*! \brief Move console's cursor to a specified position 
 *
 *  \param x X position to move the cursor to (column)
 *  \param y Y position to move the cursor to (row)
*/
void consoleMoveTo(const int x, const int y);

/*! \brief Make progress bar (aka loading) screen (with optional text and progress)
 *
 *  \param header   Progress bar header
 *  \param text     Text to put below the progress bar (optional)
 *  \param progress Progress to set (optional)
 */
void consoleInitProgress(const char* header, const char* text, const float progress);

/*! \brief Sets both text and value of a progress bar screen 
 *  \param text     Text to put below the progress bar
 *  \param progress Progress to set (from 0 to 1 inclusive)
 */
void consoleSetProgressData(const char* text, const float progress);

/*! \brief Change progress bar's text 
 *
 *  \param text Text to put below the progress bar
 */
void consoleSetProgressText(const char* text);

/*! \brief Change progress bar value
 *
 *  \param progress Progress to set (from 0 to 1 inclusive)
 */
void consoleSetProgressValue(const float progress);