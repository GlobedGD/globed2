# Globed launch arguments

Globed has a few launch arguments that might be useful in troubleshooting certain issues or just debugging. To enable them, pass the options into either the command line when launching the executable (if invoking directly), in steam launch options section, or in Geode launcher settings (if on Android).

Make sure to use the full format and prefix the option name with `globed-`, so if an option is called `skip-preload`, the full launch option is called `--geode:globed-skip-preload`

## Asset preloading

`debug-preload` - enables printing of a ton of extra information during asset preloading, so that in case there's a crash or a texture corruption isuse, it can be debugged easier.

`skip-preload` - forces asset preloading to be disabled, no matter the in-game settings.

## Troubleshooting

`skip-resource-check` - skips the resource check at the startup, which would cause globed to be unusable if a texture mismatch was detected (usually because of an outdated texture pack that modifies globed textures).

`no-ssl-verification` - disables SSL certificate verification, useful if you can't connect to the server with an error similar to "SSL peer certificate or SSH remote key was not OK."

`reset-settings` - resets all Globed settings

`crt-fix` - unused in release builds, if you are a developer using Wine and experiencing hangs on launch, you may check `src/platform/os/windows/setup.cpp` for more information.

## Debugging (more dev centered)

`tracing` - enables some extra logging.

`net-dump` - dumps as much network information as possible, both to the console and to a log file located in mod's save directory

`verbose-curl` - enables verbose curl logging (can help figure out problems with web requests)

`fake-server-data` - emulates a more lively server, for example, even if the server has no players connected to it, with this option, there will be a lot of fake players on the player list. same with fake levels and rooms. **ONLY** works in debug builds (`-DGLOBED_DEBUG=ON` was set when building the mod)

`dev-stuff` - adds extra toggles in certain places that are otherwise unavailable

