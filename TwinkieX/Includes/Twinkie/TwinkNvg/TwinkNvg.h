#pragma once
#include <nanovg/src/nanovg.h>

namespace TwinkNvg
{
	extern NVGcontext* Context;

	bool Init();
	void Invalidate();

	void BeginFrame(float Width, float Height);
	void EndFrame();
}
