#include "menu.h"

#include <cstdio>

#ifndef GIT_VER
#define GIT_VER "<unknown>"
#endif

void menuPrintHeader(PrintConsole* con) {
	con->cursorX = 2;
	con->cursorY = 1;
	printf("%sLuma3DS Updater %s%s\n\n", CONSOLE_YELLOW, GIT_VER, CONSOLE_RESET);
}

void menuPrintFooter(PrintConsole* con) {
	con->cursorX = 2;
	con->cursorY = con->consoleHeight - 1;
	printf("< > select options   A choose   START quit");
}