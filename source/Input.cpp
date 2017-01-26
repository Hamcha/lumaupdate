#include "Input.h"

void Input::ScanInput() {
	hidScanInput();
	kheld = hidKeysHeld();
	kdown = hidKeysDown();
	kup = hidKeysUp();
}

const bool Input::IsKeyHeld(u32 keys) { return Input::Get().kheld & keys; }
const bool Input::IsKeyDown(u32 keys) { return Input::Get().kdown & keys; }
const bool Input::IsKeyUp(u32 keys)   { return Input::Get().kup   & keys; }