#include "stdlib.h"
#include "string.h"
#include "unity.h"
#include "unity_internals.h"
#include "ws2812b.h"

#define UNUSED(_x_) (void)(_x_)

// ======== Utils ==================================================================================

void util_generate_iter_buf(ws2812b_handle_t *h, uint8_t *buf) {

  const uint32_t buf_len = ws2812b_required_buffer_len(h);
  memset(buf, 0x55, buf_len); // Fill with values not generate by driver to detect unwritten values

  ws2812b_iter_restart(h);

  for (uint32_t i = 0; i < buf_len; i++) {
    if (ws2812b_iter_is_finished(h)) {
      // Keeps 0x55 at end of buffer to aid in detection of prematurly finishing iterator.
      break;
    }
    buf[i] = ws2812b_iter_next(h);
  }
}

#define ERROR_MSG_LEN 200
char error_msg[ERROR_MSG_LEN];

bool util_test_driver_output(ws2812b_handle_t *h, uint8_t *buf_data_expected) {

  if (ws2812b_init(h)) {
    snprintf(error_msg, ERROR_MSG_LEN, "Init function failed!");
    return false;
  }

  uint32_t buffer_len = ws2812b_required_buffer_len(h);
  uint32_t data_len = WS2812B_DATA_LEN(h->led_count, h->config.packing);
  uint32_t prefix_len = h->config.prefix_len;
  uint32_t suffix_len = h->config.suffix_len;

  // Generate buf:
  uint8_t *buf = malloc(sizeof(uint8_t) * buffer_len);
  ws2812b_fill_buffer(h, buf);

  // Ensure prefix is 0
  for (uint32_t i = 0; i < prefix_len; i++) {
    if (buf[i] != 0) {
      snprintf(error_msg, ERROR_MSG_LEN, "Prefix is not 0 at buffer index 0x%x!", i);
      free(buf);
      return false;
    }
  }

  // Ensure data is as exptected
  for (uint32_t i = 0; i < data_len; i++) {
    if (buf[i + prefix_len] != buf_data_expected[i]) {
      snprintf(error_msg, ERROR_MSG_LEN, "Buffer is wrong at %i, expected 0x%x, got 0x%x!",
               i + prefix_len, buf_data_expected[i], buf[i + prefix_len]);
      free(buf);
      return false;
    }
  }

  // Ensure suffix is 0
  for (uint32_t i = prefix_len + data_len; i < prefix_len + data_len + suffix_len; i++) {
    if (buf[i] != 0) {
      snprintf(error_msg, ERROR_MSG_LEN, "Suffix is not 0 at buffer index 0x%x!", i);
      free(buf);
      return false;
    }
  }

  // Also generate buffer using iterator:
  uint8_t *iter_buf = malloc(sizeof(uint8_t) * buffer_len);
  util_generate_iter_buf(h, iter_buf);
  if (!ws2812b_iter_is_finished(h)) {
    snprintf(error_msg, ERROR_MSG_LEN, "Iterator did not report finished!");
    free(iter_buf);
    free(buf);
    return false;
  }

  // compare iterator buffer to normal buffer:
  for (uint32_t i = 0; i < buffer_len; i++) {
    if (buf[i] != iter_buf[i]) {
      snprintf(error_msg, ERROR_MSG_LEN,
               "Iterator does not match buffer at %i, expected 0x%x, got 0x%x!", i, buf[i],
               iter_buf[i]);
      free(iter_buf);
      free(buf);
      return false;
    }
  }

  free(iter_buf);
  free(buf);
  return true;
}

// ======== Tests ==================================================================================

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
  h_valid.config.suffix_len = 4;

  // Make sure it is valid
  TEST_ASSERT_FALSE_MESSAGE(ws2812b_init(&h_valid), "Reported correct config as incorrect!");

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

  // Make sure pulses that are too long for the double packing are detected:
  // Test pulse_len_1 only: if pulse_len_0 is too long, pulse_len_1 is also too long, or swapped
  // pulse length would trigger
  uint8_t lengths[] = {WS2812B_PULSE_LEN_4b, WS2812B_PULSE_LEN_5b, WS2812B_PULSE_LEN_6b,
                       WS2812B_PULSE_LEN_7b};

  memcpy(&h_test, &h_valid, sizeof(ws2812b_handle_t));
  h_test.config.packing = WS2812B_PACKING_DOUBLE;
  h_test.config.pulse_len_0 = WS2812B_PULSE_LEN_1b;

  for (uint32_t len = 0; len < 4; len++) {
    h_test.config.pulse_len_1 = lengths[len];
    TEST_ASSERT_TRUE_MESSAGE(ws2812b_init(&h_test),
                             "Did not detect that pulses is too long for double packing!");
  }
}

