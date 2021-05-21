/*
 * ws2812.c
 *
 *  Created on: May 5, 2021
 *      Author: pschilk
 */

#include "ws2812b.h"

#define WS2812B_BYTE_REVERSE(_x_)                                              \
  (((_x_ & 0x80) >> 7) | ((_x_ & 0x40) >> 5) | ((_x_ & 0x20) >> 3) |           \
   ((_x_ & 0x10) >> 1) | ((_x_ & 0x08) << 1) | ((_x_ & 0x04) << 3) |           \
   ((_x_ & 0x02) << 5) | ((_x_ & 0x01) << 7))
#define WS2812B_NIBBLE_REVERSE(_x_)                                            \
  (((_x_ & 0x8) >> 3) | ((_x_ & 0x4) >> 1) | ((_x_ & 0x2) << 1) |              \
   ((_x_ & 0x1) << 3))

#ifdef WS2812B_DISABLE_ERROR_MSG

#define WS2812B_INIT_ASSERT(_test_, _msg_)                                     \
  do {                                                                         \
    if (!(_test_)) {                                                           \
      return -1;                                                               \
    }                                                                          \
  } while (0)

#else

#include <stdio.h>

#define WS2812B_ERROR_MSG_MAX_LEN 200
char error_msg[WS2812B_ERROR_MSG_MAX_LEN];
char *ws2812b_error_msg;

#define WS2812B_INIT_ASSERT(_test_, _msg_)                                     \
  do {                                                                         \
    if (!(_test_)) {                                                           \
      snprintf("%s", WS2812B_ERROR_MSG_MAX_LEN, _msg_);                        \
      return -1;                                                               \
    }                                                                          \
  } while (0)

#endif /* WS2812B_DISABLE_STATIC_ASSERT */

// Private prototypes
static void add_byte(ws2812b_handle_t *ws, uint8_t value, uint8_t **buffer);
static uint8_t construct_single_pulse(ws2812b_handle_t *ws, uint_fast8_t b,
                                      uint8_t value);
static uint8_t construct_double_pulse(ws2812b_handle_t *ws, uint_fast8_t b,
                                      uint8_t value);

int ws2812b_init(ws2812b_handle_t *ws) {

#ifndef WS2812B_DISABLE_ERROR_MSG
  ws2812b_error_msg = error_msg;
#endif /* WS2812B_DISABLE_STATIC_ASSERT_STATIC_ASSERT */

  // Assert packing is valid
  WS2812B_INIT_ASSERT((ws->config.packing == WS2812B_PACKING_DOUBLE) ||
                          (ws->config.packing == WS2812B_PACKING_SINGLE),
                      "ws2812b: config.packing is invalid!");

  // Assert pulse_len_1 is valid
  // TODO

  // Asert pulse_len_0 is valid
  // TODO

  // Assert first_bit_0 is valid
  // TODO

  // Assert spi_bit_order is valid
  // TODO

  // Assert that the '1' pulse is longer than the '0' pulse:
  WS2812B_INIT_ASSERT(ws->config.pulse_len_1 > ws->config.pulse_len_0,
                      "ws2812b: One-pulse must be longer than zero-pulse!");

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

void ws2812b_iter_restart(ws2812b_handle_t *ws) {
  // TODO
}

uint_fast8_t ws2812b_iter_is_finished(ws2812b_handle_t *ws) {
  // TODO
  return 0;
}

uint8_t ws2812b_iter_next(ws2812b_handle_t *ws) {
  // TODO
  return 0;
}

// private functions
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

static uint8_t construct_single_pulse(ws2812b_handle_t *ws, uint_fast8_t b,
                                      uint8_t value) {
  return (value & ((0x80U) >> b) ? ws->state.pulse_1 : ws->state.pulse_0);
}

static uint8_t construct_double_pulse(ws2812b_handle_t *ws, uint_fast8_t b,
                                      uint8_t value) {
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
