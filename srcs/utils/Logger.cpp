#include "../../includes/utils/Logger.hpp"

#include <iostream>

#include "../../includes/utils/Colors.hpp"
#include "../../includes/utils/Time.hpp"

namespace Logger {
void info(string const& msg) {
  std::cout << Colors::CYAN << "[" << Time::timestamp() << "] " << "[INFO] "
            << msg << Colors::RESET << std::endl;
}

void warn(string const& msg) {
  std::cout << Colors::YELLOW << "[" << Time::timestamp() << "] "
            << "[WARNING] " << msg << Colors::RESET << std::endl;
}

void error(string const& msg) {
  std::cout << Colors::BOLD << Colors::RED << "[" << Time::timestamp() << "] "
            << "[ERROR] " << Colors::BOLD_OFF << msg << Colors::RESET
            << std::endl;
}
}  // namespace Logger
