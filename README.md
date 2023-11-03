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

## Contributing

If you want to contribute, please read the [Contributor guide](./contribution.md).

## Building

For building the server, you need nothing more than a Rust toolchain. Past that, it's essentially the same as any other Rust project. Building the client is, however, a bit more complex.

### Windows

Open the latest [libsodium](https://github.com/jedisct1/libsodium) release, download the asset called `libsodium-1.x.y-msvc.zip` and unzip it into `libs/`

Then just proceed with the CMake build, like you would in any other mod.

### Mac

gotta figure it out somehow

### Android

alrighty, let's have some fun shall we? if you are not on linux then use WSL. make sure to have android NDK installed.

for the first build, you need to build libsodium.

```sh
# change ndk home to whatever folder you have it installed in
ANDROID_NDK_HOME=/opt/android-ndk ./build-sodium-android.sh
```

this will pull the repo and build it from source. if you don't see any errors then you're epic! the built libraries should be automatically copied into the correct directory.

now simply use your epic build script and build the mod with cmake like you would build any other mod!

NOTE: when configuring you must set `-DANDROID_PLATFORM=android-28` or higher. versions below do not have the `getrandom()` syscall so libsodium will fail to link. this means the mod is **incompatible** with versions of Android below 9.

## Credit

ca7x3, Firee, Croozington, Coloride, Cvolton, mat, alk, maki, xTymon - thank you for being awesome, whether it's because you helped me, suggested ideas, or if I just found you awesome in general :D

camila314 - thank you for [UIBuilder](https://github.com/camila314/uibuilder)

RobTop - thank you for releasing this awesome game :)