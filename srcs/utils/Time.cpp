#include "../../includes/utils/Time.hpp"

#include <sys/time.h>

#include <cstdio>
#include <ctime>
#include <sstream>

using namespace std;

namespace Time {
string timestamp() {
    time_t now = time(NULL);

    struct tm* tm_info = localtime(&now);

    ostringstream os;

    tm_info->tm_hour > 9 ? os << tm_info->tm_hour
                         : os << "0" << tm_info->tm_hour;
    os << ":";
    tm_info->tm_min > 9 ? os << tm_info->tm_min : os << "0" << tm_info->tm_min;
    os << ":";
    tm_info->tm_sec > 9 ? os << tm_info->tm_sec : os << "0" << tm_info->tm_sec;
    return os.str();
}
}  // namespace Time