#define ITERATOR_LED_COUNT 10
void test_iterator_end_behavior(void) {
  // Ensure that the iterator takes the expected amount of steps,
  // indicates the end correctly, and repeadetly outputs 0x00 at end.

  ws2812b_led_t leds[ITERATOR_LED_COUNT];
  memset(leds, 0xff, sizeof(leds));

  uint32_t expected_len;

  // Test with single packing
  ws2812b_handle_t h;
  h.led_count = ITERATOR_LED_COUNT;
  h.leds = leds;
  h.config.packing = WS2812B_PACKING_SINGLE;
  h.config.pulse_len_0 = WS2812B_PULSE_LEN_1b;
  h.config.pulse_len_1 = WS2812B_PULSE_LEN_3b;
  h.config.first_bit_0 = WS2812B_FIRST_BIT_0_ENABLED;
  h.config.spi_bit_order = WS2812B_MSB_FIRST;
  h.config.prefix_len = 1;
  h.config.suffix_len = 4;
  expected_len = ws2812b_required_buffer_len(&h);
  TEST_ASSERT_FALSE_MESSAGE(ws2812b_init(&h), "Init function failed!");

  // Assert that iterator does not report finished
  TEST_ASSERT_FALSE(ws2812b_iter_is_finished(&h));

  for (uint32_t i = 0; i < expected_len; i++) {
    ws2812b_iter_next(&h);

    // Unless this is the last one, assert iterator does not report finished
    if (i != expected_len - 1) {
      TEST_ASSERT_FALSE(ws2812b_iter_is_finished(&h));
    }
  }

  // Assert that iterator reports finished:
  TEST_ASSERT_TRUE(ws2812b_iter_is_finished(&h));

  // From now on, the struct should not change and always report 0
  ws2812b_handle_t before = h;
  for (int i = 0; i < 100; i++) {
    TEST_ASSERT_EQUAL_HEX32(0, ws2812b_iter_next(&h));
    TEST_ASSERT_TRUE(ws2812b_iter_is_finished(&h));
  }

  // Assert that this did not change the struct
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&before, &h, sizeof(h),
                                   "Iteration changed struct after finishing!");

  // Restarting should have it not report finished anymore
  ws2812b_iter_restart(&h);
  TEST_ASSERT_FALSE(ws2812b_iter_is_finished(&h));

  // Test with double packing
  h.config.packing = WS2812B_PACKING_DOUBLE;
  expected_len = ws2812b_required_buffer_len(&h);
  TEST_ASSERT_FALSE_MESSAGE(ws2812b_init(&h), "Reported correct config as incorrect!");

  // Assert that iterator does not report finished
  TEST_ASSERT_FALSE(ws2812b_iter_is_finished(&h));

  for (uint32_t i = 0; i < expected_len; i++) {
    ws2812b_iter_next(&h);

    // Unless this is the last one, assert iterator does not report finished
    if (i != expected_len - 1) {
      TEST_ASSERT_FALSE(ws2812b_iter_is_finished(&h));
    }
  }

  // Assert that iterator reports finished:
  TEST_ASSERT_TRUE(ws2812b_iter_is_finished(&h));

  // From now on, the struct should not change and always report 0
  before = h;
  for (int i = 0; i < 100; i++) {
    TEST_ASSERT_EQUAL_HEX32(0, ws2812b_iter_next(&h));
    TEST_ASSERT_TRUE(ws2812b_iter_is_finished(&h));
  }

  // Asser that this did not change the struct
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&before, &h, sizeof(h),
                                   "Iteration changed struct after finishing!");

  // Restarting should have it not report finished anymore
  ws2812b_iter_restart(&h);
  TEST_ASSERT_FALSE(ws2812b_iter_is_finished(&h));
}

