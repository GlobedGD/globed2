# Event encoding

This document defines various parts related to event encoding.

## Type ranges

- `[0; 0xf000)` - custom, undefined types. Levels are free to use these for custom scripted events.
- `[0xf000; 0xffff]` - internal globed types. Do not manually invoke these.

# Events

## EVENT_COUNTER_CHANGE (0xf001)

## EVENT_PLAYER_JOIN (0xf002)

## EVENT_PLAYER_LEAVE (0xf003)

## EVENT_SCR_SPAWN_GROUP (0xf010)

## EVENT_SCR_SET_ITEM (0xf011)

## EVENT_SCR_REQUEST_SCRIPT_LOGS (0xf012)

## EVENT_2P_LINK_REQUEST (0xf100)

## EVENT_2P_UNLINK (0xf101)
