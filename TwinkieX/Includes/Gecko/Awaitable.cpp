#include "pch.h"
#include "Awaitable.h"

namespace Gecko
{
	void Awaitable::AddRef()
	{
		RefCount++;
	}

	void Awaitable::Release()
	{
		if (--RefCount == 0) delete this;
	}

	bool WaitState::Ready()
	{
		if (Frames > 0)
		{
			Frames--;
			return false;
		}

		if (WakeAt != std::chrono::steady_clock::time_point{} && std::chrono::steady_clock::now() < WakeAt) return false;

		if (!Awaiting.empty())
		{
			size_t StillRunning = 0;
			for (Awaitable* Item : Awaiting)
			{
				if (Item->IsRunning()) StillRunning++;
			}
			if (AwaitAny ? StillRunning == Awaiting.size() : StillRunning > 0) return false;
		}

		Clear();
		return true;
	}

	void WaitState::Clear()
	{
		for (Awaitable* Item : Awaiting) Item->Release();
		Awaiting.clear();
		AwaitAny = false;
		Frames = 0;
		WakeAt = {};
	}

	Coroutine::~Coroutine()
	{
		Stop();
	}

	bool Coroutine::IsRunning() const
	{
		return Running;
	}

	void Coroutine::Stop()
	{
		if (Ctx)
		{
			if (Ctx->GetState() == asEXECUTION_SUSPENDED) Ctx->Abort();
			Ctx->Release();
			Ctx = nullptr;
		}
		Wait.Clear();
		Running = false;
	}
}
