#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/uart.h"

#include "blink.pio.h"

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (125000000 / (2 * freq)) - 3;
}

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart0
#define BAUD_RATE 115200

// Use pins 12 and 13 for UART0
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 12
#define UART_RX_PIN 13

#define PICO_DEFAULT_LED_PIN 28
#define UART_ID uart0
#define BAUD_RATE 115200

int main()
{
    stdio_init_all();
    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // PIO Blinking example
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    printf("Loaded program at %d\n", offset);
    
    #ifdef PICO_DEFAULT_LED_PIN
    blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 3);
    #else
    blink_pin_forever(pio, 0, offset, 6, 3);
    #endif
    // For more pio examples see https://github.com/raspberrypi/pico-examples/tree/master/pio

    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
    
    // Send out a string, with CR/LF conversions
    uart_puts(UART_ID, " Hello, UART!\r\n");
    
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart

    int count = 10;    
    while (true) {
        if (count > 0) {
            printf("Hello, world!\n");
            count--;
        }
        sleep_ms(1000);
    }
}
