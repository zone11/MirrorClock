#include "keymap.h"

bool isValidKey(int key) {
  return (key >= 0 && key <= 9) || (key >= KEY_CYCLE && key <= KEY_DOWN);
}

int keyForCode(uint64_t code) {
  switch (code) {  // two supported remotes: codes 0xFD.... and 0xFF....
    case 0xFD00FF: case 0xFFA25D: return 1;
    case 0xFD807F: case 0xFF629D: return 2;
    case 0xFD40BF: case 0xFFE21D: return 3;
    case 0xFD20DF: case 0xFF22DD: return 4;
    case 0xFDA05F: case 0xFF02FD: return 5;
    case 0xFD609F: case 0xFFC23D: return 6;
    case 0xFD10EF: case 0xFFE01F: return 7;
    case 0xFD906F: case 0xFFA857: return 8;
    case 0xFD50AF: case 0xFF906F: return 9;
    case 0xFDB04F: case 0xFF9867: return 0;
    case 0xFD708F: case 0xFF6897: return KEY_CYCLE;
    case 0xFFB04F: case 0x2C739B: return KEY_RAINBOW;
    case 0xFD8877: case 0xFF18E7: return KEY_UP;
    case 0xFD28D7: case 0xFF10EF: return KEY_LEFT;
    case 0xFDA857: case 0xFF38C7: return KEY_OK;
    case 0xFD6897: case 0xFF5AA5: return KEY_RIGHT;
    case 0xFD9867: case 0xFF4AB5: return KEY_DOWN;
    default: return KEY_NONE;
  }
}
