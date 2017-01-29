#include "MainState.h"

void MainState::Render() {
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

}
