#include "remote.h"

#include <IRrecv.h>
#include <IRremoteESP8266.h>

#include "config.h"
#include "keymap.h"

namespace {

const uint64_t NEC_REPEAT = 0xFFFFFFFF;

IRrecv receiver(IR_PIN);
uint64_t lastCode = 0;

}  // namespace

void remoteBegin() {
  receiver.enableIRIn();
}

int remotePoll() {
  decode_results results;
  if (!receiver.decode(&results)) {
    return KEY_NONE;
  }
  uint64_t code = results.value;
  if (code == NEC_REPEAT) {  // button held down: repeat the last key
    code = lastCode;
  }
  lastCode = code;
  receiver.resume();
  return keyForCode(code);
}
