/*
* This is like the root `defs.hpp` except it contains things specifically needed for networking.
*/

#pragma once
#include <config.hpp>
#include <Geode/Geode.hpp>

/*
* GLOBED_SOCKET_POLL - poll function
* GLOBED_SOCKET_POLLFD - pollfd structure
*/

#ifdef GEODE_IS_WINDOWS
# pragma comment(lib, "ws2_32.lib")
# define GLOBED_SOCKET_POLL WSAPoll
# define GLOBED_SOCKET_POLLFD WSAPOLLFD

/* windows includes */

# include <ws2tcpip.h>

#else // ^ windows | v unix

# define GLOBED_SOCKET_POLL ::poll
# define GLOBED_SOCKET_POLLFD struct pollfd

/* unix includes */

# include <sys/socket.h>
# include <netinet/in.h>
# include <unistd.h>
# include <cerrno>
# include <arpa/inet.h>
# include <poll.h>

#endif // GEODE_IS_WINDOWS
