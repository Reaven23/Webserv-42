#include "../includes/utils/Logger.hpp"

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  Logger::info("Coucou");
  Logger::warn("Attention !");
  Logger::error("T'es nul");
  return (0);
}
