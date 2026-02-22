#include <exception>

#include "../includes/network/Server.hpp"
#include "../includes/utils/Logger.hpp"

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  ServerConfig config;

  try {
    Server server(config);

    server.run();
  } catch (std::exception& e) {
    Logger::error(e.what());
  }

  return (0);
}
