#include "rtc.h"
#include "../cpu/ports.h"
#include "../libc/string.h"
#include "../drivers/screen.h"
#include "../cpu/isr.h"    

#define CURRENT_YEAR 2024

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

// CMOS register numbers
#define RTC_SECONDS 0x00
#define RTC_MINUTES 0x02
#define RTC_HOURS   0x04
#define RTC_DAY     0x07
#define RTC_MONTH   0x08
#define RTC_YEAR    0x09
#define RTC_STATUS_A 0x0A
#define RTC_STATUS_B 0x0B
#define RTC_STATUS_C 0x0C
#define RTC_RATE 2

#define TEXT_POSITION_X 13
#define TEXT_POSITION_Y 0

// Global variables to store time
static uint32_t seconds, minute, hour;
static uint32_t day, month, year;

// We won't need that anymore, because we work with interrupts
static int get_update_in_progress_flag() {
    port_byte_out(CMOS_ADDR, RTC_STATUS_A);
    return (port_byte_in(CMOS_DATA) & 0x80);
}

static uint32_t get_rtc_register(uint32_t reg) {
    port_byte_out(CMOS_ADDR, reg);
    return port_byte_in(CMOS_DATA);
}

static void read_rtc() {
    uint32_t registerB;

    seconds = get_rtc_register(RTC_SECONDS);
    minute = get_rtc_register(RTC_MINUTES);
    hour = get_rtc_register(RTC_HOURS);
    day = get_rtc_register(RTC_DAY);
    month = get_rtc_register(RTC_MONTH);
    year = get_rtc_register(RTC_YEAR);

    registerB = get_rtc_register(RTC_STATUS_B);

    // Convert BCD to binary if necessary
    if (!(registerB & 0x04)) {
        seconds = (seconds & 0x0F) + ((seconds / 16) * 10);
        minute = (minute & 0x0F) + ((minute / 16) * 10);
        hour = ((hour & 0x0F) + (((hour & 0x70) / 16) * 10)) | (hour & 0x80);
        day = (day & 0x0F) + ((day / 16) * 10);
        month = (month & 0x0F) + ((month / 16) * 10);
        year = (year & 0x0F) + ((year / 16) * 10);
    }

    // Convert 12 hour clock to 24 hour clock if necessary
    if (!(registerB & 0x02) && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }

    // Calculate the full (4-digit) year
    year += (CURRENT_YEAR / 100) * 100;
    if(year < CURRENT_YEAR) year += 100;
}

void print_rtc_time() {
    char time_str[9];
    
    read_rtc();
    
    // Format as HH:MM:SS
    int_to_ascii(hour, time_str);
    time_str[2] = ':';
    int_to_ascii(minute, time_str + 3);
    time_str[5] = ':';
    int_to_ascii(seconds, time_str + 6);
    time_str[8] = '\0';
    
    kprint_at(time_str, TEXT_POSITION_X, TEXT_POSITION_Y);
}

void print_rtc_date() {
    char date_str[11];
    
    read_rtc();
    
    // Format as DD/MM/YYYY
    int_to_ascii(day, date_str);
    date_str[2] = '/';
    int_to_ascii(month, date_str + 3);
    date_str[5] = '/';
    int_to_ascii(year, date_str + 6);
    date_str[10] = '\0';
    
    kprint_at(date_str, TEXT_POSITION_X, TEXT_POSITION_Y);
}

void print_rtc_datetime() {
    char datetime_str[21];
    
    read_rtc();
    
    // Format as DD/MM/YYYY HH:MM:SS with leading zeros
    int_to_ascii_padded(day, datetime_str);
    datetime_str[2] = '/';
    int_to_ascii_padded(month, datetime_str + 3);
    datetime_str[5] = '/';
    int_to_ascii(year, datetime_str + 6);
    datetime_str[10] = ' ';
    int_to_ascii_padded(hour, datetime_str + 11);
    datetime_str[13] = ':';
    int_to_ascii_padded(minute, datetime_str + 14);
    datetime_str[16] = ':';
    int_to_ascii_padded(seconds, datetime_str + 17);
    datetime_str[19] = '\0';
    
    kprint_at(datetime_str, TEXT_POSITION_X, TEXT_POSITION_Y);
}

void rtc_handler(registers_t *regs) {    
    // Must read Status Register C to acknowledge the interrupt
    port_byte_out(CMOS_ADDR, RTC_STATUS_C);
    port_byte_in(CMOS_DATA);
    
    print_rtc_datetime();
    
    // Send EOI to PIC
    port_byte_out(0xA0, 0x20); // Send EOI to slave PIC
    port_byte_out(0x20, 0x20); // Send EOI to master PIC
}

void init_rtc() {
    // So the time is printed right away
    print_rtc_datetime();

    uint32_t status;
    
    // Disable interrupts temporarily
    __asm__ __volatile__("cli");
    
    // Select Status Register B and disable NMI (due to the fact, that if it happens the RTC can remain in an undefined state)
    port_byte_out(CMOS_ADDR, RTC_STATUS_B | 0x80);
    
    // Read current value
    status = port_byte_in(CMOS_DATA);
    
    // Write the previous value, but set PIE (Periodic Interrupt Enable)
    port_byte_out(CMOS_ADDR, RTC_STATUS_B | 0x80);
    port_byte_out(CMOS_DATA, status | 0x40);
    
    // Set rate (Status Register A)
    port_byte_out(CMOS_ADDR, RTC_STATUS_A);
    status = port_byte_in(CMOS_DATA);
    port_byte_out(CMOS_ADDR, RTC_STATUS_A);
    port_byte_out(CMOS_DATA, (status & 0xF0) | 0x0F & RTC_RATE); // Rate: 32768Hz >> (15-6) = 2Hz
    
    // Register our RTC handler
    register_interrupt_handler(IRQ8, rtc_handler);
    
    // Re-enable interrupts
    __asm__ __volatile__("sti");
}