#include "../../includes/network/ServerSocket.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

#include "../../includes/network/helpers.hpp"

using namespace std;

ServerSocket::ServerSocket(int port) {
  if (port < 0 || port > 65535) throw invalid_argument("invalid port");
  _port = port;

  // socket() creates a file descriptor for network communication.
  //   - AF_INET     : use IPv4 protocol (as opposed to AF_INET6 for IPv6)
  //   - SOCK_STREAM : TCP socket (reliable, ordered byte stream;
  //                   as opposed to SOCK_DGRAM for UDP which is unreliable and
  //                   unordered)
  //   - 0           : let the OS pick the default protocol (TCP for
  //   SOCK_STREAM)
  _fd = socket(AF_INET, SOCK_STREAM, 0);
  if (_fd == -1) {
    throw runtime_error(string("socket(): Fatal error: ") + strerror(errno));
  }

  // Allow reuse port that have just been used
  int opt = 1;
  if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    throw runtime_error(string("setsockopt(): Fatal error: ") +
                        strerror(errno));
  }

  // sockaddr_in is a struct that describes an IPv4 address (IP + port).
  sockaddr_in addr = {};

  // sin_family : must match the first argument of socket() (AF_INET = IPv4).
  addr.sin_family = AF_INET;

  // sin_port : the port the server will listen on.
  // htons() (Host To Network Short) converts a 16-bit integer from the host's
  // byte order to network byte order (big-endian).
  // This is required because the network always uses big-endian, but CPU
  // uses little-endian.
  addr.sin_port = htons(_port);

  // sin_addr.s_addr : the IP address to listen on.
  // INADDR_ANY (= 0.0.0.0) means "accept connections on all network interfaces"
  // (localhost, wifi, ethernet, etc.).
  addr.sin_addr.s_addr = INADDR_ANY;

  // bind() attaches the socket to the address (IP + port) defined above.
  //   - serverServerSocket                    : the socket fd to configure
  //   - (struct sockaddr*)&serverAddress : cast required because bind() expects
  //                                       the generic sockaddr* type, not
  //                                       sockaddr_in*
  //   - sizeof(serverAddress)            : size of the struct so bind() knows
  //                                       how many bytes to read
  // Without bind(), the socket is not attached to any port and cannot receive
  // anything.
  if (bind(_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    close(_fd);
    throw runtime_error(string("bind(): Fatal error: ") + strerror(errno));
  }

  if (!setSocketNonBlocking(_fd)) {
    throw runtime_error(string("fnctl(): Error when setting socket to "
                               "non-blocking mode: ") +
                        strerror(errno));
  }

  // listen() puts the socket in listening mode: it can now accept incoming
  // connections.
  //   - serverServerSocket : the socket fd (already bound)
  //   - 5            : backlog size = max number of pending connections
  //   waiting
  //                   in the queue before the OS starts refusing new ones.
  //                   SOMAXCONN is the maximum size, usually 128. This is
  //                   NOT the max number of connected clients.
  if (listen(_fd, SOMAXCONN) == -1) {
    close(_fd);
    throw std::runtime_error(std::string("listen(): Fatal error: ") +
                             strerror(errno));
  }
};

ServerSocket::~ServerSocket() { close(_fd); };
