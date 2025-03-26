#pragma once

// define GLOBED_IGNORE_CONFIG_HPP if you want to define all those manually
// with cmake flags or whatever floats your boat
#ifndef GLOBED_IGNORE_CONFIG_HPP

/* platform: any */

// disable voice
// #define GLOBED_DISABLE_VOICE_SUPPORT

// disalbe custom keybinds
// #define GLOBED_DISABLE_CUSTOM_KEYBINDS

// various debugging options
#if defined(GLOBED_DEBUG) && GLOBED_DEBUG

// # define GLOBED_DEBUG_INTERPOLATION // dump all interpolation stuff
// # define GLOBED_DEBUG_PACKETS // log all incoming and outgoing packets & bandwidth
// # define GLOBED_DEBUG_PACKETS_PRINT // also print each packet

#endif // GLOBED_DEBUG

/* Options made to help port the mod faster to a new GD version / other platform */

// Disable hooks/calls that aren't strictly necessary to mod's functionality
// #define GLOBED_DISABLE_EXTRA_HOOKS


/* platform-specific: Windows */

#define GLOBED_FMOD_WINDOWS 1
#define GLOBED_DRPC_WINDOWS 0

/* platform-specific: Mac */

#define GLOBED_FMOD_MAC 1
#define GLOBED_DRPC_MAC 0

/* platform-specific: Android */

#define GLOBED_FMOD_ANDROID 1
#define GLOBED_DRPC_ANDROID 0

/* platform-specific: iOS */

#define GLOBED_FMOD_IOS 0
#define GLOBED_DRPC_IOS 0

#endif // GLOBED_IGNORE_CONFIG_HPP

#define G_WC24252435(x, y) x##y
#define G_WC(x, y) G_WC24252435(x, y)

#include <stdint.h>

using LevelId = int64_t; // 64-bit
static_assert(sizeof(LevelId) == 8, "level id is not 8 bytes");
