
const array<string> ContextNames = {
	"Main", "BeforeScripts", "MainLoop", "GameLoop", "AfterMainLoop", "NetworkAfterMainLoop", "AfterScripts", "UpdateSceneEngine"
};

bool InMainTick = false;
uint MainTicks = 0;

array<uint> Runs(8);
array<uint> RunsInMainTick(8);

array<string> CurrentOrder;
array<string> LastOrder;

array<string> Results;

void Pass(const string&in what) { Results.InsertLast("\\$0f0PASS\\$z " + what); print("[Probe] PASS " + what); }
void Fail(const string&in what) { Results.InsertLast("\\$f00FAIL\\$z " + what); print("[Probe] FAIL " + what); }

void ContextLoop(int64 context)
{
	while (true)
	{
		Runs[context]++;
		if (InMainTick) RunsInMainTick[context]++;
		if (context != 0) CurrentOrder.InsertLast(ContextNames[context]);
		yield();
	}
}

uint SleepWakes = 0;
uint SleepWakesInMainTick = 0;
void SleepLoop()
{
	while (true)
	{
		sleep(250);
		SleepWakes++;
		if (InMainTick) SleepWakesInMainTick++;
	}
}

uint SwitchedRuns = 0;
uint SwitchedRunsInMainTick = 0;
void SwitchedLoop()
{
	while (true)
	{
		SwitchedRuns++;
		if (InMainTick) SwitchedRunsInMainTick++;
		yield();
	}
}

class Box { string Value; }
void CheckRef(ref@ r) { Box@ b = cast<Box>(r); if (b !is null && b.Value == "box") Pass("userdata ref"); else Fail("userdata ref"); }
void CheckInt(int64 v) { if (v == -12345) Pass("userdata int"); else Fail("userdata int: " + v); }
void CheckInt64(int64 v) { if (v == -1234567890123) Pass("userdata int64"); else Fail("userdata int64: " + v); }
void CheckUint(uint64 v) { if (v == 4000000000) Pass("userdata uint"); else Fail("userdata uint: " + v); }
void CheckUint64(uint64 v) { if (v == 12345678901234) Pass("userdata uint64"); else Fail("userdata uint64: " + v); }
void CheckDouble(double v) { if (v == 1.5) Pass("userdata double"); else Fail("userdata double: " + v); }
void CheckString(const string&in v) { if (v == "hello") Pass("userdata string"); else Fail("userdata string: " + v); }
void Noop() {}

const array<string> LoadClassNames = { "CGameCtnChallenge", "CPlugSolid", "CPlugBitmap", "CGameCtnReplayRecord", "CGameCtnGhost" };
array<uint> LoadClassIds;
array<uint> LoadHits;
array<uint> LoadHitsInMainTick;
array<string> LoadLastName;
uint UnregisteredId = 0;
uint UnregisteredHits = 0;

void OnLoadCallback(CMwNod@ nod)
{
	if (nod is null)
	{
		Fail("OnLoadCallback got a null nod");
		return;
	}
	auto info = Reflection::TypeOf(nod);
	uint id = info is null ? 0 : info.ID;
	if (id == UnregisteredId)
	{
		UnregisteredHits++;
		return;
	}
	int idx = LoadClassIds.Find(id);
	if (idx < 0) return;
	LoadHits[idx]++;
	if (InMainTick) LoadHitsInMainTick[idx]++;
	LoadLastName[idx] = info.Name;
}

void SetupLoadCallbacks()
{
	for (uint i = 0; i < LoadClassNames.Length; i++)
	{
		auto info = Reflection::GetType(LoadClassNames[i]);
		if (info is null)
		{
			print("[Probe] Class " + LoadClassNames[i] + " doesn't exist in this game, skipping it");
			continue;
		}
		LoadClassIds.InsertLast(info.ID);
		LoadHits.InsertLast(0);
		LoadHitsInMainTick.InsertLast(0);
		LoadLastName.InsertLast("");
		RegisterLoadCallback(info.ID);
	}

	auto unreg = Reflection::GetType("CPlugShader");
	if (unreg !is null)
	{
		UnregisteredId = unreg.ID;
		RegisterLoadCallback(UnregisteredId);
		UnregisterLoadCallback(UnregisteredId);
	}
}

