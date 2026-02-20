#pragma once

class ServerSocket {
 private:
  // Default constructor is private as it makes no sense to have a socket
  // without a port
  ServerSocket();
  ServerSocket(ServerSocket const& other);
  ServerSocket& operator=(ServerSocket const& other);

  int _fd;
  int _port;

 public:
  ServerSocket(int port);
  ~ServerSocket();

  int getFd() const { return (_fd); };
  int getPort() const { return (_port); }
};
