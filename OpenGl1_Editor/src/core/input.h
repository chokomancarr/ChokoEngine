#pragma once
#include "Engine.h"

/*! Mouse, keyboard, and touch input
[av] */
class Input {
public:
	static Vec2 mousePos, mousePosRelative, mouseDelta, mouseDownPos;
	static bool mouse0, mouse1, mouse2;
	static byte mouse0State, mouse1State, mouse2State;
	static string inputString;
	static void UpdateMouseNKeyboard(bool* src = nullptr);

	static bool KeyDown(InputKey key), KeyHold(InputKey key), KeyUp(InputKey key);

	static uint touchCount;
	static std::array<uint, 10> touchIds;
	static std::array<Vec2, 10> touchPoss;
	//static std::array<float, 10> touchForce;
	static std::array<byte, 10> touchStates;
	class Motion {
	public:
		static Vec2 Pan();
		static Vec2 Zoom();
		static Vec3 Rotate();
	};
#ifdef PLATFORM_ADR
	static void UpdateAdr(AInputEvent* e);
#endif

	Vec2 _mousePos, _mousePosRelative, _mouseDelta, _mouseDownPos;
	bool _mouse0, _mouse1, _mouse2;
	bool _keyStatuses[325];

	static void PreLoop();

	friend class Engine;
	friend struct Editor_PlaySyncer;
#ifdef IS_EDITOR
	friend class PopupSelector;
#endif
protected:
	static void RegisterCallbacks(), TextCallback(GLFWwindow*, uint);
	static bool keyStatusOld[325], keyStatusNew[325];
private:
	static Vec2 mousePosOld;
	static string _inputString;
	//Input(Input const &); //deliberately not defined
	//Input& operator= (Input const &);
};