// This file is for checking that the current build configuration is valid for the target game.
#pragma once''

// _M_X64 is defined for x64
#ifdef _M_X64
#define X64
#else
#define X86
#endif

// TMCN (TrackmaniaChina) is an x64 game
#if defined(TMCN) and defined(X86)
#error "TMCN is only supported on x64 architecture."
#endif

// Define GAMEBOX for TM1, TMO, TMS, TMSX and ESWC, since those are all Gamebox games
// Technically, TMCN is also built on Gamebox, but we separate it to Maniaplanet because of its differences to the rest
#if defined(TM1) || defined(TMO) || defined(TMS) || defined(TMSX) || defined(ESWC)
// GAMEBOX is for TM1, TMO, TMS, TMSX and ESWC
#define GAMEBOX
#elif defined(TMCN)
// MANIAPLANET is for TMCN
#define MANIAPLANET
#endif

// TM1, TMO, TMS, TMSX and ESWC are x86 games
#if defined(GAMEBOX) and defined(X64)
#error "TM1, TMO, TMS, TMSX and ESWC are only supported on x86 architecture."
#endif

#if !defined(TMCN) && !defined(TM1) && !defined(TMO) && !defined(TMS) && !defined(TMSX) && !defined(ESWC)
#error "No target game defined. Please define one of the following: TMCN, TM1, TMO, TMS, TMSX, ESWC."
#endif