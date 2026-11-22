#pragma once
#include <nanovg/src/nanovg.h>

namespace TwinkNvg
{
	extern NVGcontext* Context;
	extern uint32_t Generation;

	bool IsInFrame();

	bool Init();
	void Invalidate();

	void BeginFrame(float Width, float Height);
	void EndFrame();
}
