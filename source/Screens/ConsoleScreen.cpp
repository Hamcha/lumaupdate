#include "ConsoleScreen.h"

#include <saxmono_ttf.h>

ConsoleScreen::ConsoleScreen() {
	// Initialize global console font if it hasn't been initialized before
	if (ConsoleFont == nullptr) {
		ConsoleFont = sftd_load_font_mem(saxmono_ttf, saxmono_ttf_size);
	}
}

void ConsoleScreen::Render() {
	int tw, th;
	const char* txt = "Testing";
	sftd_calc_bounding_box(&tw, &th, ConsoleFont, 20, 300, txt);
	sf2d_draw_rectangle(10, 40, tw, th, RGBA8(0, 100, 0, 255));
	sftd_draw_text_wrap(ConsoleFont, 10, 40, RGBA8(255, 255, 255, 255), 20, 300, txt);
}