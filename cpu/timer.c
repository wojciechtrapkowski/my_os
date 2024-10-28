#include "timer.h"
#include "isr.h"
#include "../drivers/screen.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../libc/function.h"
#include "rtc.h"

uint32_t tick = 0;
#define TICKS_PER_SECOND 50

static void timer_callback(registers_t* regs) {
    UNUSED(regs);
    tick++;

    if (tick % TICKS_PER_SECOND == 0) {
        print_rtc_datetime();
    }
    // PRINT TICKS
    // char tick_ascii[256];
    // int_to_ascii(tick, tick_ascii);
    // kprint_at(tick_ascii, 6, 0);
}

void init_timer(uint32_t freq) {
    /* Install the function we just wrote */
    register_interrupt_handler(IRQ0, timer_callback);
    kprint_at("Date & Time: ", 0, 0);

    /* Get the PIT value: hardware clock at 1193180 Hz */
    uint32_t divisor = 1193180 / freq;
    uint8_t low  = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)( (divisor >> 8) & 0xFF);
    /* Send the command */
    port_byte_out(0x43, 0x36); /* Command port */
    port_byte_out(0x40, low);
    port_byte_out(0x40, high);
}
