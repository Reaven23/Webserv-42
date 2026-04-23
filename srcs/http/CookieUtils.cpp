#include "../../includes/http/CookieUtils.hpp"

using namespace std;

map<string, string> CookieUtils::parse(string const& header) {
    map<string, string> cookies;
    size_t              pos = 0;

    while (pos < header.size()) {
        // skip whitespaces
        while (pos < header.size() && header[pos] == ' ') pos++;

        // find '='
        size_t eq = header.find('=', pos);
        if (eq == string::npos) break;

        string key = header.substr(pos, eq - pos);

        // find ';' or end
        size_t semi = header.find(';', eq + 1);
        string val;
        if (semi == string::npos) {
            val = header.substr(eq + 1);
            pos = header.size();
        } else {
            val = header.substr(eq + 1, semi - (eq + 1));
            pos = semi + 1;
        }

        cookies[key] = val;
    }

    return cookies;
}

string CookieUtils::getValue(string const& header, string const& key) {
    map<string, string>                 cookies = parse(header);
    map<string, string>::const_iterator it = cookies.find(key);

    if (it != cookies.end()) return it->second;

    return "";
};
