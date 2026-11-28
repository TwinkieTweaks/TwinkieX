void Loop(int64 n)
{
	for (int i = 0; i < 2; i++) { print("loop " + n); yield(); }
}

void Plain()
{
	print("plain");
	yield();
	print("plain again");
}

void Ref(ref@ r)
{
	string@ s = cast<string>(r);
	print("ref " + s);
}

void Str(const string&in s) { print("str " + s); }
void Dbl(double d) { print("dbl " + d); }
void U64(uint64 u) { print("u64 " + u); }

void Switched()
{
	print("switched");
	yield();
	print("switched again");
}

void Main()
{
	Meta::StartWithRunContext(Meta::RunContext::BeforeScripts, Plain);
	Meta::StartWithRunContext(Meta::RunContext::GameLoop, Loop, 7);
	Meta::StartWithRunContext(Meta::RunContext::AfterScripts, Loop, int64(-9));
	string s = "hello";
	Meta::StartWithRunContext(Meta::RunContext::UpdateSceneEngine, Ref, @s);
	Meta::StartWithRunContext(Meta::RunContext::MainLoop, Str, "abc");
	Meta::StartWithRunContext(Meta::RunContext::NetworkAfterMainLoop, Dbl, 1.5);
	Meta::StartWithRunContext(Meta::RunContext::AfterMainLoop, U64, uint64(123456789012));
	Meta::StartWithRunContext(Meta::RunContext::Main, Plain);
	startnew(Switched).WithRunContext(Meta::RunContext::AfterMainLoop);
	print("bad context next");
	Meta::StartWithRunContext(Meta::RunContext(42), Plain);
	print("not reached");
}
