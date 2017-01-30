#pragma once

#include "../Screen.h"

class ProgressScreen : public Screen {
public:
	void Render();
	ProgressScreen();
	~ProgressScreen();

private:
	sftd_font* font;
	sf2d_texture* background;
};
