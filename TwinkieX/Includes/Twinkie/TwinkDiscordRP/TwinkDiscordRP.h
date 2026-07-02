#pragma once
#include <DiscordGameSDK/DiscordStuff.h>
#include <Twinkie/TwinkTrackmania/TwinkTrackmania.h>

// The class that dictates and manages the Discord Rich Presence for Twinkie
class TwinkDiscordRP
{
	// Nelly, the Discord Game SDK Core
	discord::Core* Nelly = nullptr;

	// The trackmania manager
	TwinkTrackmania* TrackmaniaMgr = nullptr;

public:
	TwinkDiscordRP(TwinkTrackmania& TrackmaniaMgr);
	~TwinkDiscordRP();

	void Update();
};