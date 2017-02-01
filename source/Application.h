#pragma once

#include "State.h"

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
	void SetState(State* newState);

	/*! \brief Signal the application to exit on next frame
	 *
	 *  \param returnCode (OPTIONAL) Return code
	 */
	void Exit(const int retCode = 0);

	/*! \brief Get delta time (time since last frame) in seconds 
	 *
	 *  \return Time since last frame (in seconds)
	 */
	float DeltaTime();

protected:
	Application();
	~Application();

private:
	std::unique_ptr<State> currentState;
	int lastFrameTime = 0;
	float deltaTime = 0;

	bool keepRunning = true; // Wether the app should keep running
	int returnCode = 0;
};