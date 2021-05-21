/*
 * ws2812b.h
 *
 *  Created on: May 5, 2021
 *      Author: pschilk
 */

#ifndef INC_WS2812B_H_
#define INC_WS2812B_H_

#include <stdint.h>

// =======================
// ==== Configuration ====
// =======================

// Important: See driver's README.md for details about these settings.

// === 8-bit packing configuration: ===

// Select number of bits that represent a '1' in 8-bit packing:
//#define WS2812B_8BIT_1_BASE 0x03   // 2 bit
//#define WS2812B_8BIT_1_BASE 0x07   // 3 bit
//#define WS2812B_8BIT_1_BASE 0x0F   // 4 bit
//#define WS2812B_8BIT_1_BASE 0x1F   // 5 bit
#define WS2812B_8BIT_1_BASE 0x3F     // 6 bit (default)
//#define WS2812B_8BIT_1_BASE 0x7F   // 7 bit

// Select number of bits that represent a '0' in 8-bit packing:
//#define WS2812B_8BIT_0_BASE 0x01 // 1 bit
//#define WS2812B_8BIT_0_BASE 0x03 // 2 bit
#define WS2812B_8BIT_0_BASE 0x07   // 3 bit (default)
//#define WS2812B_8BIT_0_BASE 0x0F // 4 bit

// Enable/disable 0 prefix by commenting the below line.
// 0 prefix is incompatible with 7-bit '1' representation.
#define WS2812_8BIT_0_PREF

// === 4-bit packing configuration: ===

// Select number of bits that represent a '1' in 4-bit packing:
#define WS2812B_4BIT_1_BASE 0x03   // 2 bit (default)
//#define WS2812B_4BIT_1_BASE 0x07 // 3 bit

// Select number of bits that represent a '0' in 4-bit packing:
#define WS2812B_4BIT_0_BASE 0x01   // 1 bit (default)
//#define WS2812B_4BIT_0_BASE 0x03 // 2 bit

// Enable/disable 0 prefix by commenting the below line.
// 0 prefix is incompatible with 3-bit '1' representation.
#define WS2812_4BIT_0_PREF

// === General configuration: ===

// Enable/Disable MSB-first SPI transmission (default is LSB-first transmission)
//#define WS2812B_MSB_FIRST

// Number of zeros added at beginning of buffer (default: 1)
#define WS2812B_PREFIX_LEN 1

// Number of zeros added at end of buffer (default: 4)
#define WS2812B_SUFFIX_LEN 4

typedef enum {
	WS2812B_PACKING_8b = 1, WS2812B_PACKING_4b = 2
} ws2812b_packing_t;

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} ws2812b_led_t;

typedef struct {
	ws2812b_packing_t packing;
	uint32_t led_count;
	ws2812b_led_t *leds;
} ws2812b_handle_t;


#define WS2812B_REQUIRED_BUFFER_LEN(led_count, packing) ((led_count*(packing == WS2812B_PACKING_8b ? 24 : 12))+WS2812B_PREFIX_LEN+WS2812B_SUFFIX_LEN)

void ws2812b_fill_buffer(ws2812b_handle_t *ws, uint8_t *buffer);

void ws2812b_iter_restart(ws2812b_handle_t *ws);
uint_fast8_t ws2812b_iter_is_finished(ws2812b_handle_t *ws);
uint8_t ws2812b_iter_next(ws2812b_handle_t *ws);

// === Attempt to warn about some invalid configurations: ===

// '0' representation needs to be defined:
#ifndef WS2812B_8BIT_0_BASE
#	error WS2812B: No 8-bit 0-representation selected.
#endif /* WS2812B_8BIT_0_BASE */

#ifndef WS2812B_4BIT_0_BASE
#	error WS2812B: No 4-bit 0-representation selected.
#endif /* WS2812B_8BIT_0_BASE */


// '1' representation needs to be defined:
#ifndef WS2812B_8BIT_1_BASE
#	error WS2812B: No 8-bit 1-representation selected.
#endif /* WS2812B_8BIT_1_BASE */

#ifndef WS2812B_4BIT_1_BASE
#	error WS2812B: No 4-bit 1-representation selected.
#endif /* WS2812B_8BIT_0_BASE */

// 8-bit 0-prefix and 7-bit '1' representation probably incompatible:
// Very unlikely this will work on most platforms, but warning can be
// disabled by commenting the warning directive.
#if WS2812B_8BIT_1_BASE == 0x7F
#	ifdef WS2812_8BIT_0_PREF
#   	warning WS2812B: 8bit: 0-prefix and 7-bit 1-representation are very likely uncompatible
#	endif /* WS2812_8BIT_0_PREF */
#endif /* WS2812B_8BIT_1_BASE == 0x7F */

// 4-bit 0-prefix and 3-bit '1' representation probably incompatible:
// Very unlikely this will work on most platforms but warning can be
// disabled by commenting the warning directive.
#if WS2812B_4BIT_1_BASE == 0x07
#	ifdef WS2812_4BIT_0_PREF
#   	warning WS2812B: 4bit: 0-prefix and 3-bit 1-representation are very likely uncompatible
#	endif /* WS2812_8BIT_0_PREF */
#endif /* WS2812B_8BIT_1_BASE == 0x7F */

#endif /* INC_WS2812bB_H_ */
