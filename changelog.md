## v1.1.0

Globed now uses a different networking protocol (TCP instead of UDP). This should fix **many (but not all!) of the connection related issues**, such as players not loading in a level, or the player list popup taking an infinite amount of time to load.

That means this is a **required** update which bumps the protocol version to 2. You will not be able to connect to an updated server without updating the mod to this version.

If you are a server owner, please read the [server changelog](https://github.com/dankmeme01/globed2/blob/main/server/changelog.md) as there have been some changes to the server.

Additional changes include:

* Improve the account verification system (and enable it on the main server)
* Fix the freezes/crashes on the loading screen and update the message text at the bottom when loading icons
* (Maybe) fix crash when applying a texture pack with Texture Loader
* Fix audio crash
* Add a volume indicator in the pause menu player list (it's a lie i haven't done it yet)
* Fix the TPS cap setting being impossible to change
* Make the mute button work immediately instead of with a delay
* Add an automatic rollback system for a certain save bug related to gauntlets
*  ^ in versions before v1.0.5 there used to be a bug that would cause your savefile to have some invalid levels. Now those will be automatically removed on startup.

## v1.0.5

* Preload icons on the loading screen (longer loading time but hopefully a lot less lagspikes in-game now)
* Fix disconnecting when minimizing the game
* Room popup now doesn't auto refresh, and sorts players alphabetically (also your friends are now shown before everyone else!)
* ^ same applies for the in-game player list popup
* Scroll position no longer resets when refreshing a list
* Changed the voice chat volume slider to be ranged from 0% to 200% instead of 100%
* You can now hear yourself when testing audio in settings
* Make asset preloading optional (if you have weird texture bugs, disable it in settings!)
* Hopefully fix a crash when disconnecting the active audio input device
* Maybe fixed death effects crashing the game
* Fix some crashes caused by corrupted data
* Fix gauntlet levels sometimes being broken
* Fix player opacity not working properly on spider and robot
* Add a message indicating how to use voice chat

## v1.0.4

* Make incompatible with More Level Tags (the issues were too hard to fix sorry)
* Make deafening immediate (you will stop hearing others as soon as you press B, no need to wait)
* Fix crashes on old CPUs, now for real

## v1.0.2

* Use github actions for releases (hopefully no more crashes on old CPUs)
* Add a volume slider for voice chat
* Add a button that opens the discord server
* Fixed a crash in the editor
* Fixed an incompatibility with the mod More Level Tags, should no longer crash when opening a level
