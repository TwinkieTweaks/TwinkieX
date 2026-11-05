#include "pch.h"
#include "Net.h"
#include <Gecko/Exports/GeckoAutoExport.h>

namespace Gecko::Exports::Net
{
	static std::vector<GeckoExport> s_BaseExports;
	static std::vector<GeckoExport> s_Exports;

#define NET_HTTPREQUEST_METHOD(Decl_, Ptr_, Conv_)          GECKO_EXPORT_METHOD(s_BaseExports, "Net", "HttpRequest", Decl_, Ptr_, Conv_)
#define NET_HTTPREQUEST_BEHAVIOUR(Beh_, Decl_, Ptr_, Conv_) GECKO_EXPORT_BEHAVIOUR(s_BaseExports, "Net", "HttpRequest", Beh_, Decl_, Ptr_, Conv_)
#define TO_STRINGS(...) __VA_ARGS__##,
	// A comma. For use with NET_HTTPMETHOD_ENUM.
#define C ,
	// Usage: NET_HTTPMETHOD_ENUM({ "Value1" C "Value2" }, { 0, 1 })
	// Don't ask.
#define NET_HTTPMETHOD_ENUM(Names_, ...) GECKO_EXPORT_ENUM(s_BaseExports, "Net", "HttpMethod", Names_, __VA_ARGS__)

	/*
	!CompatMeta
	for     Net
	with    HttpMethod
	as      HttpMethod
	being   enum
	declare full
	since   v0.1.0
	!CompatMeta
	*/
	NET_HTTPMETHOD_ENUM({"Get" C "Post" C "Head" C "Put" C "Delete" C "Patch"}, {0, 1, 2, 3, 4, 5});
#undef C
}