void test_single_led(void) {
  ws2812b_led_t leds[1];
  leds[0].green = 0xaa;
  leds[0].red = 0x55;
  leds[0].blue = 0x0F;

  // Test with single packing
  ws2812b_handle_t h;
  h.led_count = 1;
  h.leds = leds;
  h.config.packing = WS2812B_PACKING_SINGLE;
  h.config.pulse_len_0 = WS2812B_PULSE_LEN_2b;
  h.config.pulse_len_1 = WS2812B_PULSE_LEN_6b;
  h.config.first_bit_0 = WS2812B_FIRST_BIT_0_ENABLED;
  h.config.spi_bit_order = WS2812B_LSB_FIRST;
  h.config.prefix_len = 10;
  h.config.suffix_len = 0;

  const uint8_t p1 = 0x7E;
  const uint8_t p0 = 0x06;
  uint8_t buf_data_expected[24] = {/* g=0xaa */ p1, p0, p1, p0, p1, p0, p1, p0,
                                   /* r=0x55 */ p0, p1, p0, p1, p0, p1, p0, p1,
                                   /* b=0x0f */ p0, p0, p0, p0, p1, p1, p1, p1};

  TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected), error_msg);

  // test with double packing
  h.config.packing = WS2812B_PACKING_DOUBLE;
  h.config.pulse_len_0 = WS2812B_PULSE_LEN_1b;
  h.config.pulse_len_1 = WS2812B_PULSE_LEN_2b;

  const uint8_t p11 = 0x66;
  const uint8_t p10 = 0x26;
  const uint8_t p01 = 0x62;
  const uint8_t p00 = 0x22;
  uint8_t buf_data_expected_double[12] = {/* g=0xaa */ p10, p10, p10, p10,
                                          /* r=0x55 */ p01, p01, p01, p01,
                                          /* b=0x0f */ p00, p00, p11, p11};

  TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected_double), error_msg);
}

void test_multiple_leds(void) {
  ws2812b_led_t leds[3];
  leds[0].green = 0xaa;
  leds[0].red = 0x55;
  leds[0].blue = 0x0F;

  leds[1].red = 0xff;
  leds[1].green = 0xff;
  leds[1].blue = 0xff;

  leds[2].green = 0x0f;
  leds[2].red = 0xf0;
  leds[2].blue = 0x00;

  // Test with single packing
  ws2812b_handle_t h;
  h.led_count = 3;
  h.leds = leds;
  h.config.packing = WS2812B_PACKING_SINGLE;
  h.config.pulse_len_0 = WS2812B_PULSE_LEN_3b;
  h.config.pulse_len_1 = WS2812B_PULSE_LEN_5b;
  h.config.first_bit_0 = WS2812B_FIRST_BIT_0_DISABLED;
  h.config.spi_bit_order = WS2812B_LSB_FIRST;
  h.config.prefix_len = 0;
  h.config.suffix_len = 10;

  const uint8_t p1 = 0x1F;
  const uint8_t p0 = 0x07;
  uint8_t buf_data_expected[24 * 3] = {
      /* [0].g=0xaa */ p1, p0, p1, p0, p1, p0, p1, p0,
      /* [0].r=0x55 */ p0, p1, p0, p1, p0, p1, p0, p1,
      /* [0].b=0x0f */ p0, p0, p0, p0, p1, p1, p1, p1,

      /* [1].g=1xff */ p1, p1, p1, p1, p1, p1, p1, p1,
      /* [1].r=1xff */ p1, p1, p1, p1, p1, p1, p1, p1,
      /* [1].b=1xff */ p1, p1, p1, p1, p1, p1, p1, p1,

      /* [2].g=0x0f */ p0, p0, p0, p0, p1, p1, p1, p1,
      /* [2].r=0xf0 */ p1, p1, p1, p1, p0, p0, p0, p0,
      /* [2].b=0x00 */ p0, p0, p0, p0, p0, p0, p0, p0};

  TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected), error_msg);

  // test with double packing
  h.config.packing = WS2812B_PACKING_DOUBLE;
  h.config.pulse_len_0 = WS2812B_PULSE_LEN_2b;
  h.config.pulse_len_1 = WS2812B_PULSE_LEN_3b;

  const uint8_t p11 = 0x77;
  const uint8_t p10 = 0x37;
  const uint8_t p01 = 0x73;
  const uint8_t p00 = 0x33;
  uint8_t buf_data_expected_double[12 * 3] = {/* [0].g=0xaa */ p10, p10, p10, p10,
                                              /* [0].r=0x55 */ p01, p01, p01, p01,
                                              /* [0].b=0x0f */ p00, p00, p11, p11,

                                              /* [1].g=1xff */ p11, p11, p11, p11,
                                              /* [1].r=1xff */ p11, p11, p11, p11,
                                              /* [1].b=1xff */ p11, p11, p11, p11,

                                              /* [2].g=0x0f */ p00, p00, p11, p11,
                                              /* [2].r=0xf0 */ p11, p11, p00, p00,
                                              /* [2].b=0x00 */ p00, p00, p00, p00};

  TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected_double), error_msg);
}

