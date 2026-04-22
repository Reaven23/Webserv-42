#pragma once

#include <map>
#include <string>

class CookieUtils {
   public:
    static std::map<std::string, std::string> parse(const std::string& header);
    static std::string getValue(std ::string const& header,
                                std::string const&  key);
};
