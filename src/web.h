#pragma once

// Test interface without a remote, only with build flag -DWEB_KEYS:
// GET /key?nr=<0-9, 11-17> presses that key, GET /key alone returns the current state.
// Replaced by the full web UI in a later phase.

void webBegin();
void webHandle();
