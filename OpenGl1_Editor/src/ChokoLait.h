#pragma once

#define CHOKO_LAIT

#include "Engine.h"

typedef void(*emptyCallbackFunc)(void);

class ChokoLait {
public:
	void Init(int scrW, int scrH);

	void Update(emptyCallbackFunc);
	
	void Paint(emptyCallbackFunc);
};