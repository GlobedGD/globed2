### 1.7.2
- Corrected texture pack detection algorithm
- Fixed rare crash when viewing changelog popup
- Fixed rare crash caused by a race condition

### v1.7.1
- Fixed broken UI in room password popup

### v1.7.0
- Ported to Geode v4.0.0
- Added **2-player mode room setting**, enabling remote 2-player levels
- Added **level filtering** in the Globed level list
- Introduced compatibility for **outdated mod versions and game versions**
- Allowed **64 characters** in the direct connection popup address
- Added GitHub guides (currently about launch options)
- Added **cheat indicator** in the room creation UI to highlight safe mode settings
- Enhanced error messages for failed web requests
- Improved UI sliders (volume, etc.)
- Enhanced server security & performance
- Revamped packet fragmentation algorithm
- Revamped moderator panel
- Added ability for GDPS owners to override main server URL with several fallbacks
- Fixed crash when joining a room with an unlisted/deleted level
- Fixed minor memory leak when players leave levels
- Fixed crashes with outdated texture packs modifying Globed textures
- Fixed audio recording system logic issues
- Fixed rare crash during game closing
- Fixed multithreading crash when loading into the main menu

### v1.6.2
- Fixed crashes on some Windows devices

### v1.6.1
- Fixed crash when pressing any key or opening the Globed menu on Mac
- Fixed player list layout issues after leaving a room with a pinned level
- Enabled faster reset for all players in Death Link if the room creator enables it
- Added **verification for Discord linking**
- Showed only 1 badge near a player's name in-game

### v1.6.0
- Added ability to **pin specific levels** in a room
  - Room creators can now pin levels at the top of the player list in the Globed menu
- Added **changelog popup** showing the latest update details
- Replaced invisibility button with advanced options
  - Option to hide yourself from player list, level, or invite list
- Introduced new **Discord bot** for supporters to automatically obtain in-game roles
- Displayed all badges of a player (e.g., **Mod & Booster**)
- Enforced faster reset option in **Death Link** rooms
- Automatically **blocked invites** from blocked users
- Changed player list sorting to **random order** to prevent alt account promotion
- Added **Force Progress Bar** setting for server-connected progress bar
- Removed **Custom Keybinds** dependency from Android
- Improved security

Bugfixes:
- Fixed login issues (long login time and occasional failures)
- Fixed player names/status not rotating with camera
  - Option to disable name rotation available
- Fixed crash when closing the Globed menu quickly
- Fixed crash when muting players who left the level
- Fixed room name overlapping UI in room listing
- Fixed player count capping at 10000 instead of 65535
- Fixed crash with **Force Platformer** enabled in Megahack
- Fixed room listing closing when joining a room

### v1.5.1
- Fixed Globed not loading on Android due to missing **Custom Keybinds** dependency

### v1.5.0
- Added **Death Link** setting to rooms, causing all players to die when one dies
- Added ability for room owners to close rooms (kicks all players)
- Changed default **Receive Invites From** setting to "friends only"
- Fixed player not disconnecting from server when logging out of GD account
- Fixed crash when closing the featured level list too quickly
- Fixed error popups on self-hosted servers when no featured level is selected
- Added **age warning** to Discord invite button

### v1.4.5
- Added **player counts** to the featured level list
- Made player names in invites clickable to bring up profiles
- Fixed bugs with the featured level list
- Fixed level ordering in saved tab
- Fixed "Receive Invites From" setting crash

### v1.4.4
- Added **Globed Featured Levels** (thanks Kiba)
  - Levels rated **Featured**, **Epic**, and **Outstanding**
- Featured levels now appear at the top of saved levels list
- Fixed room list not loading on some devices
- Changed Discord RPC integration icon
- Various small UI fixes

### v1.4.3
- Fixed crash on ARM MacOS with animated player names
- Fixed rare crash with devtools
- Added error message when entering wrong room password

### v1.4.2
- Displayed yourself at the top of the player list (thanks Kiba)
- Integrated with **Discord Rich Presence mod**
  - Special rich presence for players using the mod
- Fixed issue with unlisted levels in editor showing up in the level list
- Added option to **disable multiplayer in level editor**
- Fixed crash when completing created levels on ARM MacOS
- Fixed server list issue on MacOS

### 1.4.1
- Added **MacOS support**
- Improved room listing (shows player count/limit, collision settings, password)
- Sorted room list by player count
- Fixed settings not saving when exiting game

### 1.4.0
- Overhauled the room system:
  - Send room invites to players
  - Added password, collision, player limit, and invite-only options
  - Public room listing and invite options
- Introduced **user roles** with special badges for staff/supporters
- Added **editor compatibility** (non-collaborative level building)
- Added automatic server reconnection
- Added **credits** to the Globed menu
- Fixed unsynced clock and connection issues
- Added **Force Progress Bar** setting
- Redesigned main menus for better UI

### 1.3.7
- Added a mysterious button to the room menu
- Fixed interpolation issues with vsync
- Fixed restarting a level with the **Confirm Restart** feature

### 1.3.6
- Fixed crash on startup on Android

### 1.3.5
- Fixed crashes when opening other players' profiles in a level
- Fixed texture pack issues with zip files
- Fixed crash when setting the player's name on Mac

### 1.3.4
- Fixed player collision issues in the editor
- Fixed crash when opening player list
- Fixed crash when closing level browser on Android

### 1.3.3
- Speeded up asset preloading (particularly death effects)
- Added ability to hide specific players
- Fixed animation issues with platformer squish

### 1.3.2
- Improved loading times (up to **3-5 times faster**) on Windows
- Fixed rare crashes and asset preloading issues
- Added **player counts** to tower levels

### 1.3.1
- Fixed Android issues with room joining
- Fixed **portal gauntlet** progress resetting

### 1.3.0
- Added **Android (32/64-bit)** and **MacOS** support
- Redesigned settings menu with tabbed UI
- Added deafen notifications, voice chat improvements
- Fixed oice chat** issues on Mac and Android

### 1.2.5
- Fixed crash when browsing levels
- Fixed player count label issues

### 1.2.4
- Added name colors in player list
- Fixed player count label and display issues

### 1.2.3
- Added **proximity voice** option for non-platformer mode
- Fixed visual glitches and robot fire issues

### 1.2.2
- Added voice overlay to show active speakers
- Fixed recording issues when voice chat is disabled

### 1.2.1
- Refresh server list more often
- Fixed issues with in-game player list

### 1.2.0
- Fixed **deadlock** on packet limit test failure
- Added **Geode setting** to disable asset preloading

### 1.1.2
- Added player searching in player list
- Fixed lag and crashes related to voice chat

### 1.1.1
- Added pages to the online level list
- Fixed network issues related to keeping TCP connection alive

### 1.1.0
- **Networking protocol update** (TCP/UDP hybrid)
- Fixed several connection-related issues
- Added **account verification**

### 1.0.5
- Preloaded icons on the loading screen to improve loading speed

## 1.0.4
- Make the mod incompatible with More Level Tags (the issues were too hard to fix)
- Make deafening immediate (you will stop hearing others as soon as you press B)
- Fix crashes on old CPUs

## 1.0.2
- Use GitHub Actions for releases (hopefully no more crashes on old CPUs)
- Add a volume slider for voice chat
- Add a button that opens the Discord server
- Fixed a crash in the editor
- Fixed an incompatibility with More Level Tags, should no longer crash when opening a level
