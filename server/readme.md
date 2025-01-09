# Globed Server Setup Guide

## Prerequisites

Before setting up the server, it is **recommended** that you understand what it entails. You may need to set up port forwarding or use a VPN tool like Radmin VPN if hosting the server on your PC. These steps are not covered in this guide.

If you're familiar with [Pterodactyl](https://pterodactyl.io/), there are eggs available for both the [central](https://github.com/DumbCaveSpider/globed-central-egg) and [game](https://github.com/DumbCaveSpider/globed-game-egg) servers that can simplify the setup (thanks to [@DumbCaveSpider](https://github.com/DumbCaveSpider)).

Please note, if you're setting up a public server, there are no guarantees regarding stability. The server may undergo significant changes at any time, and updates to the mod can cause your server to stop accepting users until it is updated.

## Setup

To host a server yourself, follow these steps:

1. **Download the Server Binaries:**  
   Download the server binaries from the [latest GitHub release](https://github.com/GlobedGD/globed2/releases/latest), which include `globed-central-server` and `globed-game-server`. Choose the appropriate file based on your operating system:
   - For **Windows**, download the `.exe` file.
   - For **Linux x64**, download the `-x64` file.
   - For **Linux ARM64**, download the `-arm64` file.

2. **Choose Your Setup Path:**
   - If you want a small, simple server, proceed to the [Standalone section](#standalone).
   - If you're aiming for a larger or more configurable setup, continue reading.

3. **Configure the Central Server:**
   Launch the `central-server` binary. Afterward, you should see two new files: `central-conf.json` and `Rocket.toml`. These files contain the server's configuration. For now, focus on the `game_server_password` option.

4. **Launch the Game Server:**
   With the central server configured, proceed to the [Bridged section](#bridged) of the game server configuration and launch the game server using the password set earlier.

If everything is done correctly, the console should show "Server launched on x.x.x.x", indicating a successful setup.

## Game Server Configuration

**Note:** If you're not on Windows, replace `set` with `export` and use the appropriate server binary (e.g., `globed-game-server-x64` for Linux).

### Standalone

To quickly launch a game server without the central server, run the `globed-game-server.exe` executable directly.

To change the port (default is 4202), use the following command:
```sh
# Replace 4202 with your desired port
globed-game-server.exe 0.0.0.0:4202
```

**Connecting to the server:**  
Use the **Direct Connection** option in the game’s server switcher, with the address `127.0.0.1:4202` if the server is running on the same machine.

Keep in mind, a standalone server offers limited configuration (e.g., you can't ban or mute users) and disables player authentication.

### Bridged

To bridge the game server with a central server, use the `game_server_password` from the central server configuration. You have two options to start the server:

```sh
globed-game-server.exe 0.0.0.0:4202 http://127.0.0.1:4201 password
```

Or set environment variables and then run the server:
```sh
set GLOBED_GS_ADDRESS=0.0.0.0:4202
set GLOBED_GS_CENTRAL_URL=http://127.0.0.1:4201
set GLOBED_GS_CENTRAL_PASSWORD=password
globed-game-server.exe
```

Replace:
- `0.0.0.0:4202` with your desired game server address.
- `http://127.0.0.1:4201` with the URL of your central server.
- `password` with your configured password.

### Environment Variables

- `GLOBED_GS_NO_FILE_LOG` - Set to `1` to prevent the creation of a log file, logging only to the console.

## Central Server Configuration

By default, the `central-conf.json` file is created in the current directory when you run the central server, but it can be overridden using the `GLOBED_CONFIG_PATH` environment variable.

### General Settings

| JSON Key                          | Default       | Description |
|------------------------------------|---------------|-------------|
| `web_mountpoint`                   | `"/"`         | HTTP mountpoint (prefix before every endpoint) |
| `game_servers`                     | `[]`          | List of game servers sent to clients (see below for format) |
| `maintenance`                      | `false`       | If enabled, users trying to connect will receive a maintenance message |
| `status_print_interval`            | `7200`        | Time in seconds between status messages printed to the console (set to `0` to disable) |
| `userlist_mode`                    | `"none"`      | Options: `blacklist`, `whitelist`, `none` (whitelist requires players to be whitelisted to join) |
| `tps`                              | `30`          | Number of packets per second clients can send in a level (higher = smoother experience) |
| `admin_webhook_url`                | `(empty)`     | URL for sending admin action notifications (banning, muting, etc.) to Discord |
| `rate_suggestion_webhook_url`      | `(empty)`     | URL for sending level submission notifications to Discord |
| `featured_webhook_url`             | `(empty)`     | URL for sending level feature notifications to Discord |
| `room_webhook_url`                 | `(empty)`     | URL for sending room creation notifications to Discord |
| `chat_burst_limit`                 | `0`           | Limit on the number of text messages a user can send in a period (set to `0` to disable) |
| `chat_burst_interval`              | `0`           | Time period for chat burst limit (in milliseconds) |
| `roles`                            | `(...)`       | List of available roles, with permissions and settings |

### Security Settings (Recommended for Public Servers)

| JSON Key                          | Default       | Description |
|------------------------------------|---------------|-------------|
| `admin_key`                        | `(random)`    | Password for unlocking the admin panel (max 32 characters) |
| `use_gd_api`                       | `false`       | Verify account ownership via GD servers (requires `gd_api_account` and `gd_api_gjp`) |
| `gd_api_account`                   | `0`           | Bot account ID for verification |
| `gd_api_gjp`                       | `(empty)`     | GJP2 for account verification |
| `gd_api_url`                       | `(...)`       | Base URL for GD API used for account verification |
| `skip_name_check`                  | `false`       | Skips account name validation when verifying accounts |
| `refresh_interval`                 | `3000`        | Time in milliseconds between GD server requests for refreshing messages |
| `secret_key`                       | `(random)`    | Secret key for signing authentication keys |
| `secret_key2`                      | `(random)`    | Secret key for signing session tokens |
| `game_server_password`             | `(random)`    | Password for authenticating game servers |
| `cloudflare_protection`            | `false`       | Block non-Cloudflare requests and use the `CF-Connecting-IP` header if server is proxied through Cloudflare |
| `challenge_expiry`                 | `30`          | Time (in seconds) before an authentication challenge expires |
| `token_expiry`                     | `86400` (1 day) | Time (in seconds) a session token lasts |

### Game Server Formatting Example:

```json
{
    "id": "my-server-id",
    "name": "Server name",
    "address": "127.0.0.1:4202",
    "region": "my home i guess?"
}
```

**Note:** The `address` key must be a public IP address if you want others to connect. Use `127.0.0.1` for local connections only.

### User Role Formatting Example:

```json
{
    "id": "mod",                    // Required
    "priority": 100,                // Required, determines which roles can edit users with other roles
    "badge_icon": "role-mod.png",   // Optional, make sure it's a valid sprite
    "name_color": "#ff0000",        // Optional, name color
    "chat_color": "#ff0000",        // Optional, color of chat messages
    "notices": false,               // Optional, ability to send notices (popup messages)
    "notices_to_everyone": false,   // Optional, ability to send a notice to everyone on the server
    "kick": false,                  // Optional, ability to kick users from the server
    "kick_everyone": false,         // Optional, ability to kick everyone from the server
    "mute": false,                  // Optional, ability to mute/unmute
    "ban": false,                   // Optional, ability to ban/unban & whitelist (on whitelist enabled servers)
    "edit_role": false,             // Optional, ability to change roles of a user
    "edit_featured_levels": false,  // Optional, ability to edit featured levels
    "admin": false                  // Optional, implicitly enables all other permissions
}
```

### Special Role Tinting:

You can create color transitions for names by using a special format, e.g., setting `name_color` to `#ff0000 > 00ff00 > 0000ff` will make the name fade between red, green, and blue.

Keep in mind that there is a limit of 8 colors.

### Rocket.toml Configuration

When starting the server, a `Rocket.toml` file is generated, typically in the current working directory (or the directory specified by `ROCKET_CONFIG`). This file is used for configuring the HTTP port and database path, among other settings. Refer to [Rocket documentation](https://rocket.rs/guide/v0.5/configuration/#rocket-toml) for further configuration options.

## Building the Server

To build the server yourself, you will need a nightly Rust toolchain. Once set up, use the following commands to build:

```sh
cd server/
rustup override set nightly
cargo build --release
```

## Extra Configuration

In release builds, the default log levels are set to `Info`, `Warn`, and `Error`. To adjust the log level, set the environment variable `GLOBED_LOG_LEVEL` for the central server or `GLOBED_GS_LOG_LEVEL` for the game server. This variable defines the **minimum** log level, meaning that setting it to `Trace` will enable all log levels, from `Trace` up to `Error`. 

The possible log levels are:
- `Trace` (shows all log messages, including `Trace`, `Debug`, `Info`, `Warn`, and `Error`)
- `Debug`
- `Info`
- `Warn`
- `Error`
- `None` (disables all logs)

To disable logging to a file, set `GLOBED_NO_FILE_LOG` to a nonzero value.
