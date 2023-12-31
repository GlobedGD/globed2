#include <Geode/Geode.hpp>
#include <defs.hpp>
// globed member checks
#define MCHECK(cls, member, offset) static_assert(offsetof(cls, member) == offset);
#define SCHECK(cls, size) static_assert(sizeof(cls) == size);

#if defined(GLOBED_ANDROID) && GLOBED_ANDROID
/* gjbgl */
MCHECK(GJBaseGameLayer, m_level, 0x5f0)
MCHECK(GJBaseGameLayer, m_player1, 0x884)
MCHECK(GJBaseGameLayer, m_player2, 0x888)
/* playerobject */
MCHECK(PlayerObject, m_isDart, 0x794)
MCHECK(PlayerObject, m_position, 0x804)
/* fmodaudoengine */
MCHECK(FMODAudioEngine, m_system, 0x184)
MCHECK(GameManager, m_gameLayer, 0x16c)
#else
/* gjbgl */
MCHECK(GJBaseGameLayer, m_level, 0x5d8)
MCHECK(GJBaseGameLayer, m_gameState, 0x148)
MCHECK(GJBaseGameLayer, m_player1, 0x870)
MCHECK(GJBaseGameLayer, m_player2, 0x874)
MCHECK(GJBaseGameLayer, m_objectLayer, 0x9b0)
/* playerobject */
MCHECK(PlayerObject, m_isDart, 0x7ac)
MCHECK(PlayerObject, m_position, 0x81c)
/* fmodaudioengine */
MCHECK(FMODAudioEngine, m_system, 0x190)
/* gamemanager */
MCHECK(GameManager, m_gameLayer, 0x198)
#endif

// sizes

SCHECK(SimplePlayer, 0x22c)

#if defined(GLOBED_ANDROID) && GLOBED_ANDROID
SCHECK(GJBaseGameLayer, 0x2d40)
SCHECK(PlayLayer, 0x2f38)
#else
SCHECK(GJBaseGameLayer, 0x2d60)
SCHECK(PlayLayer, 0x2f58)
SCHECK(GJGameLevel, 0x488)
#endif
