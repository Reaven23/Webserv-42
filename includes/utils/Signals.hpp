#pragma once

#include <csignal>

extern volatile sig_atomic_t g_stop;

void setupSignals();
