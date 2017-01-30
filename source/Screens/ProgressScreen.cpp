#include "ProgressScreen.h"

#include <OpenSans_Regular_ttf.h>
#include <loading_bg_png.h>

ProgressScreen::ProgressScreen() {
	font = sftd_load_font_mem(OpenSans_Regular_ttf, OpenSans_Regular_ttf_size);
	background = sfil_load_PNG_buffer(loading_bg_png, SF2D_PLACE_RAM);
}

ProgressScreen::~ProgressScreen() {
	sf2d_free_texture(background);
}

void ProgressScreen::Render() {
	// Draw background first
	sf2d_draw_texture(background, 0, 0);
}
