#include "State.h"

void State::RenderScreen(gfxScreen_t target, Screen * screen) {
	sf2d_start_frame(target, GFX_LEFT);
	screen->Render();
	sf2d_end_frame();
}
