#pragma once

// Key handling and the brightness menu, shared by the IR remote and the web interface.

void controlsHandleKey(int key);                  // key number from keymap.h; KEY_NONE is ignored
void controlsApplyBrightness(unsigned long now);  // call every loop before drawing
void controlsCheckMenuTimeout();                  // closes the menu 6 s after the last key
bool controlsMenuActive();                        // menu symbol shown instead of the time
