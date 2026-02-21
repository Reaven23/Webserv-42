#pragma once

#include <string>

using namespace std;

namespace Logger {
void info(string const& msg);
void warn(string const& msg);
void error(string const& msg);
}  // namespace Logger
