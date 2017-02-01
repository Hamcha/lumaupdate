#include "Application.h"

#include "Input.h"

#include "States/MainState.h"

Application::Application() {
	// Initialize SF2D
	sf2d_init();
	sf2d_set_clear_color(RGBA8(0x20, 0x20, 0x20, 0xFF));
	sf2d_set_vblank_wait(0);
	sf2d_set_3D(0);

	// Initialize SFTD
	sftd_init();

	// Initialize default state
	SetState(new MainState());

	// Initialize lastFrameTime to a sane value
	lastFrameTime = osGetTime();
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
		// Calculate delta time
		int currentTime = osGetTime();
		deltaTime = (currentTime - lastFrameTime) / 1000.f;

		// Refresh input
		Input::Get().ScanInput();

		// Render current state
		currentState->Render();

		sf2d_swapbuffers();
		lastFrameTime = currentTime;
	}

	return returnCode;
}

float Application::DeltaTime() {
	return deltaTime;
}


void Application::SetState(State* newState) {
	currentState = std::unique_ptr<State>(newState);
}

void Application::Exit(const int retCode) {
	keepRunning = false;
	returnCode = retCode;
}
