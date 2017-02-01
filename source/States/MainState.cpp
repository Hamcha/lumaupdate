#include "MainState.h"

void MainState::SetState(const MainMenuState newState) {
	currentState = newState;
	drawFrame = true;
}

void MainState::Render() {
	RenderScreen(GFX_TOP, &loadingScreen);
	RenderScreen(GFX_BOTTOM, &console);

	switch (currentState) {
		case MainMenuState::Detect:
			if (drawFrame) {
				loadingScreen.SetStepTitle("Detecting installed version");
				loadingScreen.SetStepProgressInfinite();
				drawFrame = false;
			}
			Utils::GetLumaVersion(&currentLumaVersion);
			//TODO Handle invalid version (Luma too old?)
			console.WriteLine("Luma3DS version detected: " + Utils::FormatLumaVersion(currentLumaVersion));
			SetState(MainMenuState::Fetch);
			break;
		case MainMenuState::Fetch:
			if (drawFrame) {
				loadingScreen.SetStepTitle("Looking for new Luma3DS releases");
				loadingScreen.SetStepProgressInfinite();
				drawFrame = false;
			}
			break;
		case MainMenuState::Wait:
			break;
	}
}

MainState::MainState() {
	currentState = MainMenuState::Detect;
	console.WriteLine(std::string("Luma Updater ") + GIT_VERSION);
	console.Write("\n");
}
