#include "Engine.h"
#include "Editor.h"

bool Input::mouse0 = false;
bool Input::mouse1 = false;
bool Input::mouse2 = false;
byte Input::mouse0State = 0;
byte Input::mouse1State = 0;
byte Input::mouse2State = 0;
string Input::inputString = "";

Vec2 Input::mousePos = Vec2(0, 0);
Vec2 Input::mousePosRelative = Vec2(0, 0);
Vec2 Input::mousePosOld = Vec2(0, 0);
Vec2 Input::mouseDelta = Vec2(0, 0);
Vec2 Input::mouseDownPos = Vec2(0, 0);

bool Input::keyStatusOld[] = {};
bool Input::keyStatusNew[] = {};

uint Input::touchCount;
std::array<uint, 10> Input::touchIds = std::array<uint, 10>();
std::array<Vec2, 10> Input::touchPoss = std::array<Vec2, 10>();
//std::array<float, 10> Input::touchForce = std::array<float, 10>();
std::array<byte, 10> Input::touchStates = std::array<byte, 10>();

string Input::_inputString = "";

void Input::RegisterCallbacks() {
	glfwSetInputMode(Display::window, GLFW_STICKY_KEYS, 1);
	glfwSetCharCallback(Display::window, TextCallback);
#ifdef IS_EDITOR
	glfwSetCharCallback(PopupSelector::window, TextCallback);
#endif
}

void Input::TextCallback(GLFWwindow* w, uint i) {
	_inputString += string((char*)&i, 1);
}

bool Input::KeyDown(InputKey k) {
	return keyStatusNew[k - 32] && !keyStatusOld[k - 32];
}

bool Input::KeyHold(InputKey k) {
	return keyStatusNew[k - 32];
}

bool Input::KeyUp(InputKey k) {
	return !keyStatusNew[k - 32] && keyStatusOld[k - 32];
}

void Input::UpdateMouseNKeyboard(bool* src) {
	//#ifdef PLATFORM_WIN
	memcpy(keyStatusOld, keyStatusNew, 325);
	if (src) std::swap_ranges(src, src + 325, keyStatusNew);
	else {
		/*
		for (byte a = 1; a < 112; a++) {
		keyStatusNew[a] = ((GetAsyncKeyState(a) >> 8) == -128);
		}
		for (byte a = Key_Plus; a <= Key_Dot; a++) {
		keyStatusNew[a] = ((GetAsyncKeyState(a) >> 8) == -128);
		}
		*/
		for (uint i = 32; i < 97; i++) {
			keyStatusNew[i - 32] = !!glfwGetKey(Display::window, i);
		}
		for (uint i = 256; i < 347; i++) {
			keyStatusNew[i - 32] = !!glfwGetKey(Display::window, i);
		}
	}
	//#endif
	/*
	bool shift = KeyHold(Key_LeftShift);
	byte b;
	for (b = Key_0; b <= Key_9; b++) {
	if (KeyDown((InputKey)b)) {
	inputString += char(b);
	}
	}
	for (b = Key_NumPad0; b <= Key_NumPad9; b++) {
	if (KeyDown((InputKey)b)) {
	inputString += char(b-48);
	}
	}
	for (b = Key_A; b <= Key_Z; b++) {
	if (KeyDown((InputKey)b)) {
	inputString += shift ? char(b) : char(b + 32);
	}
	}
	for (b = Key_Plus; b <= Key_Dot; b++) {
	if (KeyDown((InputKey)b)) {
	inputString += char(b-0x90);
	}
	}
	if (KeyDown(Key_Space)) inputString += " ";
	*/
	if (mouse0)
		mouse0State = min<byte>(mouse0State + 1U, MOUSE_HOLD);
	else
		mouse0State = ((mouse0State == MOUSE_UP) | (mouse0State == 0)) ? 0 : MOUSE_UP;
	if (mouse1)
		mouse1State = min<byte>(mouse1State + 1U, MOUSE_HOLD);
	else
		mouse1State = ((mouse1State == MOUSE_UP) | (mouse1State == 0)) ? 0 : MOUSE_UP;
	if (mouse2)
		mouse2State = min<byte>(mouse2State + 1U, MOUSE_HOLD);
	else
		mouse2State = ((mouse2State == MOUSE_UP) | (mouse2State == 0)) ? 0 : MOUSE_UP;

	if (mouse0State == MOUSE_DOWN)
		mouseDownPos = mousePos;
	else if (!mouse0State)
		mouseDownPos = Vec2(-1, -1);

	mouseDelta = mousePos - mousePosOld;
	mousePosOld = mousePos;
}

Vec2 Input::Motion::Pan() {
	return Vec2();
}

Vec2 Input::Motion::Zoom() {
	return Vec2();
}

Vec3 Input::Motion::Rotate() {
	return Vec3();
}

#ifdef PLATFORM_ADR
void Input::UpdateAdr(AInputEvent* e) {
	if (e) {
		touchCount = AMotionEvent_getPointerCount(e);
		for (uint a = 0; a < touchCount; a++) {
			touchIds[a] = AMotionEvent_getPointerId(e, a);
			touchPoss[a].x = AMotionEvent_getRawX(e, a);
			touchPoss[a].y = AMotionEvent_getRawY(e, a);
			//touchForce[a] = AMotionEvent_getPressure(e, a);
		}
		auto act = AMotionEvent_getAction(e);
		uint id = act >> 8;
		switch (act & 255) {
		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
			touchStates[id] = MOUSE_DOWN;
			break;
		case AMOTION_EVENT_ACTION_UP:
		case AMOTION_EVENT_ACTION_POINTER_UP:
			touchStates[id] = MOUSE_UP;
			break;
		default:
			touchStates[id] = MOUSE_HOLD;
			break;
		}
	}
	else {
		if (touchCount == 1 && touchStates[0] == MOUSE_UP)
			touchCount = 0;
	}
}
#endif

void Input::PreLoop() {
	inputString = _inputString;
	_inputString.clear();
}