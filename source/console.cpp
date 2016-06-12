#include "console.h"

#ifndef GIT_VER
#define GIT_VER "<unknown>"
#endif

PrintConsole* consoleCurrent = nullptr;
PrintConsole consoleTop, consoleBottom;

#define LINE_BLANK "                                                  "

void consoleInitEx() {
	consoleInit(GFX_TOP, &consoleTop);
	consoleInit(GFX_BOTTOM, &consoleBottom);
}

void consoleScreen(const gfxScreen_t screen) {
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
	std::printf("%sLuma Updater %s%s\n\n", CONSOLE_YELLOW, GIT_VER, CONSOLE_RESET);
}

void consolePrintFooter() {
	consoleMoveTo(2, consoleCurrent->consoleHeight - 2);
	std::printf("\x18\x19 select options     A choose     START quit");
}

void consoleMoveTo(const int x, const int y) {
	consoleCurrent->cursorX = x;
	consoleCurrent->cursorY = y;
}

void consoleClearLine() {
	consoleCurrent->cursorX = 0;
	std::printf("%.*s", consoleCurrent->consoleWidth, LINE_BLANK);
	consoleCurrent->cursorX = 0;
}

/* Progress bar functions */
#define ProgressBarPadding 4

void consoleInitProgress(const char* header, const char* text, const float progress) {
	consoleClear();

	// Print progress bar borders
	int progressBarY = consoleCurrent->consoleHeight / 2 - 1;
	consoleCurrent->cursorY = progressBarY;

	int startX = ProgressBarPadding;
	int endX = consoleCurrent->consoleWidth - ProgressBarPadding + 1;

	int startY = progressBarY;

	for (int i = startX; i < endX; i++) {
		// Draw left and right border
		for (int j = 0; j < 12; j++) {
			consoleCurrent->frameBuffer[((startX * 8 - 3) * 240) + (230 - (startY * 8)) + j] = 0xffff;
			consoleCurrent->frameBuffer[((endX * 8 - 6) * 240) + (230 - (startY * 8)) + j] = 0xffff;
		}
		// Draw top and bottom borders
		for (int j = 0; j < (i < endX - 1 ? 8 : 6); j++) {
			consoleCurrent->frameBuffer[((i * 8 + j - 3) * 240) + (239 - (startY * 8 - 3))] = 0xffff;
			consoleCurrent->frameBuffer[((i * 8 + j - 3) * 240) + (239 - ((startY + 1) * 8 + 2))] = 0xffff;
		}
	}

	// Print header
	consoleCurrent->cursorY = progressBarY - 2;
	consoleCurrent->cursorX = ProgressBarPadding;
	std::printf("%s%s%s", CONSOLE_YELLOW, header, CONSOLE_RESET);

	// Set data
	consoleSetProgressData(text, progress);
}

void consoleSetProgressData(const char* text, const float progress) {
	consoleSetProgressText(text);
	consoleSetProgressValue(progress);
}

void consoleSetProgressText(const char* text) {
	// Move to approriate row
	int progressBarY = consoleCurrent->consoleHeight / 2 - 1;
	consoleCurrent->cursorY = progressBarY + 2;

	// Clear line
	consoleCurrent->cursorX = 0;
	std::printf("%.*s", consoleCurrent->consoleWidth, LINE_BLANK);

	// Write text
	consoleCurrent->cursorX = ProgressBarPadding;
	std::printf("%s...", text);
}

void consoleSetProgressValue(const float progress) {
	// Move to approriate row
	consoleCurrent->cursorY = consoleCurrent->consoleHeight / 2 - 1;

	// Move to beginning of progress bar
	int progressBarLength = consoleCurrent->consoleWidth - ProgressBarPadding*2;
	consoleCurrent->cursorX = ProgressBarPadding;

	// Fill progress
	int progressBarFill = (int)(progressBarLength * progress);
	consoleCurrent->flags |= CONSOLE_COLOR_REVERSE;
	std::printf("%.*s", progressBarFill, LINE_BLANK);
	consoleCurrent->flags &= ~CONSOLE_COLOR_REVERSE;
	std::printf("%.*s", progressBarLength - progressBarFill, LINE_BLANK);
}