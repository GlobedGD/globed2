# Globed Server

## Central

The central server doesn't use any databases and uses a JSON file for configuration. By default, the configuration file is put in `central-conf.json` in the current working directory when you run the server, but it can be overriden with the environment variable `GLOBED_CONFIG_PATH`. The path can be a folder or a full file path.

### Central server configuration

The central server allows for configuration hot reloading, so you can modify the file and see updates in real time without restarting the server.

Note that the server is written with security in mind, so many of those options may not be exactly interesting for you. If you are hosting a small server for your friends then the defaults should be good enough, however if you are hosting a big public server, it is recommended that you adjust the settings accordingly.


| JSON ID | Default | Hot-reloadable | Description |
|---------|---------|----------------|-------------|
| `web_mountpoint` | `"/"` | ❌ | HTTP mountpoint (the prefix before every endpoint) |
| `web_address` | `"0.0.0.0:41000"` | ❌ | HTTP address |
| `special_users` | `{}` | ✅ | Each entry has the account ID as the key and an object with properties `name` and `color` as the value. The `color` property is used for changing the color of the name for this user |
| `game_servers` | `[]` | ✅ | Each object has 4 keys: `id` (must be a unique string), `name`, `address` (in format `ip:port`), `region` |
| `userlist_mode` | `"none"` | ✅ | Can be `blacklist`, `whitelist`, `none`. See `userlist` property for more information |
| `userlist` | `[]` | ✅ | If userlist mode is set to `blacklist`, block account IDs in this list. If set to `whitelist`, only the users in the list will be allowed access |
| `use_gd_api`<sup>*</sup> | `false` | ✅ | Use robtop's API to verify account ownership. Note that you must set `challenge_level` accordingly if you enable this setting |
| `gd_api`<sup>*</sup> | `(...)` | ✅ | Link to robtop's API that will be used if `use_gd_api` is enabled. This setting is useful for private servers |
| `secret_key`<sup>*</sup> | `(random)` | ❌ | Secret key for generating and verifying authentication codes |
| `game_server_password`<sup>*</sup> | `(random)` | ✅ | Password used to authenticate game servers |
| `cloudflare_protection`<sup>*</sup> | `false` | ✅ | Block requests coming not from Cloudflare (see `central/src/allowed_ranges.txt`) and use `CF-Connecting-IP` header to distinguish users |
| `challenge_expiry`<sup>*</sup> | `30` | ✅ | Amount of seconds before an authentication challenge expires and a new one can be requested |
| `challenge_level`<sup>*</sup> | `1` | ✅ | If `use_gd_api` is enabled, this must be set to a valid non-unlisted level ID that will be used for verification purposes |
| `challenge_ratelimit`<sup>*</sup> | `60` | ✅ | Amount of seconds to block the user for if they failed the authentication challenge |
| `token_expiry`<sup>*</sup> | `86400` (1 day) | ✅ | Amount of seconds a session token will last. Those regenerate every time you restart the game, so it doesn't have to be long |

<sup>*</sup> - security setting, be careful with changing it if making a public server

The minimum log level is `Warn` for other crates and `Trace` for the server itself in debug builds. In release builds, the default is `Info` and you can change it to `Warn` by defining the environment variable `GLOBED_LESS_LOG=1`

## Game

todo!

To bridge the servers together, you must set a password in the central server configuration file. After that you simply pass it as the environment variable `GLOBED_GS_CENTRAL_PASSWORD`. Once you boot up the game server it will try to make a request to the central server and tell you if the authentication succeeded or not.

The minimum log levels are the same as in the central server, except the environment variable to change them is `GLOBED_GS_LESS_LOG` 