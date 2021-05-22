/*
 * ws2812b.h
 *
 *  Created on: May 5, 2021
 *      Author: pschilk
 */

#ifndef INC_WS2812B_H_
#define INC_WS2812B_H_

#include <stdbool.h>
#include <stdint.h>

// Disable message buffer with diagnostic info for errors during init.
// saves RAM.
// #define WS2812B_DISABLE_ERROR_MSG

#ifndef WS2812B_DISABLE_ERROR_MSG
extern char *ws2812b_error_msg;
#endif

// Number of bits in a pulse
typedef enum {
  WS2812B_PULSE_LEN_1b = 0x01,
  WS2812B_PULSE_LEN_2b = 0x03,
  WS2812B_PULSE_LEN_3b = 0x07,
  WS2812B_PULSE_LEN_4b = 0x0F,
  WS2812B_PULSE_LEN_5b = 0x1F,
  WS2812B_PULSE_LEN_6b = 0x3F,
  WS2812B_PULSE_LEN_7b = 0x7F
} ws2812b_pulse_len_t;

// Enable/Disable prefixing of each byte with a 0 bit
typedef enum {
  WS2812B_FIRST_BIT_0_DISABLED = 0,
  WS2812B_FIRST_BIT_0_ENABLED = 1,
} ws2812b_first_bit_0_t;

// Pack 1 or 2 bits into a byte
typedef enum { WS2812B_PACKING_SINGLE = 1, WS2812B_PACKING_DOUBLE = 2 } ws2812b_packing_t;

// SPI Transmission order:
typedef enum { WS2812B_MSB_FIRST, WS2812B_LSB_FIRST } ws2812b_order_t;

typedef struct {
  ws2812b_packing_t packing;         // Number of bits packed into a byte.
  ws2812b_pulse_len_t pulse_len_0;   // Number of bits that make a '1' pulse.
  ws2812b_pulse_len_t pulse_len_1;   // Number of bits that make a '0' pulse.
  ws2812b_first_bit_0_t first_bit_0; // Start every byte with a zero.
  ws2812b_order_t spi_bit_order;     // SPI bit transmission order.
  uint32_t prefix_len;               // Number of zero bytes sent before every transmission.
  uint32_t suffix_len;               // Number of zero bytes sent after every transmission.
} ws2812b_config_t;

typedef struct {
  uint8_t pulse_1;
  uint8_t pulse_0;
  uint32_t iteration_index;
} ws2812b_state_t;

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} ws2812b_led_t;

typedef struct {
  ws2812b_config_t config;
  uint32_t led_count;
  ws2812b_led_t *leds;
  ws2812b_state_t state;
} ws2812b_handle_t;

#define WS2812B_REQUIRED_BUFFER_LEN(_led_count_, _packing_, _prefix_, _suffix_)                    \
  (WS2812B_DATA_LEN(_led_count_, _packing_) + (_prefix_) + (_suffix_))

#define WS2812B_DATA_LEN(_led_count_, _packing_)                                                   \
  ((_led_count_) * ((_packing_) == WS2812B_PACKING_SINGLE ? 24 : 12))

int ws2812b_init(ws2812b_handle_t *ws);

uint32_t ws2812b_required_buffer_len(ws2812b_handle_t *ws);

void ws2812b_fill_buffer(ws2812b_handle_t *ws, uint8_t *buffer);

void ws2812b_iter_restart(ws2812b_handle_t *ws);
bool ws2812b_iter_is_finished(ws2812b_handle_t *ws);
uint8_t ws2812b_iter_next(ws2812b_handle_t *ws);

#endif /* INC_WS2812bB_H_ */
