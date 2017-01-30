#pragma once

#include "../Screen.h"

class ConsoleScreen : public Screen {
public:
	// Console screen parameters
	u16 margin   = 5;  //!< Text margin from screen borders
	u16 fontSize = 12; //!< Font size

	void Render();
	ConsoleScreen();
	~ConsoleScreen();

	/*! \brief Write text to console screen
	 *  \param text Text to write
	 */
	void Write(const std::string text);

	/*! \brief Write a line to console screen
	 *  \param text Line to write
	 */
	void WriteLine(const std::string text);

private:
	sftd_font* font;
	std::vector<std::string> lines;
};
