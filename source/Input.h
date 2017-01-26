#pragma once

#include "libs.h"

/*! \brief Input masterclass
 *
 *  Input should be refreshed only once in the main loop
 */
class Input {
public:
	//
	// Singleton shim
	//

	/*! \brief Get Input instance (singleton)
	 *  \return Input instance, same one for every call
	 */
	static Input& Get() {
		static Input instance;
		return instance;
	}

	Input(Input const&) = delete;
	Input(Input&&) = delete;
	Input& operator=(Input const&) = delete;
	Input& operator=(Input &&) = delete;

	//
	// Public methods
	//

	/*! \brief Update input state with system functions
	 */
	void ScanInput();

	/*! \brief Check if one or more keys are being held
	 *  \param keys Key or keys to check (OR'd)
	 *  \return true if the key is being held, false otherwise
	 */
	static const bool IsKeyHeld(u32 keys);

	/*! \brief Check if one or more keys are pressed down
	 *  \param keys Key or keys to check (OR'd)
	 *  \return true if the key is pressed down, false otherwise
	 */
	static const bool IsKeyDown(u32 keys);

	/*! \brief Check if one or more keys have been just released
	 *  \param keys Key or keys to check (OR'd)
	 *  \return true if the key has been released, false otherwise
	 */
	static const bool IsKeyUp(u32 keys);

	//
	// Input state variables
	//

	/*! Key state (A B X Y L R etc) */
	u32 kheld, kdown, kup;

protected:
	Input() {}
	~Input() {}
};