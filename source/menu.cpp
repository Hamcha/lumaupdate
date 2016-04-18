#include "menu.h"

void menuPrintHeader(PrintConsole* con, const char* version) {
	con->cursorX = 2;
	con->cursorY = 1;
	printf("ARN Updater v%s\n\n", version);
}

void menuPrintFooter(PrintConsole* con) {
	con->cursorX = 2;
	con->cursorY = con->consoleHeight - 1;
	printf("< > select options   A choose   START quit");
}