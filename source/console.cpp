#include "console.h"

#ifndef GIT_VER
#define GIT_VER "<unknown>"
#endif

PrintConsole* consoleCurrent = nullptr;
PrintConsole consoleTop, consoleBottom;

void consoleInitEx() {
	consoleInit(GFX_TOP, &consoleTop);
	consoleInit(GFX_BOTTOM, &consoleBottom);
}

void consoleScreen(gfxScreen_t screen) {
	switch (screen) {
	case GFX_TOP:
		consoleCurrent = &consoleTop;
		break;
	case GFX_BOTTOM:
		consoleCurrent = &consoleBottom;
		break;
	default:
		// uh?!
		throw "Trying to select inexistant console screen";
	}
	consoleSelect(consoleCurrent);
}

void consolePrintHeader() {
	consoleMoveTo(2, 1);
	consoleCurrent->cursorX = 2;
	consoleCurrent->cursorY = 1;
	std::printf("%sLuma3DS Updater %s%s\n\n", CONSOLE_YELLOW, GIT_VER, CONSOLE_RESET);
}

void consolePrintFooter() {
	consoleMoveTo(2, consoleCurrent->consoleHeight - 1);
	std::printf("< > select options   A choose   START quit");
}

void consoleMoveTo(int x, int y) {
	consoleCurrent->cursorX = x;
	consoleCurrent->cursorY = y;
}