#include "../../includes/utils/Signals.hpp"

volatile sig_atomic_t g_stop = 0;

static void handleSignal(int signum) {
    (void)signum;
    g_stop = 1;
}

void setupSignals() {
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);
    signal(SIGPIPE, SIG_IGN);
}