void test_pulse_length() {
  ws2812b_led_t leds[1];
  leds[0].green = 0x00;
  leds[0].red = 0x00;
  leds[0].blue = 0x00;

  ws2812b_handle_t h;
  h.led_count = 1;
  h.leds = leds;
  h.config.packing = WS2812B_PACKING_SINGLE;
  h.config.pulse_len_0 = WS2812B_PULSE_LEN_1b;
  h.config.pulse_len_1 = WS2812B_PULSE_LEN_2b;
  h.config.first_bit_0 = WS2812B_FIRST_BIT_0_DISABLED;
  h.config.spi_bit_order = WS2812B_LSB_FIRST;
  h.config.prefix_len = 0;
  h.config.suffix_len = 0;

  uint8_t buf_data_expected[24] = {0};

  // Test WS2812B_PULSE_LEN_1b
  memset(buf_data_expected, 0x1, 24);
  TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected), error_msg);

  // Test WS2812B_PULSE_LEN_2b - WS2812B_PULSE_LEN_7b
  leds[0].green = 0xff;
  leds[0].red = 0xff;
  leds[0].blue = 0xff;
  uint8_t lengths[] = {WS2812B_PULSE_LEN_2b, WS2812B_PULSE_LEN_3b, WS2812B_PULSE_LEN_4b,
                       WS2812B_PULSE_LEN_5b, WS2812B_PULSE_LEN_6b, WS2812B_PULSE_LEN_7b};

  for (uint32_t len = 0; len < 6; len++) {
    h.config.pulse_len_1 = lengths[len];
    memset(buf_data_expected, lengths[len], 24);
    TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected), error_msg);
  }
}

void test_first_bit_0() {
  ws2812b_led_t leds[1];
  leds[0].green = 0xaa;
  leds[0].red = 0x55;
  leds[0].blue = 0x0F;

  // Test with single packing
  ws2812b_handle_t h;
  h.led_count = 1;
  h.leds = leds;
  h.config.packing = WS2812B_PACKING_SINGLE;
  h.config.pulse_len_0 = WS2812B_PULSE_LEN_3b;
  h.config.pulse_len_1 = WS2812B_PULSE_LEN_6b;
  h.config.spi_bit_order = WS2812B_LSB_FIRST;
  h.config.prefix_len = 0;
  h.config.suffix_len = 0;

  h.config.first_bit_0 = WS2812B_FIRST_BIT_0_DISABLED;
  uint8_t p1 = 0x3F;
  uint8_t p0 = 0x07;
  uint8_t buf_data_expected[24] = {/* g=0xaa */ p1, p0, p1, p0, p1, p0, p1, p0,
                                   /* r=0x55 */ p0, p1, p0, p1, p0, p1, p0, p1,
                                   /* b=0x0f */ p0, p0, p0, p0, p1, p1, p1, p1};
  TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected), error_msg);

  h.config.first_bit_0 = WS2812B_FIRST_BIT_0_ENABLED;
  p1 = 0x7E;
  p0 = 0x0E;
  uint8_t buf_data_expected_with_0[24] = {/* g=0xaa */ p1, p0, p1, p0, p1, p0, p1, p0,
                                          /* r=0x55 */ p0, p1, p0, p1, p0, p1, p0, p1,
                                          /* b=0x0f */ p0, p0, p0, p0, p1, p1, p1, p1};

  TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected_with_0), error_msg);

  // test with double packing
  h.config.packing = WS2812B_PACKING_DOUBLE;
  h.config.pulse_len_0 = WS2812B_PULSE_LEN_1b;
  h.config.pulse_len_1 = WS2812B_PULSE_LEN_2b;

  h.config.first_bit_0 = WS2812B_FIRST_BIT_0_DISABLED;
  uint8_t p11 = 0x33;
  uint8_t p10 = 0x13;
  uint8_t p01 = 0x31;
  uint8_t p00 = 0x11;
  uint8_t buf_data_expected_double[12] = {/* g=0xaa */ p10, p10, p10, p10,
                                          /* r=0x55 */ p01, p01, p01, p01,
                                          /* b=0x0f */ p00, p00, p11, p11};
  TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected_double), error_msg);

  h.config.first_bit_0 = WS2812B_FIRST_BIT_0_ENABLED;
  p11 = 0x66;
  p10 = 0x26;
  p01 = 0x62;
  p00 = 0x22;
  uint8_t buf_data_expected_double_with_0[12] = {/* g=0xaa */ p10, p10, p10, p10,
                                                 /* r=0x55 */ p01, p01, p01, p01,
                                                 /* b=0x0f */ p00, p00, p11, p11};
  TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected_double_with_0), error_msg);
}

