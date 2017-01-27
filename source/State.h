#pragma once

#include "libs.h"

#include "Screen.h"

/*! \brief (ABSTRACT) A single state in which the program can find itself in
 */
class State {
protected:
	/*! \brief Render screen state on a physical 3DS screen
	 *
	 *  \param target What screen to render to (GFX_TOP/BOTTOM)
	 *  \param screen Screen state to render
	 */
	void RenderScreen(gfxScreen_t target, Screen* screen);

public:
	/*! \brief Render screen contents
	 */
	virtual void Render() = 0;
};