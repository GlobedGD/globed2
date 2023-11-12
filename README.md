# Globed

Globed is an open-source, highly customizable multiplayer mod for Geometry Dash.

This repository contains the complete rewrite of Globed, for Geometry Dash 2.2 and all future versions. If you want the 2.1 version, the [old repository](https://github.com/dankmeme01/globed) is still up, however it is no longer maintained.

## Installation

Globed is a [Geode](https://github.com/geode-sdk/geode) mod, so it requires you to install Geode first. Once that's done, simply open the mods page in-game and download it from the index.

## Hosting a server

todo

## Roadmap

Planned features:

* wait for 2.2

Known issues:

* i am silly
* voice chat is a bit silly

## Building

Building should be the same as any other mod, except for Android. On Android, when configuring the mod you must set `-DANDROID_PLATFORM=android-28` or higher. versions below do not have the `getrandom()` syscall so libsodium will fail to link. This means the mod is **incompatible** with versions of Android below 9.

## Credit

ca7x3, Firee, Croozington, Coloride, Cvolton, mat, alk, maki, xTymon - thank you for being awesome, whether it's because you helped me, suggested ideas, or if I just found you awesome in general :D

camila314 - thank you for [UIBuilder](https://github.com/camila314/uibuilder)

RobTop - thank you for releasing this awesome game :)

## Open source acknowledgments

* [Geode](https://github.com/geode-sdk/geode) - the one thing that made all of this possible :)
* [Opus](https://github.com/xiph/opus) - audio codec used for audio compression
* [Sodium](https://github.com/jedisct1/libsodium) - crypto library used for data encryption