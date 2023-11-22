# Globed Server

Prebuilt server binaries are available for Linux (x64 and ARM64) and Windows (x64) in every GitHub release.

If you want to build it yourself, do note that the central server must be compiled with a nightly Rust toolchain, until `async fn` in traits becomes stable (which is set to be in Rust 1.75).

## Central server

The central server doesn't uses a single JSON file for configuration. By default, the file is put in `central-conf.json` in the current working directory when you run the server, but it can be overriden with the environment variable `GLOBED_CONFIG_PATH`. The path can be a folder or a full file path.

### Central server configuration

The central server allows for configuration hot reloading, so you can modify the file and see updates in real time without restarting the server.

Note that the server is written with security in mind, so many of those options may not be exactly interesting for you. If you are hosting a small server for your friends then the defaults should be good enough, however if you are hosting a big public server, it is recommended that you adjust the settings accordingly.


| JSON ID | Default | Hot-reloadable | Description |
|---------|---------|----------------|-------------|
| `web_mountpoint` | `"/"` | ❌ | HTTP mountpoint (the prefix before every endpoint) |
| `web_address` | `"0.0.0.0:41000"` | ❌ | HTTP address |
| `special_users` | `{}` | ✅<sup>**</sup> | Each entry has the account ID as the key and an object with properties `name` and `color` as the value. The `color` property is used for changing the color of the name for this user |
| `game_servers` | `[]` | ✅ | Each object has 4 keys: `id` (must be a unique string), `name`, `address` (in format `ip:port`), `region` |
| `userlist_mode` | `"none"` | ✅ | Can be `blacklist`, `whitelist`, `none`. See `userlist` property for more information |
| `userlist` | `[]` | ✅ | If `userlist_mode` is set to `blacklist`, block account IDs in this list. If set to `whitelist`, only the users in the list will be allowed access |
| `no_chat_list` | `[]` | ✅<sup>**</sup> | List of account IDs of users who are able to connect and play, but have cannot send text/voice messages |
| `use_gd_api`<sup>*</sup> | `false` | ✅ | Use robtop's API to verify account ownership. Note that you must set `challenge_level` accordingly if you enable this setting |
| `gd_api`<sup>*</sup> | `(...)` | ✅ | Link to robtop's API that will be used if `use_gd_api` is enabled. This setting is useful for GDPS owners |
| `gd_api_ratelimit`<sup>*</sup> | `5` | ❌ | If `use_gd_api` is enabled, sets the maximum request number per `gd_api_period` that can be made to robtop's API. Used to avoid ratelimits |
| `gd_api_period`<sup>*</sup> | `5` | ❌ | The period for `gd_api_ratelimit`, in seconds. For example if both settings are set to 5 (the default), only 5 requests can be made in 5 seconds |
| `secret_key`<sup>*</sup> | `(random)` | ❌ | Secret key for generating and verifying authentication keys |
| `game_server_password`<sup>*</sup> | `(random)` | ✅ | Password used to authenticate game servers |
| `cloudflare_protection`<sup>*</sup> | `false` | ✅ | Block requests coming not from Cloudflare (see `central/src/allowed_ranges.txt`) and use `CF-Connecting-IP` header to distinguish users |
| `challenge_expiry`<sup>*</sup> | `30` | ✅ | Amount of seconds before an authentication challenge expires and a new one can be requested |
| `challenge_level`<sup>*</sup> | `1` | ✅ | If `use_gd_api` is enabled, this must be set to a valid non-unlisted level ID that will be used for verification purposes |
| `challenge_ratelimit`<sup>*</sup> | `60` | ✅ | Amount of seconds to block the user for if they failed the authentication challenge |
| `token_expiry`<sup>*</sup> | `86400` (1 day) | ✅ | Amount of seconds a session token will last. Those regenerate every time you restart the game, so it doesn't have to be long |

<sup>*</sup> - security setting, be careful with changing it if making a public server

<sup>**</sup> - it may take a few minutes for you to see any changes

The minimum log level is `Warn` for other crates and `Trace` for the server itself in debug builds. In release builds, the default is `Info` and you can change it to `Warn` by defining the environment variable `GLOBED_LESS_LOG=1`

## Game

To bridge the servers together, you must use the password from the `game_server_password` option in the central server configuration. Then, you have 2 options whenever you start the server:

```sh
./globed-game-server 0.0.0.0:41001 http://127.0.0.1:41000 password

# or like this: (replace 'export' with 'set' on windows)

export GLOBED_GS_ADDRESS=0.0.0.0:41001
export GLOBED_GS_CENTRAL_URL=http://127.0.0.1:41000 
export GLOBED_GS_CENTRAL_PASSWORD=password
./globed-game-server
```

Replace `0.0.0.0:41001` with the address you want the game server to listen on, `http://127.0.0.1:41000` with the URL of your central server, and `password` with the password.

If you want to start the game server in a standalone manner, so that it doesn't need a central server to work, replace the URL with the string "none". Do keep in mind this disables player authentication and some other features, and is only recommended for testing.

The minimum log levels are the same as in the central server, except the environment variable to change them is `GLOBED_GS_LESS_LOG`.
