# Globed Server

## Prerequisites

Before trying to setup a server, it is **recommended** that you understand what it involves. You will likely need to either setup port forwarding or use a VPN tool like Radmin VPN if you are hosting the server on your PC, and that won't be covered in this guide.

In case you are familiar with [Pterodactyl](https://pterodactyl.io/), there are eggs available for the [central](https://github.com/DumbCaveSpider/globed-central-egg) and [game](https://github.com/DumbCaveSpider/globed-game-egg) servers that could simplify the setup (thanks to [@DumbCaveSpider](https://github.com/DumbCaveSpider/))

Additionally, if you are setting up a big public server, please keep in mind that the architecture is *not* stable. Big changes can be made to the server at any point, and updates to the mod can cause your server to stop working until you also update it.

## Setup

If you want to host a server yourself, first you have to download the server binaries from the [latest GitHub release](https://github.com/dankmeme01/globed2/releases/latest), named `globed-central-server` and `globed-game-server`. Depending on your OS and architecture, you want the one ending in `.exe` on Windows, the `-x64` one on Linux x64, and the `-arm64` one on Linux ARM64.

After that is done, you have 2 paths:

* If you want to setup a small, simple server you can jump to the [standalone section](#standalone)
* If you want to setup a bigger or more configurable server, keep reading.

First, you want to launch the central server binary. It should create two files called `central-conf.json` and `Rocket.toml` in the folder you ran it from. This is where you can configure everything about the server. For documentation about all the options, jump to the [configuration section](#central-server-configuration), however for now we only need the option `game_server_password`.

With your central server properly setup and started, jump to the [bridged](#bridged) section of the game server configuration and launch the game server, with the password that you configured earlier.

If you did everything right, you should see no errors or warnings in the console and instead you should see "Server launched on x.x.x.x". This means everything worked! Congrats :)

## Game server configuration

note: if you are not on Windows, in the following examples replace `set` with `export` and replace `globed-game-server.exe` with the appropriate server binary (such as `globed-game-server-x64`)

### Standalone

If you want to spin up a quick, standalone game server, without needing to start a central server, then it is as simple as running the `globed-game-server.exe` executable directly.

If you want to change the port then you'll have to run the executable with an additional argument like so:
```sh
# replace 0.0.0.0:41001 with your address and port
globed-game-server.exe 0.0.0.0:41001
```

To connect to your server, you want to use the Direct Connection option inside the server switcher in-game. (with the address `127.0.0.1:41001` if the server is running on the same device)

Keep in mind that a standalone server makes the configuration very limited (for example you can't blacklist/whitelist users anymore) and disables any kind of player authentication.


### Bridged

To start the game server and bridge it together with an active central server you must use the password from the `game_server_password` option in the central server configuration. Then, you have 2 options whenever you start the server:

```sh
globed-game-server.exe 0.0.0.0:41001 http://127.0.0.1:41000 password

# or like this:

set GLOBED_GS_ADDRESS=0.0.0.0:41001
set GLOBED_GS_CENTRAL_URL=http://127.0.0.1:41000
set GLOBED_GS_CENTRAL_PASSWORD=password
globed-game-server.exe
```

Replace `0.0.0.0:41001` with the address you want the game server to listen on, `http://127.0.0.1:41000` with the URL of your central server, and `password` with the password.

### Additional parameters

`GLOBED_GS_NO_FILE_LOG` - if set to 1, don't create a log file and only log to the console.

## Central server configuration

The central server allows configuration hot reloading, so you can modify the configuration file and see updates in real time without restarting the server.

By default, the file is created with the name `central-conf.json` in the current working directory when you run the server, but it can be overriden with the environment variable `GLOBED_CONFIG_PATH`. The path can be a folder or a full file path.


Note that the server is written with security in mind, so many of those options may not be exactly interesting for you. If you are hosting a small server for your friends then the defaults should be good enough, however if you are hosting a big public server, it is recommended that you adjust the settings accordingly.


| JSON key | Default | Hot-reloadable<sup>**</sup> | Description |
|---------|---------|----------------|-------------|
| `web_mountpoint` | `"/"` | ❌ | HTTP mountpoint (the prefix before every endpoint) |
| `game_servers` | `[]` | ✅ | List of game servers that will be sent to the clients (see below for the format) |
| `maintenance` | `false` | ⏳ | When enabled, anyone trying to connect will get an appropriate error message saying that the server is under maintenance |
| `status_print_interval` | `7200` | ⚠️ | How often (in seconds) the game servers will print various status information to the console, 0 to disable |
| `userlist_mode` | `"none"` | ✅ | Can be `blacklist`, `whitelist`, `none`. See `userlist` property for more information |
| `tps` | `30` | ⏳ | Dictates how many packets per second clients can (and will) send when in a level. Higher = smoother experience but more processing power and bandwidth |
| `admin_webhook_url` | `""` | ⏳ | When enabled, admin actions (banning, muting, etc.) will send a message to the given discord webhook URL |
| `admin_key`<sup>*</sup> | `(random)` | ⏳ | The password used to unlock the admin panel in-game, must be 32 characters or less |
| `use_gd_api`<sup>*</sup> | `false` | ✅ | Use robtop's API to verify account ownership. Note that you must set `gd_api_credentials` accordingly if you enable this setting |
| `gd_api_account`<sup>*</sup> | `0` | ✅ | Account ID of a bot account that will be used to verify account ownership |
| `gd_api_gjp`<sup>*</sup> | `(empty)` | ✅ | GJP2 of the GD account used for verifying ownership. Figuring this out is left as an excercise to the reader :) |
| `secret_key`<sup>*</sup> | `(random)` | ❌ | Secret key for generating and verifying authentication keys |
| `secret_key2`<sup>*</sup> | `(random)` | ⏳ | Secret key for generating and verifying session tokens |
| `game_server_password`<sup>*</sup> | `(random)` | ✅ | Password used to authenticate game servers |
| `cloudflare_protection`<sup>*</sup> | `false` | ✅ | Block requests coming not from Cloudflare (see `central/src/allowed_ranges.txt`) and use `CF-Connecting-IP` header to distinguish users. If your server is proxied through cloudflare, you **must** turn on this option. |
| `challenge_expiry`<sup>*</sup> | `30` | ✅ | Amount of seconds before an authentication challenge expires and a new one can be requested |
| `token_expiry`<sup>*</sup> | `86400` (1 day) | ⏳ | Amount of seconds a session token will last. Those regenerate every time you restart the game, so it doesn't have to be long |

<sup>*</sup> - security setting, be careful with changing it if making a public server

<sup>**</sup> - meanings of different values in the Hot-reloadable column:

* ✅ - fully hot-reloadable, changes should apply immediately
* ⏳ - fully hot-reloadable, however this setting is synced with game servers, so you may have to wait a few minutes for them to update their configuration
* ⚠️ - partly hot-reloadable, the central server does **not** need to be restarted, but game servers do
* ❌ - not hot-reloadable, you must restart the central server to see changes

Formatting for game servers:

```json
{
    "id": "my-server-id",
    "name": "Server name",
    "address": "127.0.0.1:41001",
    "region": "my home i guess?"
}
```

Note that the `address` key must be the publicly visible IP address for other players to be able to connect, not a local address.

Additionally, when first starting up a server, a `Rocket.toml` file will be created from a template. By default, it will be put in the current working directory, or `ROCKET_CONFIG` if specified.

The `Rocket.toml` is primarily used for changing the HTTP port and the path to the database, however other settings are also free to change, as per [Rocket documentation](https://rocket.rs/v0.5/guide/configuration/#rockettoml). Changing the TOML configuration is as simple as the JSON configuration, however it is *not* hot-reloadable.

## Building

If you want to build the server yourself, you need a nightly Rust toolchain. After that, it's as simple as:
```sh
cd server/
rustup override set nightly # has to be done only once
cargo build --release
```

## Extra

In release builds, by default, the `Debug` and `Trace` log levels are disabled, so you will only see logs with levels `Info`, `Warn` and `Error`.

This can be changed by setting the environment variable `GLOBED_LOG_LEVEL` for the central server, or `GLOBED_GS_LOG_LEVEL` for the game server. The appropriate values are: `trace`, `debug`, `info`, `warn`, `error`, `none`.
