#include "../../includes/utils/Time.hpp"

#include <sys/time.h>

#include <cstdio>

using namespace std;

// TODO: a la fin du projet il faudra surement retirer cette partie car elle
// contient des fonctions interdites
namespace Time {
string timestamp() {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  struct tm* tm_info = localtime(&tv.tv_sec);

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d", tm_info->tm_hour,
           tm_info->tm_min, tm_info->tm_sec, (int)(tv.tv_usec / 1000));
  return string(buffer);
}
}  // namespace Time
