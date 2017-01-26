#include "Application.h"

#include "Input.h"

void Application::SetScreen(const gfxScreen_t target, Screen& newScreen) {
	switch (target) {
	case GFX_TOP:
		topScreen = newScreen;
		break;
	case GFX_BOTTOM:
		bottomScreen = newScreen;
		break;
	}
}

int Application::Run() {
	while (aptMainLoop() && keepRunning) {
		Input::Get().ScanInput();

		if (Input::IsKeyDown(KEY_START)) {
			break;
		}

		// Render top and bottom screens
		sf2d_start_frame(GFX_TOP, GFX_LEFT);
		topScreen.render();
		sf2d_end_frame();

		sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
		bottomScreen.render();
		sf2d_end_frame();

		sf2d_swapbuffers();
	}

	return returnCode;
}

void Application::Exit(const int retCode) {
	keepRunning = false;
	returnCode = retCode;
}