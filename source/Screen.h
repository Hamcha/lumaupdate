#pragma once

#include "libs.h"

/*! \brief (ABSTRACT) A single UI state providing controls and functionality
 */
class Screen {
public:
	/*! \brief Render screen contents
	 */
	virtual void Render() = 0;
};