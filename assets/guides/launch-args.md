# Globed Launch Arguments

Globed has several launch arguments that can be useful for troubleshooting or debugging. To enable them, pass the options into either the command line when launching the executable (if invoking directly), in the Steam launch options section, or in the Geode launcher settings (if on Android).

## Asset Preloading

- **`--geode:globed-debug-preload`**: Enables extra logging during asset preloading. This is helpful in diagnosing crashes or texture corruption issues.
  
- **`--geode:globed-skip-preload`**: Forces asset preloading to be disabled, regardless of in-game settings.

## Troubleshooting

- **`--geode:globed-skip-resource-check`**: Skips the resource check at startup. This can be helpful if there’s a texture mismatch, often caused by outdated texture packs that modify Globed textures.
  
- **`--geode:globed-no-ssl-verification`**: Disables SSL certificate verification. This option is useful if you're unable to connect to the server with an error like "SSL peer certificate or SSH remote key was not OK."

- **`--geode:reset-settings`**: - Resets all Globed settings.
  
- **`--geode:globed-crt-fix`**: This is not used in release builds. However, if you are a developer using Wine and experiencing launch hangs, you may find more details in `src/platform/os/windows/setup.cpp`.

## Debugging (Developer-Centered)

- **`--geode:globed-tracing`**: Enables additional logging to help with debugging.
  
- **`--geode:globed-verbose-curl`**: Enables verbose curl logging, which can be helpful for diagnosing issues with web requests.
  
- **`--geode:globed-fake-server-data`**: Simulates a more active server environment. Even if there are no players connected to the server, this option will populate the player list with fake players, and generate fake levels and rooms. **ONLY** works in debug builds (`-DGLOBED_DEBUG=ON` set during mod build).
