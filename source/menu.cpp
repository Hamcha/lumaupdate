#include "menu.h"

void menuPrintHeader(PrintConsole* con, const char* version) {
	con->cursorX = 2;
	con->cursorY = 1;
	printf("%sARN Updater v%s%s\n\n", CONSOLE_YELLOW, version, CONSOLE_RESET);
}

void menuPrintFooter(PrintConsole* con) {
	con->cursorX = 2;
	con->cursorY = con->consoleHeight - 1;
	printf("< > select options   A choose   START quit");
}