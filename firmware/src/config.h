#pragma once

// Build-time hardware configuration.
// Don't edit this file for local setups. Instead copy config_local.example.h to config_local.h
// (gitignored) and override the settings there.

#if __has_include("config_local.h")
#include "config_local.h"
#endif

// E-paper panel. The default is the GDEH0154D67 (Waveshare 1.54" V2).
// Define DISPLAY_GDEY0154D67 to use the newer GDEY0154D67 panel instead.
// #define DISPLAY_GDEY0154D67
