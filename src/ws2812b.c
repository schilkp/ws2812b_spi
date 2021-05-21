/*
 * ws2812.c
 *
 *  Created on: May 5, 2021
 *      Author: pschilk
 */

#include "ws2812b.h"

// Private defines

// 8bit: Apply 0 prefix if selected:
#ifdef WS2812_8BIT_0_PREF
#	define WS2812B_8BIT_1_PREFIXED (WS2812B_8BIT_1_BASE << 1)
#	define WS2812B_8BIT_0_PREFIXED (WS2812B_8BIT_0_BASE << 1)
#else /* WS2812_8BIT_0_PREF */
#	define WS2812B_8BIT_1_PREFIXED (WS2812B_8BIT_1_BASE)
#	define WS2812B_8BIT_0_PREFIXED (WS2812B_8BIT_0_BASE)
#endif /* WS2812_8BIT_0_PREF */

// 4bit: Apply 0 prefix if selected:
#ifdef WS2812_4BIT_0_PREF
#	define WS2812B_4BIT_1_PREFIXED (WS2812B_4BIT_1_BASE << 1)
#	define WS2812B_4BIT_0_PREFIXED (WS2812B_4BIT_0_BASE << 1)
#else /* WS2812_4BIT_0_PREF */
#	define WS2812B_4BIT_1_PREFIXED (WS2812B_4BIT_1_BASE)
#	define WS2812B_4BIT_0_PREFIXED (WS2812B_4BIT_0_BASE)
#endif /* WS2812_4BIT_0_PREF */

// Invert to MSB-first if selected:
#define WS2812B_BYTE_REVERSE(_x_)   (((_x_&0x80) >> 7) | ((_x_&0x40) >> 5) | ((_x_&0x20) >> 3) | ((_x_&0x10) >> 1) | ((_x_&0x08) << 1) | ((_x_&0x04) << 3) | ((_x_&0x02) << 5) | ((_x_&0x01) << 7))
#define WS2812B_NIBBLE_REVERSE(_x_) (((_x_&0x8) >> 3) | ((_x_&0x4) >> 1) | ((_x_&0x2) << 1) | ((_x_&0x1) << 3))
#ifdef WS2812B_MSB_FIRST
#	define WS2812B_8BIT_1 WS2812B_BYTE_REVERSE(WS2812B_8BIT_1_PREFIXED)
#	define WS2812B_8BIT_0 WS2812B_BYTE_REVERSE(WS2812B_8BIT_1_PREFIXED)
#   define WS2812B_4BIT_1 WS2812B_NIBBLE_REVERSE(WS2812B_4BIT_1_PREFIXED)
#   define WS2812B_4BIT_0 WS2812B_NIBBLE_REVERSE(WS2812B_4BIT_0_PREFIXED)
#else /* WS2812_MSB_FIRST */
#	define WS2812B_8BIT_1 (WS2812B_8BIT_1_PREFIXED)
#	define WS2812B_8BIT_0 (WS2812B_8BIT_0_PREFIXED)
#   define WS2812B_4BIT_1 (WS2812B_4BIT_1_PREFIXED)
#   define WS2812B_4BIT_0 (WS2812B_4BIT_0_PREFIXED)
#endif /* WS2812_MSB_FIRST */

// Private prototypes
static void add_byte(ws2812b_handle_t *ws, uint8_t value, uint8_t **buffer);

static uint8_t construct_8b_bit(uint_fast8_t b, uint8_t value);
static uint8_t construct_4b_bit(uint_fast8_t b, uint8_t value);


void ws2812b_fill_buffer(ws2812b_handle_t *ws, uint8_t *buffer){
	ws2812b_led_t *led  = ws->leds;

	// Add 0x00 prefix
	for(uint_fast8_t i = 0; i < WS2812B_PREFIX_LEN; i++){
		*buffer = 0x00;
		buffer++;
	}

	// Fill buffer
	for(uint32_t i = 0; i < ws->led_count; i++){
		add_byte(ws, led->green, &buffer);
		add_byte(ws, led->red, &buffer);
		add_byte(ws, led->blue, &buffer);
		led++;
	}

	// Add 0x00 suffix
	for(uint_fast8_t i = 0; i < WS2812B_SUFFIX_LEN; i++){
		*buffer = 0x00;
		buffer++;
	}
}

void ws2812b_iter_restart(ws2812b_handle_t *ws){
	// TODO
}

uint_fast8_t ws2812b_iter_is_finished(ws2812b_handle_t *ws){
	// TODO
	return 0;
}

uint8_t ws2812b_iter_next(ws2812b_handle_t *ws){
	// TODO
	return 0;
}

// private functions
static void add_byte(ws2812b_handle_t *ws, uint8_t value, uint8_t **buffer){
	if(ws->packing == WS2812B_PACKING_4b){
		// 4b packing
		for(uint_fast8_t b = 0; b < 8; b = b+2){
			**buffer = construct_4b_bit(b, value);
			++*buffer;
		}
	} else {
		// 8b packing
		for(uint_fast8_t b = 0; b < 8; b++){
			**buffer = construct_8b_bit(b, value);
			++*buffer;
		}
	}
}

static uint8_t construct_8b_bit(uint_fast8_t b, uint8_t value){
	return (value & ((0x80U) >> b) ? WS2812B_8BIT_1:WS2812B_8BIT_0);
}

static uint8_t construct_4b_bit(uint_fast8_t b, uint8_t value){
	uint8_t result;

#	ifdef WS2812B_MSB_FIRST
		// MSB is first
		result = value & ((0x80U) >> (b+1)) ? WS2812B_4BIT_1:WS2812B_4BIT_0;
		result |= value & ((0x80U) >> (b)) ? (WS2812B_4BIT_1<<4):(WS2812B_4BIT_0<<4);
#	else /* WS2812B_MSB_FIRST */
		// LSB is first
		result = value & ((0x80U) >> (b)) ? WS2812B_4BIT_1:WS2812B_4BIT_0;
		result |= value & ((0x80U) >> (b+1)) ? (WS2812B_4BIT_1<<4):(WS2812B_4BIT_0<<4);
#	endif /* WS2812B_MSB_FIRST */

	return result;
}
