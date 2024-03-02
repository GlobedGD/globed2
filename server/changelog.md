# server changelog

least chaotic thing about this repo tbh

## v1.2.1

* protocol bumped to v4
* added a discord webhook configuration option

## v1.2.0

* switched to a different web framework (Rocket), now there is an additional Rocket.toml file with configuration
* added an sqlite database for persistent storage, `special_users` and `userlist` keys in the configuration are now replaced by that
* added a lot of admin abilities
* fixed a loophole allowing account impersonation
* clarified some error messages

## game v1.1.2

* added a TCP keepalive packet.
* added ability to kick everyone on the server in the admin panel (literally type `@everyone` as the name)

## central v1.1.1

* fix (?) the configuration file not being created and reword some error messages

## game v1.1.0

Protocol version is bumped to v2, so people on older versions of the mod won't be able to connect anymore.

The game server now no longer uses only UDP, and instead uses a hybrid of TCP and UDP (on the same port). This means that if you used to do port forwarding, you now need to forward the port for both TCP and UDP.

## central v1.1.0

* bumped the protocol version to v2 to be compatible with the game server update.
* change the way account verification is done, removed some of the related settings and added new ones.
* allow writing comments in the JSON configuration file

## game v1.0.2

* add two new packets: `ConnectionTestPacket` and `ConnectionTestResponsePacket`. used for testing the connection.

## central v1.0.1

* change the template game server ip address from `127.0.0.0` to `127.0.0.1` (oops)

## game v1.0.1

* don't include unauthenticated players in the room packet
