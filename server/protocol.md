## Protocol

if you somehow stumbled upon this file, hi! this is a brief protocol description so that I don't forget what everything does :p

`+` - encrypted packet.

`!` - this is just a planned packet and there is no existing implementation, at all.

`?` - the handler needs to be looked into because some things were changed elsewhere that broke it

`&` - structure exists but no implementation or handling

`^` - handled server-side but no client implementation

i will probably forget to update this very often

rn all question marks are regarding rooms

### Client

Connection related

* 10000 - PingPacket - ping
* 10001 - CryptoHandshakeStartPacket - handshake
* 10002 - KeepalivePacket - keepalive
* 10003+ - LoginPacket - authentication
* 10004 - DisconnectPacket - client disconnection

General

* 11000^ - SyncIconsPacket - store client's icons
* 11001 - RequestGlobalPlayerListPacket - request list of all people in the server (response 21000)
* 11002^ - CreateRoomPacket - create a room
* 11003^ - JoinRoomPacket - join a room
* 11004^ - LeaveRoomPacket - leave a room (no need for a response)
* 11005 - RequestRoomPlayerListPacket - request list of all people in the given room (response 21004)

Game related

* 12000 - RequestPlayerProfilesPacket - request account data of another player (or all people on the level)
* 12001 - LevelJoinPacket - join a level
* 12002 - LevelLeavePacket - leave a level
* 12003 - PlayerDataPacket - player data
* 12010+ - VoicePacket - voice frame
* 12011?^+ - ChatMessagePacket - chat message


### Server

Connection related

* 20000 - PingResponsePacket - ping response
* 20001 - CryptoHandshakeResponsePacket - handshake response
* 20002 - KeepaliveResponsePacket - keepalive response
* 20003 - ServerDisconnectPacket - server kicked you out
* 20004 - LoggedInPacket - successful auth
* 20005 - LoginFailedPacket - bad auth (has error message)
* 20006 - ServerNoticePacket - message popup for the user

General

* 21000 - GlobalPlayerListPacket - list of people in the server
* 21001^ - RoomCreatedPacket - returns room id (returns existing one if already in a room)
* 21002^ - RoomJoinedPacket - returns nothing ig?? just indicates success
* 21003^ - RoomJoinFailedPacket - also nothing, the only possible error is no such room id exists
* 21004^ - RoomPlayerListPacket - list of people in the room

Game related

* 22000 - PlayerProfilesPacket - list of requested profiles
* 22001 - LevelDataPacket - level data
* 22010+ - VoiceBroadcastPacket - voice frame from another user
* 22011+ - ChatMessageBroadcastPacket - chat message from another user