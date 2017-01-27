#include "Application.h"

#include "Input.h"

Application::Application() {
	// Initialize SF2D
	sf2d_init();
	sf2d_set_clear_color(RGBA8(0x40, 0x40, 0x40, 0xFF));
	sf2d_set_3D(0);

	// Initialize SFTD
	sftd_init();
}

Application::~Application() {
	// Dispose SF2D
	sf2d_fini();

	// Dispose SFTD
	sftd_fini();
}

int Application::Run() {
	// Main loop
	while (aptMainLoop() && keepRunning) {
		// Refresh input
		Input::Get().ScanInput();

		// Render top and bottom screens
		currentState.Render();

		sf2d_swapbuffers();
	}

	return returnCode;
}

void Application::SetState(State& newState) {
	currentState = newState;
}

void Application::Exit(const int retCode) {
	keepRunning = false;
	returnCode = retCode;
}
