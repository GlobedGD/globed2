# Globed Server

## Central

The central server doesn't use any databases and uses a JSON file for configuration. By default, the configuration file is put in `central-conf.json` in the current working directory when you run the server, but it can be overriden with the environment variable `GLOBED_CONFIG_PATH`. The path can be a folder or a full file path.

### Central server configuration

The central server allows for configuration hot reloading, so you can modify the config and see updates in real time without restarting the server. Some of the values (like IP addresses, secret key) will only apply on the next restart.

The following is a simple configuration template with descriptions of all options. Don't worry about creating it if you haven't already, as it will be created automatically if it doesn't already exist when you start the server.

```js
{
    "web_mountpoint": "/", // server HTTP mountpoint
    "web_address": "0.0.0.0:41000", // server address
    // security related
    "secret_key": "random", // secret key for auth codes, keep it secure and don't use the default value.
    "challenge_expiry": 60, // amount of seconds before an auth challenge expires
    "challenge_level": 1, // level ID for challenges, must be a valid non-unlisted level
}
```


## Game

todo!