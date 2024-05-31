## Protocol

if you somehow stumbled upon this file, hi! this is a brief protocol description so that I don't forget what everything does :p

`+` - encrypted packet.

`!` - this packet is unused and may be removed completely in the future

`^` - this packet is not fully functional and work needs to be done on either the client side or the server side

i will probably forget to update this very often

### Client

Connection related

* 10000 - PingPacket - ping
* 10001 - CryptoHandshakeStartPacket - handshake
* 10002 - KeepalivePacket - keepalive
* 10003+ - LoginPacket - authentication
* 10004 - LoginRecoverPacket - recover a disconnected session
* 10005 - ClaimThreadPacket - claim a tcp thread from a udp connection
* 10006 - DisconnectPacket - client disconnection
* 10007 - KeepaliveTCPPacket - keepalive but for the tcp connection
* 10200 - ConnectionTestPacket - connection test (response 20010)

General

* 11000 - SyncIconsPacket - store client's icons
* 11001 - RequestGlobalPlayerListPacket - request list of all people in the server (response 21000)
* 11002 - RequestLevelListPacket - request list of all levels people are playing right now (response 21005)
* 11003 - RequestPlayerCountPacket - request amount of people on up to 128 different levels (response 21006)

Game related

* 12000 - RequestPlayerProfilesPacket - request account data of another player (or all people on the level)
* 12001 - LevelJoinPacket - join a level
* 12002 - LevelLeavePacket - leave a level
* 12003 - PlayerDataPacket - player data
* 12004 - PlayerMetadataPacket - player metadata
* 12010+ - VoicePacket - voice frame
* 12011^+ - ChatMessagePacket - chat message

Room related

* 13000 - CreateRoomPacket - create a room
* 13001 - JoinRoomPacket - join a room
* 13002 - LeaveRoomPacket - leave a room (no need for a response)
* 13003 - RequestRoomPlayerListPacket - request list of all people in the given room (response 21004)
* 13004 - UpdateRoomSettingsPacket - update the settings of a room
* 13005 - RoomSendInvitePacket - send invite to a room
* 13006 - RequestRoomListPacket - request a list of all public rooms

Admin related

* 19000+ - AdminAuthPacket - admin auth
* 19001+ - AdminSendNoticePacket - send notice to everyone or a specific connected person
* 19002 - AdminDisconnectPacket - disconnect a user with a specific message
* 19003 - AdminGetUserStatePacket - get user state
* 19004+ - AdminUpdateUserPacket - mute/ban/whitelist a user, etc.

### Server

Connection related

* 20000 - PingResponsePacket - ping response
* 20001 - CryptoHandshakeResponsePacket - handshake response
* 20002 - KeepaliveResponsePacket - keepalive response
* 20003 - ServerDisconnectPacket - server kicked you out
* 20004 - LoggedInPacket - successful auth
* 20005 - LoginFailedPacket - bad auth (has error message)
* 20006 - ProtocolMismatchPacket - protocol version mismatch
* 20007 - KeepaliveTCPResponsePacket - keepalive response but for tcp
* 20008 - ClaimThreadFailedPacket - failed to claim thread
* 20009 - LoginRecoveryFailedPacket - failed to recover session
* 20100 - ServerNoticePacket - message popup for the user
* 20101 - ServerBannedPacket - message about being banned
* 20102 - ServerMutedPacket - message about being muted
* 20200 - ConnectionTestResponsePacket - connection test response

General

* 21000! - GlobalPlayerListPacket - list of people in the server
* 21001 - LevelListPacket - list of all levels in the room
* 21002 - LevelPlayerCountPacket - amount of players on certain requested levels

Game related

* 22000 - PlayerProfilesPacket - list of requested profiles
* 22001 - LevelDataPacket - level data
* 22002 - LevelPlayerMetadataPacket - metadata of other players
* 22010+ - VoiceBroadcastPacket - voice frame from another user
* 22011+ - ChatMessageBroadcastPacket - chat message from another user

Room related

* 23000 - RoomCreatedPacket - returns room id (returns existing one if already in a room)
* 23001 - RoomJoinedPacket - returns nothing ig?? just indicates success
* 23002 - RoomJoinFailedPacket - also nothing, the only possible error is no such room id exists
* 23003 - RoomPlayerListPacket - list of people in the room
* 23004 - RoomInfoPacket - settings updated and stuff
* 23005 - RoomInvitePacket - invite from another player
* 23006 - RoomListPacket - list of all public rooms

Admin related

* 29000 - AdminAuthSuccessPacket - admin auth successful
* 29001+ - AdminErrorPacket - error happened when doing an admin action
* 29002+ - AdminUserDataPacket - data about the player
* 29003+ - AdminSuccessMessagePacket - small success message about an action
* 29004 - AdminAuthFailedPacket - admin auth failed