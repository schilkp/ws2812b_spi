/*
 * ws2812.c
 *
 *  Created on: May 5, 2021
 *      Author: pschilk
 */

#include "ws2812b.h"
#include <stdint.h>

// ======== Private Macros =========================================================================

#define WS2812B_BYTE_REVERSE(_x_)                                                                  \
  (((_x_ & 0x80) >> 7) | ((_x_ & 0x40) >> 5) | ((_x_ & 0x20) >> 3) | ((_x_ & 0x10) >> 1) |         \
   ((_x_ & 0x08) << 1) | ((_x_ & 0x04) << 3) | ((_x_ & 0x02) << 5) | ((_x_ & 0x01) << 7))

#define WS2812B_NIBBLE_REVERSE(_x_)                                                                \
  (((_x_ & 0x8) >> 3) | ((_x_ & 0x4) >> 1) | ((_x_ & 0x2) << 1) | ((_x_ & 0x1) << 3))

#define WS2812B_IS_PULSE_LEN(_x_)                                                                  \
  ((_x_) == WS2812B_PULSE_LEN_1b || (_x_) == WS2812B_PULSE_LEN_2b ||                               \
   (_x_) == WS2812B_PULSE_LEN_3b || (_x_) == WS2812B_PULSE_LEN_4b ||                               \
   (_x_) == WS2812B_PULSE_LEN_6b || (_x_) == WS2812B_PULSE_LEN_5b ||                               \
   (_x_) == WS2812B_PULSE_LEN_7b)

char *ws2812b_error_msg;

#ifndef WS2812B_DISABLE_ERROR_MSG

// Set aside error message buffer unless disabled
#define WS2812B_ERROR_MSG_MAX_LEN 60
char error_msg_buf[WS2812B_ERROR_MSG_MAX_LEN];

#endif /* WS2812B_ERROR_MSG_MAX_LEN */

#define WS2812B_INIT_ASSERT(_assertion_, _error_msg_)                                              \
  do {                                                                                             \
    if (!(_assertion_)) {                                                                          \
      set_init_error_msg(_error_msg_);                                                             \
      return -1;                                                                                   \
    }                                                                                              \
  } while (0)

// ======== Private Prototypes =====================================================================

static void set_init_error_msg(const char *error_msg);
static void add_byte(ws2812b_handle_t *ws, uint8_t value, uint8_t **buffer);
static uint8_t construct_single_pulse(ws2812b_handle_t *ws, uint_fast8_t b, uint8_t value);
static uint8_t construct_double_pulse(ws2812b_handle_t *ws, uint_fast8_t b, uint8_t value);

// ======== Public Functions =======================================================================

