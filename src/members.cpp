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
MCHECK(GameManager, m_gameLayer, 0x174)

// sizes

SCHECK(SimplePlayer, 0x22c)
SCHECK(GJBaseGameLayer, 0x2d40)
SCHECK(PlayLayer, 0x2f38)
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

// sizes

SCHECK(SimplePlayer, 0x2a8)

#endif
