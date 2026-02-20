#include <cerrno>
#include <cstring>

#include "../includes/Client.hpp"
#include "../includes/Server.hpp"
#include "../includes/utils/Logger.hpp"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  Server server(8080);

  int serverFd = server.getSocketFd();
  sockaddr_in clientAddr = {};
  socklen_t clientAddrLen = sizeof(clientAddr);

  // accept() blocks until a client connects, then returns a NEW fd dedicated
  // to that specific client.
  //   - serverSocket : the listening socket (stays open for other clients)
  //   - NULL         : we don't need the client's address (otherwise: sockaddr*
  //   + socklen_t*)
  //   - NULL         : same, we don't need the address length
  // The returned fd (clientSocket) is used to communicate with THIS specific
  // client.
  int clientSocketFd =
      accept(serverFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
  if (clientSocketFd == -1)
    Logger::error(std::string("accept(): Failed to accept connection: ") +
                  strerror(errno));

  Client client(clientSocketFd, clientAddr);
  client.read();

  return (0);
}