void test_spi_bit_order(void) {
  ws2812b_led_t leds[1];
  leds[0].green = 0xaa;
  leds[0].red = 0x55;
  leds[0].blue = 0x0F;

  // Test with single packing
  ws2812b_handle_t h;
  h.led_count = 1;
  h.leds = leds;
  h.config.packing = WS2812B_PACKING_SINGLE;
  h.config.pulse_len_0 = WS2812B_PULSE_LEN_3b;
  h.config.pulse_len_1 = WS2812B_PULSE_LEN_6b;
  h.config.first_bit_0 = WS2812B_FIRST_BIT_0_DISABLED;
  h.config.prefix_len = 0;
  h.config.suffix_len = 0;

  h.config.spi_bit_order = WS2812B_LSB_FIRST;
  uint8_t p1 = 0x3F;
  uint8_t p0 = 0x07;
  uint8_t buf_data_expected[24] = {/* g=0xaa */ p1, p0, p1, p0, p1, p0, p1, p0,
                                   /* r=0x55 */ p0, p1, p0, p1, p0, p1, p0, p1,
                                   /* b=0x0f */ p0, p0, p0, p0, p1, p1, p1, p1};
  TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected), error_msg);

  h.config.spi_bit_order = WS2812B_MSB_FIRST;
  p1 = 0xFC;
  p0 = 0xE0;
  uint8_t buf_data_expected_with_0[24] = {/* g=0xaa */ p1, p0, p1, p0, p1, p0, p1, p0,
                                          /* r=0x55 */ p0, p1, p0, p1, p0, p1, p0, p1,
                                          /* b=0x0f */ p0, p0, p0, p0, p1, p1, p1, p1};

  TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected_with_0), error_msg);

  // test with double packing
  h.config.packing = WS2812B_PACKING_DOUBLE;
  h.config.pulse_len_0 = WS2812B_PULSE_LEN_1b;
  h.config.pulse_len_1 = WS2812B_PULSE_LEN_2b;

  h.config.spi_bit_order = WS2812B_LSB_FIRST;
  uint8_t p11 = 0x33;
  uint8_t p10 = 0x13;
  uint8_t p01 = 0x31;
  uint8_t p00 = 0x11;
  uint8_t buf_data_expected_double[12] = {/* g=0xaa */ p10, p10, p10, p10,
                                          /* r=0x55 */ p01, p01, p01, p01,
                                          /* b=0x0f */ p00, p00, p11, p11};
  TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected_double), error_msg);

  h.config.spi_bit_order = WS2812B_MSB_FIRST;
  p11 = 0xCC;
  p10 = 0xC8;
  p01 = 0x8C;
  p00 = 0x88;
  uint8_t buf_data_expected_double_with_0[12] = {/* g=0xaa */ p10, p10, p10, p10,
                                                 /* r=0x55 */ p01, p01, p01, p01,
                                                 /* b=0x0f */ p00, p00, p11, p11};
  TEST_ASSERT_TRUE_MESSAGE(util_test_driver_output(&h, buf_data_expected_double_with_0), error_msg);
}

// ======== Main ===================================================================================

void setUp(void) {}
void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_no_vla);
  RUN_TEST(test_buffer_len);
  RUN_TEST(test_invalid_config_detected);
  RUN_TEST(test_iterator_end_behavior);
  RUN_TEST(test_single_led);
  RUN_TEST(test_multiple_leds);
  RUN_TEST(test_pulse_length);
  RUN_TEST(test_first_bit_0);
  RUN_TEST(test_spi_bit_order);
  return UNITY_END();
}