void Main()
{
	SetupLoadCallbacks();

	for (int c = 0; c < 8; c++) Meta::StartWithRunContext(Meta::RunContext(c), ContextLoop, int64(c));
	Meta::StartWithRunContext(Meta::RunContext::GameLoop, SleepLoop);
	startnew(SwitchedLoop).WithRunContext(Meta::RunContext::AfterMainLoop);

	Box b;
	b.Value = "box";
	Meta::StartWithRunContext(Meta::RunContext::MainLoop, CheckRef, b);
	Meta::StartWithRunContext(Meta::RunContext::MainLoop, CheckInt, -12345);
	Meta::StartWithRunContext(Meta::RunContext::MainLoop, CheckInt64, int64(-1234567890123));
	Meta::StartWithRunContext(Meta::RunContext::MainLoop, CheckUint, uint(4000000000));
	Meta::StartWithRunContext(Meta::RunContext::MainLoop, CheckUint64, uint64(12345678901234));
	Meta::StartWithRunContext(Meta::RunContext::MainLoop, CheckDouble, 1.5);
	Meta::StartWithRunContext(Meta::RunContext::MainLoop, CheckString, "hello");

	bool threw = false;
	try
	{
		Meta::StartWithRunContext(Meta::RunContext(42), Noop);
	}
	catch
	{
		threw = true;
	}
	if (threw) Pass("invalid run context throws");
	else Fail("invalid run context didn't throw");

	while (true)
	{
		InMainTick = true;
		MainTicks++;
		LastOrder = CurrentOrder;
		CurrentOrder.RemoveRange(0, CurrentOrder.Length);
		yield();
	}
}

void Update(float dt)
{
	InMainTick = false;
}

string Verdict(uint runs, uint inMain, bool isMain)
{
	if (runs == 0) return "\\$f00never ran";
	if (isMain) return inMain == runs ? "\\$0f0in the Main tick, as expected" : "\\$f00ran outside the Main tick";
	if (inMain == 0) return "\\$0f0hooked, runs between frames";
	if (inMain == runs) return "\\$fa0not hooked, falls back to the Main tick";
	return "\\$f00mixed: " + inMain + " of " + runs + " runs were in the Main tick";
}

void RenderInterface()
{
	if (UI::Begin("Run Context Probe"))
	{
		UI::Text("Main ticks: " + MainTicks);
		UI::Separator();

		for (uint c = 0; c < 8; c++)
		{
			UI::Text(ContextNames[c] + ": " + Runs[c] + " runs, " + RunsInMainTick[c] + " in the Main tick  " + Verdict(Runs[c], RunsInMainTick[c], c == 0));
		}
		UI::Text("sleep() in GameLoop: " + SleepWakes + " wakes  " + Verdict(SleepWakes, SleepWakesInMainTick, false));
		UI::Text("WithRunContext(AfterMainLoop): " + SwitchedRuns + " runs  " + Verdict(SwitchedRuns, SwitchedRunsInMainTick, false));

		UI::Separator();
		string order = "";
		for (uint i = 0; i < LastOrder.Length; i++) order += (i == 0 ? "" : " > ") + LastOrder[i];
		UI::Text("Order in the last frame: " + (order.Length == 0 ? "(none outside Main)" : order));

		UI::Separator();
		UI::Text("OnLoadCallback (load a map, a replay or a ghost to trigger it)");
		for (uint i = 0; i < LoadClassIds.Length; i++)
		{
			string id = Text::Format("%08x", LoadClassIds[i]);
			UI::Text(LoadClassNames[i] + " (" + id + "): " + LoadHits[i] + " loads, " + LoadHitsInMainTick[i] + " during the Main tick" + (LoadLastName[i].Length > 0 ? ", last: " + LoadLastName[i] : ""));
		}
		if (UnregisteredId != 0)
		{
			UI::Text("CPlugShader, registered then unregistered: " + UnregisteredHits + (UnregisteredHits == 0 ? " \\$0f0(good)" : " \\$f00(should be 0)"));
		}

		UI::Separator();
		for (uint i = 0; i < Results.Length; i++) UI::Text(Results[i]);

		if (UI::Button("Print report to the log")) PrintReport();
	}
	UI::End();
}

void PrintReport()
{
	print("[Probe] Main ticks: " + MainTicks);
	for (uint c = 0; c < 8; c++) print("[Probe] " + ContextNames[c] + ": " + Runs[c] + " runs, " + RunsInMainTick[c] + " in the Main tick");
	print("[Probe] sleep() in GameLoop: " + SleepWakes + " wakes, " + SleepWakesInMainTick + " in the Main tick");
	print("[Probe] WithRunContext(AfterMainLoop): " + SwitchedRuns + " runs, " + SwitchedRunsInMainTick + " in the Main tick");
	string order = "";
	for (uint i = 0; i < LastOrder.Length; i++) order += (i == 0 ? "" : " > ") + LastOrder[i];
	print("[Probe] Order in the last frame: " + order);
	for (uint i = 0; i < LoadClassIds.Length; i++) print("[Probe] " + LoadClassNames[i] + ": " + LoadHits[i] + " loads, " + LoadHitsInMainTick[i] + " during the Main tick");
	if (UnregisteredId != 0) print("[Probe] CPlugShader (unregistered): " + UnregisteredHits);
	for (uint i = 0; i < Results.Length; i++) print("[Probe] " + Results[i]);
}
