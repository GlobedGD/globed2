# Globed Server

If you want to host a server yourself, first you have to download the server binaries from the latest GitHub release, named `globed-central-server` and `globed-game-server`. Depending on your OS and architecture, you want the one ending in `.exe` on Windows, the `-x64` one on Linux x64, and the `-arm64` one on Linux ARM64.

After that is done, you have 2 paths:

* If you want to setup a small, simple server you can jump to the [standalone section](#standalone)
* If you want to setup a bigger or more configurable server, keep reading.

First, you want to launch the central server binary. It should create a file called `central-conf.json` in the folder you ran it from. This is where you can configure everything about the server. For documentation about all the options, jump to the [configuration section](#central-server-configuration), however for now we only need the option `game_server_password`.

With your central server properly setup and started, jump to the [bridged](#bridged) section of the game server configuration and launch the game server, with the password that you configured earlier.

If you did everything right, you should see no errors or warnings in the console and instead you should see "Server launched on x.x.x.x". This means everything worked! Congrats :)

## Game server configuration

note: if you are not on Windows, in the following examples replace `set` with `export` and replace `globed-game-server.exe` with the appropriate server binary (such as `globed-game-server-x64`)

### Standalone

If you want to spin up a quick, standalone game server, without needing to start a central server, then it is as simple as running the `globed-game-server.exe` executable directly.

If you want to change the address then you'll have to run the executable with an additional argument like so:
```sh
# replace 0.0.0.0:41001 with your address
globed-game-server.exe 0.0.0.0:41001
```

To connect to your server, you want to use the Direct Connection option inside the server switcher in-game.

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

## Central server configuration

The central server allows configuration hot reloading, so you can modify the configuration file and see updates in real time without restarting the server.

By default, the file is created with the name `central-conf.json` in the current working directory when you run the server, but it can be overriden with the environment variable `GLOBED_CONFIG_PATH`. The path can be a folder or a full file path.


Note that the server is written with security in mind, so many of those options may not be exactly interesting for you. If you are hosting a small server for your friends then the defaults should be good enough, however if you are hosting a big public server, it is recommended that you adjust the settings accordingly.


| JSON key | Default | Hot-reloadable<sup>**</sup> | Description |
|---------|---------|----------------|-------------|
| `web_mountpoint` | `"/"` | ❌ | HTTP mountpoint (the prefix before every endpoint) |
| `web_address` | `"0.0.0.0:41000"` | ❌ | HTTP address |
| `special_users` | `{}` | ⏳ | List of users that have special properties, for example a unique name color (see below for the format) |
| `game_servers` | `[]` | ✅ | List of game servers that will be sent to the clients (see below for the format) |
| `maintenance` | `false` | ⏳ | When enabled, anyone trying to connect will get an appropriate error message saying that the server is under maintenance |
| `userlist_mode` | `"none"` | ✅ | Can be `blacklist`, `whitelist`, `none`. See `userlist` property for more information |
| `userlist` | `[]` | ✅ | If `userlist_mode` is set to `blacklist`, block account IDs in this list. If set to `whitelist`, only the users in the list will be allowed to connect |
| `no_chat_list` | `[]` | ⏳ | List of account IDs of users who are able to connect and play, but have cannot send text/voice messages |
| `tps` | `30` | ⏳ | Dictates how many packets per second clients can (and will) send when in a level. Higher = smoother experience but more processing power and bandwidth |
| `admin_key`<sup>*</sup> | `(random)` | ⏳ | The password used to unlock the admin panel in-game, must be 16 characters or less |
| `use_gd_api`<sup>*</sup> | `false` | ✅ | Use robtop's API to verify account ownership. Note that you must set `challenge_level` accordingly if you enable this setting |
| `gd_api`<sup>*</sup> | `(...)` | ✅ | Link to robtop's API that will be used if `use_gd_api` is enabled. This setting is useful for GDPS owners |
| `gd_api_ratelimit`<sup>*</sup> | `5` | ❌ | If `use_gd_api` is enabled, sets the maximum request number per `gd_api_period` that can be made to robtop's API. Used to avoid ratelimits |
| `gd_api_period`<sup>*</sup> | `5` | ❌ | The period for `gd_api_ratelimit`, in seconds. For example if both settings are set to 5 (the default), only 5 requests can be made in 5 seconds |
| `secret_key`<sup>*</sup> | `(random)` | ❌ | Secret key for generating and verifying authentication keys |
| `secret_key2`<sup>*</sup> | `(random)` | ❌ | Secret key for generating and verifying session tokens |
| `game_server_password`<sup>*</sup> | `(random)` | ✅ | Password used to authenticate game servers |
| `cloudflare_protection`<sup>*</sup> | `false` | ✅ | Block requests coming not from Cloudflare (see `central/src/allowed_ranges.txt`) and use `CF-Connecting-IP` header to distinguish users |
| `challenge_expiry`<sup>*</sup> | `30` | ✅ | Amount of seconds before an authentication challenge expires and a new one can be requested |
| `challenge_level`<sup>*</sup> | `1` | ✅ | If `use_gd_api` is enabled, this must be set to a valid non-unlisted level ID that will be used for verification purposes |
| `challenge_ratelimit`<sup>*</sup> | `60` | ✅ | Amount of seconds to block the user for if they failed the authentication challenge |
| `token_expiry`<sup>*</sup> | `86400` (1 day) | ⚠️ | Amount of seconds a session token will last. Those regenerate every time you restart the game, so it doesn't have to be long |

<sup>*</sup> - security setting, be careful with changing it if making a public server

<sup>**</sup> - meanings of different values in the Hot-reloadable column:

* ✅ - fully hot-reloadable, changes should apply immediately
* ⏳ - fully hot-reloadable, however this setting is synced with game servers, so you may have to wait a few minutes for them to update their configuration
* ⚠️ - partly hot-reloadable, the central server does **not** need to be restarted, but game servers do
* ❌ - not hot-reloadable, you must restart the central server to see changes

Formatting for special users:

```json
{
    "123123": {
        "name": "myname",
        "color": "#ff0000",
    }
}
```

Formatting for game servers:

```json
{
    "id": "my-server-id",
    "name": "Server name",
    "address": "127.0.0.1:41001",
    "region": "my home i guess?"
}
```

## Building

If you want to build the server yourself, you need a nightly Rust toolchain. After that, it's as simple as:
```sh
cd server/
rustup override set nightly # has to be done only once
cargo build --release
```

## Extra

In release builds, the `Debug` and `Trace` levels are disabled, so you will only see logs with levels `Info`, `Warn` and `Error`.

This can be changed by setting the environment variable `GLOBED_LESS_LOG=1` for the central server, or `GLOBED_GS_LESS_LOG=1` for the game server. With this option, only logs with levels `Warn` and `Error` will be printed.
