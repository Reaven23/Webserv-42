#include "../../includes/network/Client.hpp"

#include <sstream>

#include "../../includes/utils/Logger.hpp"

using namespace std;

// Constructors

Client::Client() : _fd(-1), _addr(){};

// Destructor

Client::~Client() { close(_fd); };

// Getters

int Client::getFd() const { return (_fd); };

string& Client::getBuffer() { return (_buffer); };

string Client::getIp() const {
  unsigned char* ip = (unsigned char*)&_addr.sin_addr.s_addr;

  stringstream ss;
  ss << (int)ip[0] << "." << (int)ip[1] << "." << (int)ip[2] << "."
     << (int)ip[3];

  return (string(ss.str()));
};

// Methods

void Client::accept(int serverFd) {
  socklen_t addrLen = sizeof(_addr);

  _fd = ::accept(serverFd, (struct sockaddr*)&_addr, &addrLen);
}

void Client::read() {
  Logger::info(string("Request from client ") + getIp());

  // Temp buffer to store the received data.
  char tmp[1024] = {0};
  size_t bytes = recv(_fd, tmp, sizeof(tmp), 0);

  // recv() reads data sent by the client through the connection.
  //   - clientSocket   : the client fd (returned by accept, NOT the listening
  //   socket!)
  //   - buffer         : where to store the received bytes
  //   - sizeof(buffer) : max number of bytes to read (to avoid buffer
  //   overflow)
  //   - 0              : no flags (default behavior, blocking)
  // Returns the number of bytes read, 0 if the client disconnected, -1 on
  // error
  if (bytes > 0) {
    _buffer.append(tmp, bytes);
  } else if (bytes == 0) {  // client has ended the connection
    close(_fd);  // automatically removes the fd from the epoll interest list;
  } else {
    Logger::error(string("recv(): Failed to read data: ") + strerror(errno));
    close(_fd);
  }
}
