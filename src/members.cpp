#include <Geode/Geode.hpp>
#include <defs.hpp>
// globed member checks
#define MCHECK(cls, member, offset) static_assert(offsetof(cls, member) == offset);
#define SCHECK(cls, size) static_assert(sizeof(cls) == size);

//
// Android32
//

#ifdef GLOBED_ANDROID32

MCHECK(GJBaseGameLayer, m_level, 0x5f0)
MCHECK(GJBaseGameLayer, m_player1, 0x884)
MCHECK(GJBaseGameLayer, m_player2, 0x888)
MCHECK(GJBaseGameLayer, m_isPracticeMode, 0x2a8c)

MCHECK(PlayerObject, m_isDart, 0x794)
MCHECK(PlayerObject, m_position, 0x804)

MCHECK(FMODAudioEngine, m_system, 0x184)

MCHECK(GameManager, m_playLayer, 0x16c)
MCHECK(GameManager, m_baseGameLayer, 0x174)

// sizes

SCHECK(SimplePlayer, 0x22c)
SCHECK(GJBaseGameLayer, 0x2d40)
SCHECK(PlayLayer, 0x2f38)
SCHECK(PlayerObject, 0x970)
#endif

//
// Win32
//

#ifdef GLOBED_WIN32
MCHECK(GJBaseGameLayer, m_gameState, 0x148)
MCHECK(GJBaseGameLayer, m_level, 0x5d8)
MCHECK(GJBaseGameLayer, m_player1, 0x870)
MCHECK(GJBaseGameLayer, m_player2, 0x874)
MCHECK(GJBaseGameLayer, m_objectLayer, 0x9b0)

MCHECK(PlayerObject, m_isDart, 0x7ac)
MCHECK(PlayerObject, m_position, 0x81c)

MCHECK(FMODAudioEngine, m_system, 0x190)

MCHECK(GameManager, m_playLayer, 0x198)
MCHECK(GameManager, m_gameLayer, 0x1a0)

// sizes

SCHECK(SimplePlayer, 0x22c)
SCHECK(GJBaseGameLayer, 0x2d60)
SCHECK(PlayLayer, 0x2f58)
SCHECK(GJGameLevel, 0x488)
// SCHECK(PlayerObject, 0x980)
#endif

//
// Android64
//

#ifdef GLOBED_ANDROID64
MCHECK(GameManager, m_playerFrame, offsetof(GameManager, m_gameLayer) + 0x7c)
MCHECK(GameLevelManager, m_commentUploadDelegate, 0x2e8)

MCHECK(GJAccountManager, m_username, 0x148)
MCHECK(GJAccountManager, m_gjp2, 0x160)

MCHECK(PlayerObject, m_isShip, 0x951)
MCHECK(PlayerObject, m_position, 0x9d0) // unsure

MCHECK(SimplePlayer, m_hasCustomGlowColor, 0x29c)

MCHECK(GJBaseGameLayer, m_gameState, 0x1a8)
MCHECK(GJBaseGameLayer, m_level, 0x890)
MCHECK(GJBaseGameLayer, m_player1, 0xdb0)
MCHECK(GJBaseGameLayer, m_player2, 0xdb8)
MCHECK(GJBaseGameLayer, m_objectLayer, 0xfe8)
MCHECK(GJBaseGameLayer, m_isPracticeMode, 0x3208)

MCHECK(GameManager, m_playLayer, 0x1d8)
MCHECK(GameManager, m_editorLayer, 0x1e0)
MCHECK(GameManager, m_gameLayer, 0x1e8)

MCHECK(FMODAudioEngine, m_system, 0x218)

// sizes

SCHECK(SimplePlayer, 0x2a8)
SCHECK(PlayerObject, 0xbb0)

#endif


    /*
    GJGameState android64 0x6e8
    GJBGL

    PAD = win 0xc, android32 0xc, android64 0x14;
    GJGameState m_gameState;
    GJGameLevel* m_level;
    PlaybackMode m_playbackMode;
    PAD = win 0x290, android32 0x28c, android64 0x510;
    PlayerObject* m_player1;
    PlayerObject* m_player2;
    LevelSettingsObject* m_levelSettings;
    PAD = win 0x134, android32 0x134, android64 0x21c;
    cocos2d::CCLayer* m_objectLayer;
    PAD = win 0x20C0, android32 0x20C4, android64 0x2218;
    bool m_isPracticeMode;
    bool m_practiceMusicSync;
    PAD = win 0x2E8, android32 0x2B0, android64 0x1;

    gamemanager


	PlayLayer* getPlayLayer() {
		return m_playLayer;
	}

	LevelEditorLayer* getEditorLayer() {
		return m_editorLayer;
	}

	GJBaseGameLayer* getGameLayer() {
		return m_gameLayer;
	}

    glm

	PAD = win 0x8, android32 0x18, android64 0x30;

    playlayer


	static PlayLayer* get() {
		return GameManager::sharedState()->getPlayLayer();
	}

    playerobject

	PAD = win 0x531, android32 0x305, android64 0x661;
    bool m_isShip; // win 0x7a9, android32 0x791
    bool m_isBird;
    bool m_isBall;
    bool m_isDart;
    bool m_isRobot;
    bool m_isSpider;
    bool m_isUpsideDown;
    bool m_isDead;
    bool m_isOnGround;
    bool m_isGoingLeft;
	bool m_unkBool;
	bool m_isSwing;
	PAD = win 0x66, android32 0x66, android64 0x72;
	cocos2d::CCPoint m_position;
	PAD = win 0x15c, android32 0x164, android64 0x1d8;

    gjam

	PAD = win 0x4, android32 0x4, android64 0x8;
	gd::string m_username;
	int m_accountID;
	PAD = win 0x8, android32 0x8, android64 0xc;
	gd::string m_gjp2;
    */