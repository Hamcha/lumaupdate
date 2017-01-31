#include "MainState.h"

void MainState::Render() {
	RenderScreen(GFX_TOP, &loadingScreen);
	RenderScreen(GFX_BOTTOM, &console);

	//TODO Handle this stuff
	switch (currentState) {
		case MainMenuState::Detect:
			break;
		case MainMenuState::Fetch:
			break;
		case MainMenuState::Wait:
			break;
	}
}

MainState::MainState() {
	console.Write(std::string("Luma Updater ") + GIT_VERSION);

	loadingScreen.SetStepTitle("Detecting installed version");
	loadingScreen.SetStepTitle("Checking for new versions of Luma3DS");
	loadingScreen.SetStepProgress(1, 3);
}
