#include <fcntl.h>

bool setSocketNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);

  if (flags == -1) return (false);
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return (false);

  return (true);
}

bool setCloseOnExec(int fd) {
  int flags = fcntl(fd, F_GETFD, 0);

  if (flags == -1) return (false);
  if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) return (false);

  return (true);
}
