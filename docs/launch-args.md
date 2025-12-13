# Globed launch arguments

Globed has a few launch arguments that might be useful in troubleshooting certain issues or just debugging. To enable them, pass the options into either the command line when launching the executable (if invoking directly), in steam launch options section, or in Geode launcher settings (if on Android).

Make sure to use the full format and prefix the option name with `globed/`, so if an option is called `core.skip-preload`, the full launch option will be called `--geode:globed/core.skip-preload`

`net.dont-override-dns` - use system DNS instead of 1.1.1.1/8.8.8.8
