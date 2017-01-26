#include "Input.h"

void Input::ScanInput() {
	hidScanInput();
	kheld = hidKeysHeld();
	kdown = hidKeysDown();
	kup = hidKeysUp();
}

const bool Input::IsKeyHeld(const u32 keys) { return Input::Get().kheld & keys; }
const bool Input::IsKeyDown(const u32 keys) { return Input::Get().kdown & keys; }
const bool Input::IsKeyUp(const u32 keys)   { return Input::Get().kup   & keys; }