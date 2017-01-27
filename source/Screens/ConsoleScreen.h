#pragma once

#include "../Screen.h"

class ConsoleScreen : public Screen {
public:
	void Render();

	ConsoleScreen();

private:
	static sftd_font* ConsoleFont;

	std::string content;
};