int ws2812b_init(ws2812b_handle_t *ws) {

  // Point ws2812b_error_msg to error buffer unless error message buffer is disabled.
#ifndef WS2812B_DISABLE_ERROR_MSG
  ws2812b_error_msg = error_msg_buf;
  error_msg_buf[0] = '\0';
#else  /* WS2812B_DISABLE_ERROR_MSG */
  ws2812b_error_msg = 0;
#endif /* WS2812B_DISABLE_ERROR_MSG */

  // Assert packing is valid
  WS2812B_INIT_ASSERT((ws->config.packing == WS2812B_PACKING_DOUBLE) ||
                          (ws->config.packing == WS2812B_PACKING_SINGLE),
                      "ws2812b: config.packing is invalid!");

  // Assert pulse_len_1 is valid
  WS2812B_INIT_ASSERT(WS2812B_IS_PULSE_LEN(ws->config.pulse_len_1),
                      "ws2812b: config.pulse_len_1 is invalid!");

  // Asert pulse_len_0 is valid
  WS2812B_INIT_ASSERT(WS2812B_IS_PULSE_LEN(ws->config.pulse_len_0),
                      "ws2812b: config.pulse_len_0 is invalid!");

  // Assert first_bit_0 is valid
  WS2812B_INIT_ASSERT((ws->config.first_bit_0 == WS2812B_FIRST_BIT_0_DISABLED) ||
                          (ws->config.first_bit_0 == WS2812B_FIRST_BIT_0_ENABLED),
                      "ws2812b: config.first_bit_0 is invalid!");

  // Assert spi_bit_order is valid
  WS2812B_INIT_ASSERT((ws->config.spi_bit_order == WS2812B_LSB_FIRST) ||
                          (ws->config.spi_bit_order == WS2812B_MSB_FIRST),
                      "ws2812b: config.spi_bit_order is invalid!");

  // Assert that the '1' pulse is longer than the '0' pulse:
  WS2812B_INIT_ASSERT(ws->config.pulse_len_1 > ws->config.pulse_len_0,
                      "ws2812b: One-pulse must be longer than zero-pulse!");

  // Assert that pulse is not too long if in double packing:
  if (ws->config.packing == WS2812B_PACKING_DOUBLE) {
    WS2812B_INIT_ASSERT(ws->config.pulse_len_1 < WS2812B_PULSE_LEN_4b,
                        "ws2812b: Pulse is too long for double packing!");
  }

  // Apply 0 prefix to pulse if selected
  ws->state.pulse_0 = ws->config.pulse_len_0 << ws->config.first_bit_0;
  ws->state.pulse_1 = ws->config.pulse_len_1 << ws->config.first_bit_0;

  // Pulse needs to be reverse for MSB-first transmission:
  if (ws->config.spi_bit_order == WS2812B_MSB_FIRST) {
    if (ws->config.packing == WS2812B_PACKING_DOUBLE) {
      ws->state.pulse_0 = WS2812B_NIBBLE_REVERSE(ws->state.pulse_0);
      ws->state.pulse_1 = WS2812B_NIBBLE_REVERSE(ws->state.pulse_1);
    } else {
      ws->state.pulse_0 = WS2812B_BYTE_REVERSE(ws->state.pulse_0);
      ws->state.pulse_1 = WS2812B_BYTE_REVERSE(ws->state.pulse_1);
    }
  }

  ws->state.iteration_index = 0;

  return 0;
}

uint32_t ws2812b_required_buffer_len(ws2812b_handle_t *ws) {
  return WS2812B_REQUIRED_BUFFER_LEN(ws->led_count, ws->config.packing, ws->config.prefix_len,
                                     ws->config.suffix_len);
}

void ws2812b_fill_buffer(ws2812b_handle_t *ws, uint8_t *buffer) {
  ws2812b_led_t *led = ws->leds;

  // Add 0x00 prefix
  for (uint_fast8_t i = 0; i < ws->config.prefix_len; i++) {
    *buffer = 0x00;
    buffer++;
  }

  // Fill buffer
  for (uint32_t i = 0; i < ws->led_count; i++) {
    add_byte(ws, led->green, &buffer);
    add_byte(ws, led->red, &buffer);
    add_byte(ws, led->blue, &buffer);
    led++;
  }

  // Add 0x00 suffix
  for (uint_fast8_t i = 0; i < ws->config.suffix_len; i++) {
    *buffer = 0x00;
    buffer++;
  }
}

void ws2812b_iter_restart(ws2812b_handle_t *ws) { ws->state.iteration_index = 0; }

bool ws2812b_iter_is_finished(ws2812b_handle_t *ws) {
  // The iterator index is always handled as if in single packing mode,
  // so the iterator limit has to be calculated using single packing no matter
  // what is configued.

  const uint32_t iteration_limit = WS2812B_REQUIRED_BUFFER_LEN(
      ws->led_count, WS2812B_PACKING_SINGLE, ws->config.prefix_len, ws->config.suffix_len);

  return ws->state.iteration_index >= iteration_limit;
}

