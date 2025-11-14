// This file is for checking that the current build configuration is valid for the target game.
#pragma once

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

// TM1, TMO, TMS, TMSX and ESWC are x86 games
#if (defined(TM1) || defined(TMO) || defined(TMS) || defined(TMSX) || defined(ESWC)) and defined(X64)
#error "TM1, TMO, TMS, TMSX and ESWC are only supported on x86 architecture."
#endif