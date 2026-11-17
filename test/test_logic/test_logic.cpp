// Host tests for the hardware-independent logic: pio test -e native
// Compares the rewrite against golden data recorded from the original firmware.

#include <unity.h>

#include <set>
#include <string>

#include "../../src/keymap.cpp"
#include "../../src/layout.cpp"
#include "golden.h"

static std::set<int> lit;

static void record(uint8_t led) {
  lit.insert(led);
}

// Lit LEDs as "1 2 3", the format of the golden data
static std::string litLeds() {
  std::string text;
  for (int led : lit) {
    text += (text.empty() ? "" : " ") + std::to_string(led);
  }
  lit.clear();
  return text;
}

void setUp() {
  lit.clear();
}

void tearDown() {}

void test_every_minute_of_the_day() {
  for (int hour = 0; hour < 24; hour++) {
    for (int minute = 0; minute < 60; minute++) {
      layoutTime(hour, minute, record);
      char label[16];
      snprintf(label, sizeof(label), "%02d:%02d", hour, minute);
      TEST_ASSERT_EQUAL_STRING_MESSAGE(GOLDEN_TIME[hour * 60 + minute], litLeds().c_str(), label);
    }
  }
}

void test_symbols() {
  layoutWifiSymbol(record);
  TEST_ASSERT_EQUAL_STRING(GOLDEN_WIFI, litLeds().c_str());
  layoutAutoSymbol(record);
  TEST_ASSERT_EQUAL_STRING(GOLDEN_AUTO, litLeds().c_str());
  layoutManualSymbol(record);
  TEST_ASSERT_EQUAL_STRING(GOLDEN_MANUAL, litLeds().c_str());
  for (int level = 1; level <= 5; level++) {
    layoutLevelBars(level, record);
    TEST_ASSERT_EQUAL_STRING(GOLDEN_LEVEL_BARS[level - 1], litLeds().c_str());
  }
}

void test_ir_codes() {
  for (const GoldenCode &golden : GOLDEN_CODES) {
    TEST_ASSERT_EQUAL_INT(golden.key, keyForCode(golden.code));
    TEST_ASSERT_TRUE(isValidKey(golden.key));
  }
  TEST_ASSERT_EQUAL_INT(KEY_NONE, keyForCode(0x123456));
  TEST_ASSERT_EQUAL_INT(KEY_NONE, keyForCode(0xFFFFFFFF));  // NEC repeat is handled in remote.cpp
}

void test_valid_keys() {
  TEST_ASSERT_FALSE(isValidKey(KEY_NONE));
  TEST_ASSERT_FALSE(isValidKey(10));
  TEST_ASSERT_FALSE(isValidKey(18));
  for (int key = 0; key <= 9; key++) {
    TEST_ASSERT_TRUE(isValidKey(key));
  }
  for (int key = KEY_CYCLE; key <= KEY_DOWN; key++) {
    TEST_ASSERT_TRUE(isValidKey(key));
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_every_minute_of_the_day);
  RUN_TEST(test_symbols);
  RUN_TEST(test_ir_codes);
  RUN_TEST(test_valid_keys);
  return UNITY_END();
}
