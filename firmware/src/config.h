#pragma once

// Build-time hardware configuration.
// Don't edit this file for local setups. Instead copy config_local.example.h to config_local.h
// (gitignored) and override the settings there.

// Supported e-paper panels, used as values for DISPLAY_PANEL.
#define DISPLAY_GDEH0154D67 1 // Waveshare 1.54" V2
#define DISPLAY_GDEY0154D67 2

#if __has_include("config_local.h")
#include "config_local.h"
#endif

// E-paper panel, one of the DISPLAY_* values above.
#ifndef DISPLAY_PANEL
#define DISPLAY_PANEL DISPLAY_GDEH0154D67
#endif
