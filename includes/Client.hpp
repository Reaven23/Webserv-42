#pragma once

#include <netinet/in.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <sstream>
#include <string>

#include "../includes/utils/Logger.hpp"

using namespace std;

class Client {
 private:
  // Default constructor is private as it makes no sense to have a client
  // without a fd nor an address
  Client();
  Client(Client const& other);
  Client& operator=(Client const& other);

  int _fd;
  sockaddr_in _addr;

 public:
  Client(int fd, sockaddr_in addr) : _fd(fd), _addr(addr){};
  ~Client() { close(_fd); };

  int getFd() const { return (_fd); };

  string getIp() const {
    unsigned char* ip = (unsigned char*)&_addr.sin_addr.s_addr;

    stringstream ss;
    ss << (int)ip[0] << "." << (int)ip[1] << "." << (int)ip[2] << "."
       << (int)ip[3];

    return (string(ss.str()));
  };

  void read() {
    // Buffer to store the received data.
    char buffer[1024] = {0};
    // recv() reads data sent by the client through the connection.
    //   - clientSocket   : the client fd (returned by accept, NOT the listening
    //   socket!)
    //   - buffer         : where to store the received bytes
    //   - sizeof(buffer) : max number of bytes to read (to avoid buffer
    //   overflow)
    //   - 0              : no flags (default behavior, blocking)
    // Returns the number of bytes read, 0 if the client disconnected, -1 on
    if (recv(_fd, buffer, sizeof(buffer), 0) == -1)
      Logger::error(std::string("recv(): Failed to read data: ") +
                    strerror(errno));

    Logger::info(std::string("Request from client ") + getIp() + ": " + buffer);
  }
};
