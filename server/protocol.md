## Protocol

if you somehow stumbled upon this file, hi! this is a brief protocol description so that I don't forget what everything does :p

plus sign means encrypted packet.

i will probably forget to update this very often

### Client

Connection related

* 10000 - PingPacket - ping
* 10001 - CryptoHandshakeStartPacket - handshake
* 10002 - KeepalivePacket - keepalive
* 10003+ - LoginPacket - authentication
* 10004 - DisconnectPacket - client disconnection

Game related

* 11000 - SyncIconsPacket - store client's icons
* 11001 - RequestProfilesPacket - request icons of other players (response 21000)
* 11002 - LevelJoinPacket - join a level
* 11003 - LevelJoinPacket - leave a level
* 11004 - PlayerDataPacket - player data
* 11005 - RequestPlayerListPacket - request list of all people on the game server (response 21002)
* 11006 - SyncPlayerMetadataPacket - request player metadata
* 11010+ - VoicePacket - voice frame
* 11011+ - ChatMessagePacket - chat message


### Server

Connection related

* 20000 - PingResponsePacket - ping response
* 20001 - CryptoHandshakeResponsePacket - handshake response
* 20002 - KeepaliveResponsePacket - keepalive response
* 20003 - ServerDisconnectPacket - server kicked you out
* 20004 - LoggedInPacket - successful auth
* 20005 - LoginFailedPacket - bad auth (has error message)
* 20006 - ServerNoticePacket - message popup for the user

Game related

* 21000 - PlayerProfilesPacket - list of players' names and icons
* 21001 - LevelDataPacket - level data
* 21002 - PlayerListPacket - list of all people on the game server
* 21003 - PlayerMetadataPacket - list of player metadata
* 21010+ - VoiceBroadcastPacket - voice frame from another user
* 21011+ - ChatMessageBroadcastPacket - chat message from another user