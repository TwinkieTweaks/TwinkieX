#pragma once
#include "SECRETS.h"
// #include "discordpp.h"
#define _CRT_SECURE_NO_WARNINGS
#include <DiscordGameSDK/discord.h>
#undef _CRT_SECURE_NO_WARNINGS

#ifdef TMCN
#define DISCORD_APP_ID 1520334169883017348ULL
#elif defined(TM1)
#elif defined(TMO)
#define DISCORD_APP_ID 1520334569847521390ULL
#elif defined(TMS)
#elif defined(TMSX)
#elif defined(ESWC)
#else
#error "No target game defined."
#endif

//extern std::shared_ptr<discordpp::Client> gDiscordClient;
//extern discordpp::Activity gDiscordActivity;

// Nelly, the Discord Game SDK core
extern discord::Core* gNelly;