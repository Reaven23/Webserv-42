#pragma once

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>

class Client {
 private:
  Client(Client const& other);
  Client& operator=(Client const& other);

  // Attributes

  int _fd;
  sockaddr_in _addr;
  std::string _buffer;

  // Methods

  bool _isRequestComplete() const { return (true); };

 public:
  // Constructors

  Client();

  // Destructor

  ~Client();

  // Getters

  int getFd() const;
  std::string& getBuffer();
  std::string getIp() const;

  // Methods

  void accept(int serverFd);

  void read();
};
