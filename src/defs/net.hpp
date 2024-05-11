#pragma once
#include "platform.hpp"

/*
* GLOBED_SOCKET_POLL - poll function
* GLOBED_SOCKET_POLLFD - pollfd structure
*/

#ifdef GEODE_IS_WINDOWS

# define GLOBED_SOCKET_POLL WSAPoll
# define GLOBED_SOCKET_POLLFD WSAPOLLFD

#elif defined(GLOBED_IS_UNIX) // ^ windows | v unix

# define GLOBED_SOCKET_POLL ::poll
# define GLOBED_SOCKET_POLLFD struct pollfd

#endif
