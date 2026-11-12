#pragma once

#include "island/page_context.h"

// Power page: shut down, reboot, reboot to UEFI, lock, sleep. The three that end the session arm on the
// first click (red, "Confirm?") and run only on a second click within theme::confirmTimeoutMs.
BuiltPage buildPowerPage(const PageContext& ctx, float width, float height);