uint8_t ws2812b_iter_next(ws2812b_handle_t *ws) {
  // The iterator index is always handled as if in single packing mode,
  // so the block lengths have to be calculated using single packing no matter
  // what is configued.

  uint32_t prefix_len = ws->config.prefix_len;
  uint32_t suffix_len = ws->config.suffix_len;
  uint32_t data_len = WS2812B_DATA_LEN(ws->led_count, WS2812B_PACKING_SINGLE);

  uint32_t i = ws->state.iteration_index;

  if (i < prefix_len) {
    // In prefix
    ws->state.iteration_index++;
    return 0x00;

  } else if (i < prefix_len + data_len) {
    // In data block

    // Determined which LED, color and bit(s) should be sent:
    uint32_t led = (i - prefix_len) / 24;

    uint_fast8_t color = ((i - prefix_len) % 24) / 8;

    uint_fast8_t bit = (i - prefix_len) % 8;

    // Grab the current data byte in which the bit(s) that should
    // be sent are located:
    uint8_t data_byte;
    if (color == 0) {
      data_byte = ws->leds[led].green;
    } else if (color == 1) {
      data_byte = ws->leds[led].red;
    } else {
      data_byte = ws->leds[led].blue;
    }

    uint8_t result;
    if (ws->config.packing == WS2812B_PACKING_SINGLE) {
      // Single packing
      result = construct_single_pulse(ws, bit, data_byte);
      ws->state.iteration_index += 1;
    } else {
      // Double packing
      result = construct_double_pulse(ws, bit, data_byte);
      ws->state.iteration_index += 2;
    }

    return result;
  } else if (i < prefix_len + data_len + suffix_len) {
    // In suffix
    ws->state.iteration_index++;
    return 0x00;
  }

  // Iteration finished, return 0
  return 0x00;
}

// ======== Private Functions ======================================================================

static void set_init_error_msg(const char *error_msg) {
#ifndef WS2812B_DISABLE_ERROR_MSG
  // If error mesages are enabled, copy over the error message
  int i = 0;

  // Copy content
  while (i < WS2812B_ERROR_MSG_MAX_LEN - 1 && error_msg[i] != '\0') {
    error_msg_buf[i] = error_msg[i];
    i++;
  }

  // Terminate string
  error_msg_buf[i] = '\0';
#else
  // Otherwise avoid the unused-args warning
  (void)(error_msg);
#endif /* WS2812B_DISABLE_ERROR_MSG */
}

static void add_byte(ws2812b_handle_t *ws, uint8_t value, uint8_t **buffer) {
  if (ws->config.packing == WS2812B_PACKING_DOUBLE) {

    for (uint_fast8_t b = 0; b < 8; b = b + 2) {
      **buffer = construct_double_pulse(ws, b, value);
      ++*buffer;
    }

  } else {

    for (uint_fast8_t b = 0; b < 8; b++) {
      **buffer = construct_single_pulse(ws, b, value);
      ++*buffer;
    }
  }
}

static uint8_t construct_single_pulse(ws2812b_handle_t *ws, uint_fast8_t b, uint8_t value) {
  return (value & ((0x80U) >> b) ? ws->state.pulse_1 : ws->state.pulse_0);
}

static uint8_t construct_double_pulse(ws2812b_handle_t *ws, uint_fast8_t b, uint8_t value) {
  uint8_t result;
  uint8_t pulse_1 = ws->state.pulse_1;
  uint8_t pulse_0 = ws->state.pulse_0;

  if (ws->config.spi_bit_order == WS2812B_MSB_FIRST) {
    // MSB is first
    result = value & ((0x80U) >> (b + 1)) ? pulse_1 : pulse_0;
    result |= value & ((0x80U) >> (b)) ? (pulse_1 << 4) : (pulse_0 << 4);
  } else {
    // LSB is first
    result = value & ((0x80U) >> (b)) ? pulse_1 : pulse_0;
    result |= value & ((0x80U) >> (b + 1)) ? (pulse_1 << 4) : (pulse_0 << 4);
  }

  return result;
}
