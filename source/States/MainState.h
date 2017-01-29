#pragma once

#include "../State.h"

class MainState : public State {
public:
	void Render();

	MainState();

private:
	enum class MainMenuState {
		Detect, //! Detecting installed version
		Fetch,  //! Fetching updated versions from internet
		Wait,   //! Waiting for user input
	};

	MainMenuState currentState = MainMenuState::Detect;
};