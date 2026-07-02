#include "pch.h"
#include <Twinkie/TwinkDiscordRP/TwinkDiscordRP.h>

TwinkDiscordRP::TwinkDiscordRP(TwinkTrackmania& TrackmaniaMgr)
{
    this->TrackmaniaMgr = &TrackmaniaMgr;

    discord::Result NellyCreationResult = discord::Core::Create(DISCORD_APP_ID, DiscordCreateFlags_NoRequireDiscord, &Nelly);
    if (NellyCreationResult != discord::Result::Ok)
    {
        Nelly = nullptr;
    }
}

TwinkDiscordRP::~TwinkDiscordRP()
{
    delete Nelly;
}

void TwinkDiscordRP::Update()
{
	// If Discord is initalized
	if (Nelly)
	{
		auto& ActivityMgr = Nelly->ActivityManager();

		auto Map = TrackmaniaMgr->ParamGet<CMwNod*>(TrackmaniaMgr->GetApp(),
#ifdef MANIAPLANET
			"RootMap"
#else
			"Challenge"
#endif
		);
		auto Activity = discord::Activity();
		if (!Map)
		{
			Activity.SetState("In-game");

			ActivityMgr.UpdateActivity(Activity, []([[maybe_unused]] discord::Result) {});
			Nelly->RunCallbacks();

			return;
		}

		Activity.SetState("Playing a map");

		// Because in older games, the editor is always active, even if you're just playing a map in TA.
#ifdef MANIAPLANET
		auto Editor = TrackmaniaMgr->ParamGet<CMwNod*>(TrackmaniaMgr->GetApp(), "Editor");

		if (Editor)
		{
			Activity.SetState("Editing a map");
		}
#endif

		// TODO: TM1 uses a different format for strings, see TMTypes.h
		CFastStringInt* pMapNameStr = TrackmaniaMgr->ParamGet<CFastStringInt>(Map,
#ifdef MANIAPLANET
			"MapName"
#else
			"ChallengeName"
#endif
		);

		if (!pMapNameStr)
		{
			ActivityMgr.UpdateActivity(Activity, []([[maybe_unused]] discord::Result) {});
			Nelly->RunCallbacks();

			return;
		}

#ifndef TM1
#ifdef TMCN
#pragma warning(push)
#pragma warning(disable : 4267)
#endif
		char8_t MapNameUTF8[257] = {};
		WideCharToMultiByte(CP_UTF8, 0, pMapNameStr->CStr, pMapNameStr->Count, (char*)MapNameUTF8, 256, NULL, NULL);
#ifdef TMCN
#pragma warning(pop)
#endif
		Activity.SetDetails((char*)MapNameUTF8);
#else
		Activity.SetDetails(pMapNameStr->CStr);
#endif
		ActivityMgr.UpdateActivity(Activity, []([[maybe_unused]] discord::Result) {});

		Nelly->RunCallbacks();
	}
}