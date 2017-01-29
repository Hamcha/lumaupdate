#pragma once

#include "State.h"

/*! \brief Empty state (for initial status) */
class NoState : public State {
	void Render() {}
};

/*! \brief Application logic and screen manager class */
class Application {
public:
	//
	// Singleton shim
	//

	/*! \brief Get Application instance (singleton)
	 *  \return Application instance, same one for every call
	 */
	static Application& Get() {
		static Application instance;
		return instance;
	}

	Application(Application const&) = delete;
	Application(Application&&) = delete;
	Application& operator=(Application const&) = delete;
	Application& operator=(Application &&) = delete;

	//
	// Public methods
	//

	/*! \brief Run main loop until an error occurs or the application exits
	 *  \return Return code
	 */
	int Run();

	/*! \brief Switch to a new UI state
	 *
	 *  \param newScreen Screen to switch to
	 */
	void SetState(State& newState);

	/*! \brief Signal the application to exit on next frame
	 *
	 *  \param returnCode (OPTIONAL) Return code
	 */
	void Exit(const int retCode = 0);

protected:
	Application();
	~Application();

private:
	State& currentState = noState;
	NoState noState; // Empty screen (initial value for both screens)

	bool keepRunning = true; // Wether the app should keep running
	int returnCode = 0;
};