#pragma once
#include "platform.hpp"

/*
* GLOBED_SOCKET_POLL - poll function
* GLOBED_SOCKET_POLLFD - pollfd structure
*/

#ifdef GEODE_IS_WINDOWS
# pragma comment(lib, "ws2_32.lib")
# define GLOBED_SOCKET_POLL WSAPoll
# define GLOBED_SOCKET_POLLFD WSAPOLLFD

/* windows includes */
# include <WS2tcpip.h>

#elif defined(GLOBED_IS_UNIX) // ^ windows | v unix

# define GLOBED_SOCKET_POLL ::poll
# define GLOBED_SOCKET_POLLFD struct pollfd

/* unix includes */

# include <sys/socket.h>
# include <netinet/in.h>
# include <unistd.h>
# include <arpa/inet.h>
# include <poll.h>
# include <netdb.h> // struct addrinfo
# include <fcntl.h> // fcntl, F_GETFL, F_SETFL, O_NONBLOCK
# include <cerrno>

#endif
