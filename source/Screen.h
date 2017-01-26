#pragma once

#include "libs.h"

/*! \brief (ABSTRACT) A single UI screen providing controls and functionality on one 3DS screen
 */
class Screen {
public:
	/*! \brief Render screen contents
	 */
	virtual void render() = 0;
};

typedef Screen& ScreenRef;