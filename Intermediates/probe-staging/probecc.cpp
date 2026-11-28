#include "pch.h"
#include "Global.h"
#include <fstream>
#include <sstream>
namespace Gecko::Exports::Global { void yield(); void yieldFor(uint32_t); }
using namespace Gecko;
namespace Gecko
{
	ScriptManager* TheScriptManager = nullptr;
	bool g_Active[(size_t)RunContext::Count] = {};
	bool IsRunContextActive(RunContext) { return false; }
	WaitState* ScriptManager::CurrentWaitState() { return nullptr; }
}
static void Msg(const asSMessageInfo* m, void*) { printf("%s (%d,%d) %s: %s\n", m->section, m->row, m->col, m->type == asMSGTYPE_ERROR ? "ERR" : m->type == asMSGTYPE_WARNING ? "WARN" : "INFO", m->message); }
static void Dummy(asIScriptGeneric*) {}
#define R(x) do { int r_ = (x); if (r_ < 0) printf("reg fail %d: %s\n", r_, #x); } while (0)

int main(int, char** argv)
{
	asIScriptEngine* E = asCreateScriptEngine();
	E->SetMessageCallback(asFUNCTION(Msg), 0, asCALL_CDECL);
	E->SetEngineProperty(asEP_PROPERTY_ACCESSOR_MODE, 2);
	E->SetEngineProperty(asEP_DISALLOW_EMPTY_LIST_ELEMENTS, true);
	RegisterStdString(E);
	RegisterScriptArray(E, true);
	Exports::Global::RegisterCoroutines(E);
	R(E->RegisterGlobalFunction("void yield()", asFUNCTION(Dummy), asCALL_GENERIC));
	R(E->RegisterGlobalFunction("void print(const string&in)", asFUNCTION(Dummy), asCALL_GENERIC));
	R(E->RegisterObjectType("CMwNod", 0, asOBJ_REF | asOBJ_NOCOUNT));
	R(E->RegisterGlobalFunction("void RegisterLoadCallback(uint id)", asFUNCTION(Dummy), asCALL_GENERIC));
	R(E->RegisterGlobalFunction("void UnregisterLoadCallback(uint id)", asFUNCTION(Dummy), asCALL_GENERIC));
	R(E->RegisterObjectType("vec2", 8, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS));
	R(E->RegisterObjectBehaviour("vec2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Dummy), asCALL_GENERIC));
	E->SetDefaultNamespace("Reflection");
	R(E->RegisterObjectType("MwClassInfo", 0, asOBJ_REF | asOBJ_NOCOUNT));
	R(E->RegisterObjectProperty("MwClassInfo", "const uint ID", 0));
	R(E->RegisterObjectMethod("MwClassInfo", "string get_Name() const property", asFUNCTION(Dummy), asCALL_GENERIC));
	R(E->RegisterGlobalFunction("MwClassInfo@ GetType(const string&in name)", asFUNCTION(Dummy), asCALL_GENERIC));
	R(E->RegisterGlobalFunction("MwClassInfo@ TypeOf(CMwNod@ nod)", asFUNCTION(Dummy), asCALL_GENERIC));
	E->SetDefaultNamespace("Text");
	R(E->RegisterGlobalFunction("string Format(const string&in format, uint)", asFUNCTION(Dummy), asCALL_GENERIC));
	E->SetDefaultNamespace("UI");
	R(E->RegisterGlobalFunction("bool Begin(const string&in title, int flags = 0)", asFUNCTION(Dummy), asCALL_GENERIC));
	R(E->RegisterGlobalFunction("void End()", asFUNCTION(Dummy), asCALL_GENERIC));
	R(E->RegisterGlobalFunction("void Text(const string&in text, int length = -1)", asFUNCTION(Dummy), asCALL_GENERIC));
	R(E->RegisterGlobalFunction("void Separator()", asFUNCTION(Dummy), asCALL_GENERIC));
	R(E->RegisterGlobalFunction("bool Button(const string&in label, const vec2&in size = vec2())", asFUNCTION(Dummy), asCALL_GENERIC));
	E->SetDefaultNamespace("Meta");
	Exports::Global::RegisterRunContexts(E);
	E->SetDefaultNamespace("");

	std::ifstream F(argv[1]); std::stringstream SS; SS << F.rdbuf();
	asIScriptModule* M = E->GetModule("m", asGM_ALWAYS_CREATE);
	M->AddScriptSection("Main.as", SS.str().c_str());
	int r = M->Build();
	printf(r < 0 ? "BUILD FAILED\n" : "BUILD OK\n");
	const char* Decls[] = { "void Main()", "void Update(float dt)", "void RenderInterface()", "void OnLoadCallback(CMwNod@ nod)" };
	for (const char* D : Decls) printf("  %-36s %s\n", D, M && M->GetFunctionByDecl(D) ? "found" : "MISSING");
	E->ShutDownAndRelease();
}
