# Globed Server

## Prerequisites

Before trying to setup a server, it is **recommended** that you understand what it involves. You will likely need to either setup port forwarding or use a VPN tool like Radmin VPN if you are hosting the server on your PC, and that won't be covered in this guide.

In case you are familiar with [Pterodactyl](https://pterodactyl.io/), there are eggs available for the [central](https://github.com/DumbCaveSpider/globed-central-egg) and [game](https://github.com/DumbCaveSpider/globed-game-egg) servers that could simplify the setup (thanks to [@DumbCaveSpider](https://github.com/DumbCaveSpider/))

Additionally, if you are setting up a public server, please keep in mind that there are no stability guarantees. Big changes to the server can be made at any time, and updates to the mod can cause your server to stop accepting users until you update it.

## Setup

If you want to host a server yourself, first you have to download the server binaries from the [latest GitHub release](https://github.com/dankmeme01/globed2/releases/latest), named `globed-central-server` and `globed-game-server`. Depending on your OS and architecture, you want the one ending in `.exe` on Windows, the `-x64` one on Linux x64, and the `-arm64` one on Linux ARM64.

After that is done, you have 2 paths:

* If you want to setup a small, simple server you can jump to the [standalone section](#standalone)
* If you want to setup a bigger or more configurable server, keep reading.

After launching the central server binary, you should see two new files called `central-conf.json` and `Rocket.toml`. This is where you can configure everything about the server. For documentation about all the options, jump to the [configuration section](#central-server-configuration), however for now we only need the option `game_server_password`.

With your central server properly setup and started, jump to the [bridged](#bridged) section of the game server configuration and launch the game server, with the password that you configured earlier.

If you did everything right, you should see no errors or warnings in the console and instead you should see "Server launched on x.x.x.x". This means everything worked! Congrats :)

## Game server configuration

note: if you are not on Windows, in the following examples replace `set` with `export` and replace `globed-game-server.exe` with the appropriate server binary (such as `globed-game-server-x64`)

### Standalone

If you want to spin up a quick, standalone game server, without needing to start a central server, then it is as simple as running the `globed-game-server.exe` executable directly.

If you want to change the port (default is 4202) then you'll have to run the executable with an additional argument like so:
```sh
# replace 4202 with your desired port
globed-game-server.exe 0.0.0.0:4202
```

**To connect to your server, you want to use the Direct Connection option inside the server switcher in-game**. (with the address `127.0.0.1:4202` if the server is running on the same device)

Keep in mind that a standalone server makes the configuration very limited (for example you can't ban/mute users anymore) and disables any kind of player authentication.


### Bridged

To start the game server and bridge it together with an active central server you must use the password from the `game_server_password` option in the central server configuration. Then, you have 2 options whenever you start the server:

```sh
globed-game-server.exe 0.0.0.0:4202 http://127.0.0.1:4201 password

# or like this:

set GLOBED_GS_ADDRESS=0.0.0.0:4202
set GLOBED_GS_CENTRAL_URL=http://127.0.0.1:4201
set GLOBED_GS_CENTRAL_PASSWORD=password
globed-game-server.exe
```

Replace `0.0.0.0:4202` with the address you want the game server to listen on, `http://127.0.0.1:4201` with the URL of your central server, and `password` with the password.

### Environment variables

`GLOBED_GS_NO_FILE_LOG` - if set to 1, don't create a log file and only log to the console.

## Central server configuration

By default, the file is created with the name `central-conf.json` in the current working directory when you run the server, but it can be overriden with the environment variable `GLOBED_CONFIG_PATH`. The path can be a folder or a full file path.

### General settings
| JSON key | Default | Description |
|---------|---------|-----------------|
| `web_mountpoint` | `"/"` | HTTP mountpoint (the prefix before every endpoint) |
| `game_servers` | `[]` | List of game servers that will be sent to the clients (see below for the format) |
| `maintenance` | `false` | When enabled, anyone trying to connect will get an appropriate error message saying that the server is under maintenance |
| `status_print_interval` | `7200` | How often (in seconds) the game servers will print various status information to the console, 0 to disable |
| `userlist_mode` | `"none"` | Can be `blacklist`, `whitelist`, `none` (same as `blacklist`). When set to `whitelist`, players will need to be first whitelisted before being able to join |
| `tps` | `30` | Dictates how many packets per second clients can (and will) send when in a level. Higher = smoother experience but more processing power and bandwidth |
| `admin_webhook_url` | `(empty)` | When enabled, admin actions (banning, muting, etc.) will send a message to the given discord webhook URL |
| `chat_burst_limit` | `0` | Controls the amount of text chat messages users can send in a specific period of time, before getting rate limited. 0 to disable |
| `chat_burst_interval` | `0` | Controls the period of time for the `chat_burst_limit_setting`. Time is in milliseconds |
| `roles` | `(...)` | Controls the roles available on the server (moderator, admin, etc.), their permissions, name colors, and various other things |

### Security settings (the boring stuff)

These are recommended to adjust if you're hosting a public server, otherwise the defaults should be fine.

| JSON key | Default | Description |
|---------|---------|-----------------|
| `admin_key` | `(random)` | The password used to unlock the admin panel in-game, must be 32 characters or less |
| `use_gd_api` | `false` | Verify account ownership via requests to GD servers. Note that you must set `gd_api_account` and `gd_api_gjp` accordingly if you enable this setting |
| `gd_api_account` | `0` | Account ID of a bot account that will be used to verify account ownership |
| `gd_api_gjp` | `(empty)` | GJP2 of the GD account used for verifying ownership. Figuring this out is left as an excercise to the reader :) |
| `gd_api_url` | `(...)` | Base link to the GD API used for account verification. By default is `https://www.boomlings.com/database`. Change this if you're hosting a server for a GDPS |
| `skip_name_check` | `false` | Skips validation of account names when verifying accounts |
| `refresh_interval` | `3000` | Controls the time (in milliseconds) between requests to the GD server for refreshing messages |
| `secret_key` | `(random)` | Secret key for signing authentication keys |
| `secret_key2` | `(random)` | Secret key for signing session tokens |
| `game_server_password` | `(random)` | Password used to authenticate game servers |
| `cloudflare_protection` | `false` | Block requests coming not from Cloudflare (see `central/src/allowed_ranges.txt`) and use `CF-Connecting-IP` header to distinguish users. If your server is proxied through cloudflare, you **must** turn on this option. |
| `challenge_expiry` | `30` | Amount of seconds before an authentication challenge expires and a new one can be requested |
| `token_expiry` | `86400` (1 day) | Amount of seconds a session token will last. Those regenerate every time you restart the game, so it doesn't have to be long |

Formatting for game servers:

```json
{
    "id": "my-server-id",
    "name": "Server name",
    "address": "127.0.0.1:4202",
    "region": "my home i guess?"
}
```

**Note that the `address` key must be a public IP address if you want others to be able to connect. Putting 127.0.0.1 will make it possible to only connect from *your* machine.**

Formatting for user roles:

```json
{
    // all keys except id and priority are optional.

    "id": "mod",
    "priority": 100, // determines which roles can edit users with other roles
    "badge_icon": "role-mod.png", // make sure it's a valid sprite! (can be empty)
    "name_color": "#ff0000", // name color
    "chat_color": "#ff0000", // color of chat messages

    // permissions
    "notices": false, // ability to send notices (popup messages)
    "notices_to_everyone": false, // ability to send a notice to everyone on the server
    "kick": false, // ability to disconnect users from the server
    "kick_everyone": false, // ability to disconnect everyone from the server
    "mute": false, // ability to mute/unmute
    "ban": false, // ability to ban/unban & whitelist (on whitelist enabled servers)
    "edit_role": false, // ability to change roles of a user
    "admin": false, // implicitly enables all other permissions and also does some additional things
}
```

There is also a special format for tinting colors, for example setting `name_color` to `#ff0000 > 00ff00 > 0000ff` would make your name fade between red, green and blue. Spaces and a `#` at the start are for clarity and are optional. (Maximum 8 colors supported in one string)

### Rocket.toml

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
