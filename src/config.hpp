#pragma once

// define GLOBED_IGNORE_CONFIG_HPP if you want to define all those manually
// with cmake flags or whatever floats your boat
#ifndef GLOBED_IGNORE_CONFIG_HPP

/* platform: any */

// disable voice if you have skill issues setting up opus
// #define GLOBED_DISABLE_VOICE_SUPPORT

// various debugging options
#if defined(GLOBED_DEBUG) && GLOBED_DEBUG

// for source location, unrecommended because may break things
# define GLOBED_FORCE_CONSTEVAL 1
// # define GLOBED_DEBUG_PACKETS // log all incoming and outgoing packets & bandwidth

#endif // GLOBED_DEBUG

/* platform-specific: Windows */

#define GLOBED_FMOD_WINDOWS 1
#define GLOBED_DRPC_WINDOWS 0

/* platform-specific: Mac */

#define GLOBED_FMOD_MAC 0
#define GLOBED_DRPC_MAC 0

/* platform-specific: Android */

#define GLOBED_FMOD_ANDROID 1
#define GLOBED_DRPC_ANDROID 0



#endif // GLOBED_IGNORE_CONFIG_HPP