# Globed Soft Link API

Soft link API is a way to interact with globed in a way that does not require linking to the mod (this means, Globed does not have to be a dependency of your mod at all). The way it works is your mod requests a function table using Geode events, and then caches it and uses that table to call various Globed functions. This is less efficient, less powerful and more tedious than the [Link API](./link-api.md) but a huge upside is that you aren't forcing users of your mod to have Globed enabled.

## Setup

Add Globed as a dependency in your mod's `mod.json`:

```json
{
    "dependencies": {
        "dankmeme.globed2": {
            "importance": "suggested",
            "version": ">=2.0.0"
        }
    }
}
```

## Usage

TODO, this is a lot of work oh god
