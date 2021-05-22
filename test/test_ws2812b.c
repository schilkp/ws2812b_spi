#include "string.h"
#include "unity.h"
#include "unity_internals.h"
#include "ws2812b.h"

#define UNUSED(_x_) (void)(_x_)

void test_no_vla(void) {
  // Ensure the REQUIRED_BUFFER_LEN macro allows the creation
  // of a non-vla array when used with constant arguments.

  // This test will not compile otherwise, as -Werror=vla is set

  volatile uint8_t buffer[WS2812B_REQUIRED_BUFFER_LEN(10, WS2812B_PACKING_DOUBLE, 1, 4)];

  UNUSED(buffer);

  TEST_PASS();
}

void test_buffer_len(void) {
  TEST_ASSERT_EQUAL_UINT32(24, WS2812B_REQUIRED_BUFFER_LEN(1, WS2812B_PACKING_SINGLE, 0, 0));

  TEST_ASSERT_EQUAL_UINT32(12, WS2812B_REQUIRED_BUFFER_LEN(1, WS2812B_PACKING_DOUBLE, 0, 0));

  TEST_ASSERT_EQUAL_UINT32(53, WS2812B_REQUIRED_BUFFER_LEN(2, WS2812B_PACKING_SINGLE, 1, 4));

  TEST_ASSERT_EQUAL_UINT32(29, WS2812B_REQUIRED_BUFFER_LEN(2, WS2812B_PACKING_DOUBLE, 1, 4));

  TEST_ASSERT_EQUAL_UINT32(0, WS2812B_REQUIRED_BUFFER_LEN(0, WS2812B_PACKING_SINGLE, 0, 0));

  TEST_ASSERT_EQUAL_UINT32(5, WS2812B_REQUIRED_BUFFER_LEN(0, WS2812B_PACKING_SINGLE, 1, 4));

  // Sanity check required_buffer_len function, which is just a wrapper around this macro:
  ws2812b_handle_t h;

  h.led_count = 2;
  h.config.packing = WS2812B_PACKING_SINGLE;
  h.config.prefix_len = 1;
  h.config.suffix_len = 4;
  TEST_ASSERT_EQUAL_UINT32(53, ws2812b_required_buffer_len(&h));

  h.led_count = 2;
  h.config.packing = WS2812B_PACKING_DOUBLE;
  h.config.prefix_len = 1;
  h.config.suffix_len = 4;
  TEST_ASSERT_EQUAL_UINT32(29, ws2812b_required_buffer_len(&h));
}

void test_invalid_config_detected(void) {
  // A base, valid configuration
  ws2812b_handle_t h_valid;
  h_valid.led_count = 0;
  h_valid.config.packing = WS2812B_PACKING_SINGLE;
  h_valid.config.pulse_len_0 = WS2812B_PULSE_LEN_1b;
  h_valid.config.pulse_len_1 = WS2812B_PULSE_LEN_3b;
  h_valid.config.first_bit_0 = WS2812B_FIRST_BIT_0_ENABLED;
  h_valid.config.spi_bit_order = WS2812B_MSB_FIRST;
  h_valid.config.prefix_len = 1;
  h_valid.config.prefix_len = 4;

  // Make sure it is valid
  TEST_ASSERT_FALSE_MESSAGE(ws2812b_init(&h_valid), "Reported correct config as incorrect.");

  ws2812b_handle_t h_test = {0};

  // Make sure incorrect configs are detected

  // packing
  memcpy(&h_test, &h_valid, sizeof(ws2812b_handle_t));
  h_test.config.packing = 0xFF;
  TEST_ASSERT_TRUE_MESSAGE(ws2812b_init(&h_test), "Did not detect invalid config.packing!");

  // pulse_len_0
  memcpy(&h_test, &h_valid, sizeof(ws2812b_handle_t));
  h_test.config.pulse_len_0 = 0;
  TEST_ASSERT_TRUE_MESSAGE(ws2812b_init(&h_test), "Did not detect invalid config.pulse_len_0!");
  memcpy(&h_test, &h_valid, sizeof(ws2812b_handle_t));
  h_test.config.pulse_len_0 = 0x55;
  TEST_ASSERT_TRUE_MESSAGE(ws2812b_init(&h_test), "Did not detect invalid config.pulse_len_0!");

  // pulse_len_1
  memcpy(&h_test, &h_valid, sizeof(ws2812b_handle_t));
  h_test.config.pulse_len_1 = 0;
  TEST_ASSERT_TRUE_MESSAGE(ws2812b_init(&h_test), "Did not detect invalid config.pulse_len_1!");
  memcpy(&h_test, &h_valid, sizeof(ws2812b_handle_t));
  h_test.config.pulse_len_1 = 0x55;
  TEST_ASSERT_TRUE_MESSAGE(ws2812b_init(&h_test), "Did not detect invalid config.pulse_len_1!");

  // first_bit_0
  memcpy(&h_test, &h_valid, sizeof(ws2812b_handle_t));
  h_test.config.first_bit_0 = 0xFF;
  TEST_ASSERT_TRUE_MESSAGE(ws2812b_init(&h_test), "Did not detect invalid config.first_bit_0!");

  // spi_bit_order
  memcpy(&h_test, &h_valid, sizeof(ws2812b_handle_t));
  h_test.config.spi_bit_order = 0xFF;
  TEST_ASSERT_TRUE_MESSAGE(ws2812b_init(&h_test), "Did not detect invalid config.spi_bit_order!");

  // Make sure same / swapped pulse lengths are detected

  // same
  memcpy(&h_test, &h_valid, sizeof(ws2812b_handle_t));
  h_test.config.pulse_len_0 = WS2812B_PULSE_LEN_1b;
  h_test.config.pulse_len_1 = WS2812B_PULSE_LEN_1b;
  TEST_ASSERT_TRUE_MESSAGE(ws2812b_init(&h_test), "Did not detect that pulses where same length!");

  // swapped
  memcpy(&h_test, &h_valid, sizeof(ws2812b_handle_t));
  h_test.config.pulse_len_0 = WS2812B_PULSE_LEN_2b;
  h_test.config.pulse_len_1 = WS2812B_PULSE_LEN_1b;
  TEST_ASSERT_TRUE_MESSAGE(ws2812b_init(&h_test), "Did not detect that pulses where same length!");
}

void setUp(void) {}
void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_no_vla);
  RUN_TEST(test_buffer_len);
  RUN_TEST(test_invalid_config_detected);
  return UNITY_END();
}
