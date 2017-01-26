#include "libs.h"

#include "Screen.h"
#include "Input.h"

ScreenRef topScreen, bottomScreen;

int main() {
	sf2d_init();
	sf2d_set_clear_color(RGBA8(0x40, 0x40, 0x40, 0xFF));
	sf2d_set_3D(0);

	while (aptMainLoop()) {
		Input::Get().ScanInput();

		if (Input::IsKeyDown(KEY_START)) {
			break;
		}

		// Render top and bottom screens
		if (topScreen != nullptr) {
			sf2d_start_frame(GFX_TOP, GFX_LEFT);
			topScreen->render();
			sf2d_end_frame();
		}
		if (bottomScreen != nullptr) {
			sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
			bottomScreen->render();
			sf2d_end_frame();
		}

		sf2d_swapbuffers();
	}

	sf2d_fini();
	return 0;
}