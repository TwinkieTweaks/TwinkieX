#pragma once
#include "SECRETS.h"
// #include "discordpp.h"
#define _CRT_SECURE_NO_WARNINGS
#include <DiscordGameSDK/discord.h>
#undef _CRT_SECURE_NO_WARNINGS

#ifdef TMCN
#define DISCORD_APP_ID 1520334169883017348ULL
#elif defined(TM1)
#define DISCORD_APP_ID 1520353467095449681ULL
#elif defined(TMO)
#define DISCORD_APP_ID 1520334569847521390ULL
#elif defined(TMS)
#define DISCORD_APP_ID 0ULL
#elif defined(TMSX)
#define DISCORD_APP_ID 1520352680260931645ULL
#elif defined(ESWC)
#define DISCORD_APP_ID 1520353733261656134ULL
#else
#error "No target game defined."
#endif