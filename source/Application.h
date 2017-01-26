#pragma once

#include "Screen.h"

/*! \brief Empty screen (for initial status) */
class NoScreen : public Screen {
	void render() {}
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

	/*! \brief Switch to a new screen
	 *
	 *  \param target    Screen to render to (GFX_TOP / GFX_BOTTOM)
	 *  \param newScreen Screen to switch to
	 */
	void SetScreen(const gfxScreen_t target, Screen& newScreen);

	/*! \brief Signal the application to exit on next frame 
	 *
	 *  \param returnCode (OPTIONAL) Return code
	 */
	void Exit(const int retCode = 0);

protected:
	Application()
		: topScreen(noScreen), bottomScreen(noScreen) {
		sf2d_init();
		sf2d_set_clear_color(RGBA8(0x40, 0x40, 0x40, 0xFF));
		sf2d_set_3D(0);;
	}

	~Application() {
		sf2d_fini();
	}

private:
	ScreenRef topScreen, bottomScreen;
	NoScreen noScreen; // Empty screen (initial value for both screens)

	bool keepRunning = true; // Wether the app should keep running
	int returnCode = 0;
};