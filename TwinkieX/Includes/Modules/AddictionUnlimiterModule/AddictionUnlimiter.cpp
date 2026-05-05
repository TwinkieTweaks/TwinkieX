#include "pch.h"
#ifdef TMCN
#include "AddictionUnlimiter.h"

AddictionUnlimiter::~AddictionUnlimiter() {}

void AddictionUnlimiter::Render()
{
	CMwNod* App = Twinkie->GetApp();

	auto UserMgr = Twinkie->ParamGet<CMwNod*>(App, "UserManagerScript");
	if (!UserMgr) return;

	auto UsersArray = Twinkie->ParamGet<CFastBuffer<CMwNod*>>(*UserMgr, "Users");
	if (!UsersArray) return;

	for (auto& User : *UsersArray)
	{
		if (!User) continue;

		auto UserConfig = Twinkie->ParamGet<CMwNod*>(User, "Config");
		if (!UserConfig) continue;

		int DailyQuotaMinutes = 90;
		Twinkie->ParamSet(*UserConfig, "AddictionLimiter_DailyQuotaMinutes", &DailyQuotaMinutes);
	}
}
#endif