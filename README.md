# WS2812B SPI Driver

Philipp Schilk 2021

## Usage

Allows the usage of a standard SPI peripheral to generate the PWM/One-wire signal required
by WS2812B leds.

The input of the first LED is hooked up to the MOSI pin of the SPI peripheral. No other
SPI outputs are needed

Because the exact timings of the output will depend on implementation details and
the specific device used, a number of adjustment parameters are provided.

Using these it should be possible to generate a usable signal using most SPI peripherals
that are capable of running fast enough. 

This driver will handle all data formatting. SPI transmission is platform-dependent and not provided,
as it will have to be implemented on a per-device basis.

!!! TODO: ONLY BUFFERED IS IMPLEMENTED !!!!

This driver can be used in two ways: Buffered or unbuffered.

### Buffered:

In this mode, all LED information is converted and saved into a single buffer.
This buffer can then be transmitted as a whole, for example using DMA.

This method has a larger RAM overhead: An additional 24 bytes are required per LED.

However, it is usually the easiest to use and, if used with DMA, will usually
provide the most consistent timing. 

If the RAM overhead can be tolerated, it is recommended. 

### Unbuffered:

In this mode, one byte at a time can be requested from the driver to be transmitted.

This will reduce RAM-overhead, but will usually increase interrupt load.

Furthermore the time between SPI bytes is critical and has to be consistent. Any
interrupt-based implementation must run at a high enough priority to be handeled instantly.

!!! TODO: ONLY BUFFERED IS IMPLEMENTED !!!!

## Data Format:

This driver creates the different-length pulses by sending different numbers of 1's and
0's via the SPI port.

A single pulse representing a single bit of LED information may be created using 4 or 8
bits sent via the SPI port (4-bit packing or 8-bit packing)

While 4-bit packing will need less RAM space in buffered mode and allow the SPI port 
to be run at a lower frequency, it leaves less timing margin than 8-bit packing mode. 
Therefore it is more likely for 8-bit packing to work on a give platform.

## Configuration 

The driver provides some adjustment parameters to be able to overcome hardware differences.

It is unlikely that the default settings will work right away. It is recommended to look
at the output waveform with a scope, and adjust driver parameters and SPI clock frequency
to match the timing in the datasheet.

### SPI Setup

The SPI port should be set to 8-bit transmission.

The SPI mode or clock edge/phase usually does not impact transmission, but may
on some platforms.

The SPI port should be run at approximately 6.5MHz in 8-bit packing mode and
3.2MHz in 4-bit packing mode. The exact frequency may vary alot (+/- 1MHz) and is
best determined experimentally.

By default the driver is configured for LSB-first transmission, but can
be set to MSB-first transmission using the `#define WS2812B_MSB_FIRST` setting in ws2812b.h
    
It is usually beneficially to transmit four empty bytes (0x00) to ensure the data-line starts
low. 

### Driver Setup

The exact number of bits that represent a '1' pulse or '0' pulse in both 4-bit and 8-bit
packing mode can be configured in ws2812b.h. Adjust while observing the output waveform. 

By default one empty byte will be sent at the beginning of the transmitted, and four at
the end of the transmission. This ensure that the SPI MOSI line remains low in-between
transmissions. It is unlikely that these settings need to be adjusted, but the can be in
ws2812b.h, see `WS2812B_PREFIX_LEN` and `WS2812B_SUFFIX_LEN`.

On some platforms, especially when transmitting using DMA, the first bit of the next package
will already be present on the MOSI line when the previous byte finishes. If there is any
time between byte-transmission (which is often the case), this can cause the pulses to be longer
than desired. Therefor, by default, each byte starts with a 0, and the pulse starts with the second bit.

This can behavior can be disabled using the `WS2812B_8BIT_0_PREF` and `WS2812B_4BIT_0_PREF` settings in ws2812b.h


## Usage: Buffered

- Configure the SPI port and driver as described above.
- Create an array of leds (`ws2812b_led_t`) and set the colors as desired.
- Create a `ws2812b_handle_t` handle, set the desired packing, number of LEDs, and pointer to the array of LEDs created.
- Create a `uint8_t` buffer of required length, which can be determined using the `WS2812B_REQUIRED_BUFFER_LEN(led_count, packing)` macro.
- Fill the buffer using `ws2812b_fill_buffer()`
- Transmit buffer via SPI.

To update the LEDs, the led array cam be modified, the buffer re-filled, and re-transmitted.

Make sure to respect the minimum time between packages before sending another package!

## Example: Buffered
```c
// Init SPI....

// Create array of LEDs
ws2812b_led_t leds[3];
// Set LED 0 to red
leds[0].red = 0xff;
leds[0].green = 0;
leds[0].blue = 0;

// Set LED 1 and 2 ...

// Create handle and configure
ws2812b_handle_t hws2812b;
hws2812b.packing = WS2812B_PACKING_8b;
hws2812b.led_count = 3;
hws2812b.leds = leds;

// Create buffer
uint8_t dma_buf[WS2812B_REQUIRED_BUFFER_LEN(hws2812b.led_count, hws2812b.packing)];

while (1)
{ 
    ws2812b_fill_buffer(&hws2812b, dma_buf);
    HAL_SPI_Transmit_DMA(&hspi1, dma_buf, WS2812B_REQUIRED_BUFFER_LEN(hws2812b.led_count, hws2812b.packing));
    HAL_Delay(10); // 10ms delay
}
```
## Usage: Unbuffered

TODO

## Example: Unbuffered

TODO 
