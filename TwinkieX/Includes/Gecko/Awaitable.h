#pragma once
#include "pch.h"

namespace Gecko
{
	enum class RunContext : int32_t
	{
		Main,
		BeforeScripts,
		MainLoop,
		GameLoop,
		AfterMainLoop,
		NetworkAfterMainLoop,
		AfterScripts,
		UpdateSceneEngine,
		Count
	};

	class Awaitable
	{
		int RefCount = 1;
	public:
		virtual ~Awaitable() = default;
		virtual bool IsRunning() const = 0;
		virtual void SetRunContext(RunContext) {}

		void AddRef();
		void Release();
	};

	struct WaitState
	{
		uint32_t Frames = 0;
		std::chrono::steady_clock::time_point WakeAt{};
		std::vector<Awaitable*> Awaiting;
		bool AwaitAny = false;

		bool Ready();
		void Clear();
	};

	class Coroutine final : public Awaitable
	{
	public:
		asIScriptContext* Ctx = nullptr;
		WaitState Wait;
		std::string StringArg;
		RunContext Context = RunContext::Main;
		bool Running = true;

		~Coroutine();

		bool IsRunning() const override;
		void SetRunContext(RunContext NewContext) override;
		void Stop();
	};
}
