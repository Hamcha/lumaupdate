#include "MainState.h"

void MainState::Render() {
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
	console.WriteLine(std::string("Luma Updater ") + GIT_VERSION);
}
