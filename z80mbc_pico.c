#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/uart.h"

#include "blink.pio.h"

// Z80 Pins
#define CLK_Pin 26
#define RD_Pin  22
#define MREQ_Pin    21
#define IORQ_Pin    20
#define RFSH_Pin    23
#define WAIT_Pin    24
#define BUSRQ_Pin   29
#define RESET_Pin   28

#define OE_Pin      14
#define WE_Pin      15

#define TEST_Pin    27

#define TOGGLE() do {    gpio_xor_mask(((uint32_t)1)<<TEST_Pin); } while(0)

#if 0
void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (125000000 / (2 * freq)) - 3;
}
#endif

void clockgen_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint phase, float divide) {
    clockgen_program_init(pio, sm, offset, pin, phase, divide);
}

void sram_control_forever(PIO pio, uint sm, uint offset) {
    sram_control_program_init(pio, sm, offset);
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

void gpio_out_init(uint gpio, bool value) {
    gpio_set_dir(gpio, GPIO_OUT);
    gpio_put(gpio, value);
    gpio_set_function(gpio, GPIO_FUNC_SIO);
}

static uint8_t ram16[16] = {
    0x76
};

static uint8_t z80_sram_cycle(int pindir, uint8_t instruction, uint8_t wr_data)
{
    uint8_t mask = pindir ? 0xff : 0;   // pindir: true: OUT, false: IN
    uint8_t data = 0;
    TOGGLE();
    while((gpio_get_all() & (1<<MREQ_Pin)));    // wait for MREQ cycle start
    if (pindir > 0) {
        // SRAM WR cycle
        gpio_set_dir_masked(0xff, 0xff);    // D0-D7 output
        gpio_put_masked(0xff, wr_data);
        gpio_put(WE_Pin, false);
        sleep_us(1);
        gpio_put(WE_Pin, true);
    } else if (pindir == 0) {
        // SRAM WR cycle
        gpio_set_dir_masked(0xff, 0);    // D0-D7 input
        gpio_put(OE_Pin, false);
        sleep_us(1);
        data = gpio_get_all() & 0xff;
        gpio_put(OE_Pin, true);
    }
    TOGGLE();
    // post process
    gpio_set_dir_masked(0xff, 0xff);    // D0-D7 output
    gpio_put_masked(0xff, instruction);
    gpio_put(WAIT_Pin, true);
    gpio_put(BUSRQ_Pin, false);
    sleep_us(10);
    gpio_set_dir_masked(0xff, 0);    // D0-D7 input
    gpio_put(WAIT_Pin, false);
    gpio_put(BUSRQ_Pin, true);
    return data;
}

void boot(uint16_t start, uint8_t *buf, int length)
{
    uint8_t data;
    gpio_out_init(OE_Pin, true);
    gpio_out_init(WE_Pin, true);

    gpio_put(WAIT_Pin, false);
    gpio_put(RESET_Pin, true);  // start it
    z80_sram_cycle(-1, 0xc3, 0);
    z80_sram_cycle(-1, (start)&0xff, 0);
    z80_sram_cycle(-1, (start>>8)&0xff, 0);
    for (int i = 0; i < sizeof ram16; ++i) {
        z80_sram_cycle(1, 0, ram16[i]);
    }
    // read sram
    gpio_put(RESET_Pin, false);
    sleep_ms(1);
    gpio_put(WAIT_Pin, false);
    gpio_put(RESET_Pin, true);  // restart it
    z80_sram_cycle(-1, 0xc3, 0);
    z80_sram_cycle(-1, (start)&0xff, 0);
    z80_sram_cycle(-1, (start>>8)&0xff, 0);
    for (int i = 0; i < sizeof ram16; ++i) {
        data = z80_sram_cycle(0, 0, 0);
        if (i % 8 == 0)
            printf("%04X ", start + i);
        printf("%02X ", data);
        if (i % 8 == 7)
            printf("\n");
    }
    gpio_put(RESET_Pin, false);  // reset again
}

int main()
{
    stdio_init_all();
    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // GPIO Out
    gpio_out_init(WAIT_Pin, true);
    gpio_out_init(RESET_Pin, false);
    gpio_out_init(BUSRQ_Pin, true);
    //gpio_out_init(INT_Pin, false);      // INT Pin has an inverter, so negate signal is needed
    gpio_out_init(OE_Pin, true);
    gpio_out_init(WE_Pin, true);

    gpio_out_init(TEST_Pin, true);
    TOGGLE();
    TOGGLE();
    //TOGGLE();

    // GPIO In
    // MREQ, IORQ, RD, RFSH, M1 are covered by PIO
    //
    gpio_init_mask(0xfff);     // D0-D7,A0-A3 input 
    gpio_set_dir_masked(0xff, 0);   // D0-D7 input
    //gpio_init(BUSAK_Pin);
    gpio_init(MREQ_Pin);
    gpio_set_pulls(MREQ_Pin, true, false);
    gpio_init(IORQ_Pin);
    gpio_set_pulls(IORQ_Pin, true, false);
    gpio_init(RFSH_Pin);
    gpio_set_pulls(RFSH_Pin, true, false);
    gpio_init(RD_Pin);
    gpio_set_pulls(RD_Pin, true, false);

    // PIO Blinking example
    PIO pio_clock = pio0;
    PIO pio_wait = pio1;
    uint sm_clock = 0;

    // pio_set_gpio_base should be invoked before pio_add_program
    uint offset1;
    offset1 = pio_add_program(pio_clock, &clockgen_program);
    printf("clockgen_program at %d\n", offset1);
    // two-phase: (4 instruction loop)
    //  16.0 ... 2.33MHz (420ns/cycle)
    //   9.42 ... 4.0MHz  (250ns/cycle)
    // single clock: (2 instruction loop)
    //  50.0 ... 1.5MHz (660-670ns) 
    //  30.0 ... 2.5MHz (400ns) ... no wait, 
    //  18.7 ... 4.0-4.17MHz (250-260ns) .... 0/1 wait in M1, 1 wait in WR, 0/1 wait in RD
    //  10.0 ... 7.1-7.6MHz (130-140ns) ... seems to work
    //   5.0 ... 14-16MHz (60-70ns) ... does not works

    clockgen_pin_forever(pio_clock, sm_clock, offset1, CLK_Pin, 1, 60.0);

    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
    
    // Send out a string, with CR/LF conversions
    uart_puts(UART_ID, " Hello, UART!\r\n");
    
    // start CPU clock
    pio_sm_set_enabled(pio_clock, sm_clock, true);
    sleep_us(20);

    // boot ... expand Z80 program to SRAM
    boot(0, &ram16[0], sizeof ram16);

    // setup sram_control PIO sm
    offset1 = pio_add_program(pio_clock, &sram_control_program);
    printf("sram_control_program at %d\n", offset1);
    sram_control_forever(pio_clock, 1, offset1);
    pio_sm_set_enabled(pio_clock, 1, true);
    // restart CPU
    gpio_put(WAIT_Pin, true);
    gpio_put(RESET_Pin, true);

    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart

    int count = 3;    
    while (true) {
        if (count > 0) {
            printf("Hello, world!\n");
            count--;
        }
        sleep_ms(1000);
    }
}
