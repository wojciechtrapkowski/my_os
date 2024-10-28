#include "rtc.h"
#include "../cpu/ports.h"
#include "../libc/string.h"
#include "../drivers/screen.h"

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

// Global variables to store time
static uint32_t seconds, minute, hour;
static uint32_t day, month, year;

static int get_update_in_progress_flag() {
    port_byte_out(CMOS_ADDR, RTC_STATUS_A);
    return (port_byte_in(CMOS_DATA) & 0x80);
}

static uint32_t get_rtc_register(uint32_t reg) {
    port_byte_out(CMOS_ADDR, reg);
    return port_byte_in(CMOS_DATA);
}

static void read_rtc() {
    uint32_t last_seconds, last_minute, last_hour;
    uint32_t last_day, last_month, last_year;
    uint32_t registerB;

    // Wait until an update isn't in progress
    while (get_update_in_progress_flag());

    seconds = get_rtc_register(RTC_SECONDS);
    minute = get_rtc_register(RTC_MINUTES);
    hour = get_rtc_register(RTC_HOURS);
    day = get_rtc_register(RTC_DAY);
    month = get_rtc_register(RTC_MONTH);
    year = get_rtc_register(RTC_YEAR);
    
    // Read until we get the same values twice
    do {
        last_seconds = seconds;
        last_minute = minute;
        last_hour = hour;
        last_day = day;
        last_month = month;
        last_year = year;

        while (get_update_in_progress_flag());
        
        seconds = get_rtc_register(RTC_SECONDS);
        minute = get_rtc_register(RTC_MINUTES);
        hour = get_rtc_register(RTC_HOURS);
        day = get_rtc_register(RTC_DAY);
        month = get_rtc_register(RTC_MONTH);
        year = get_rtc_register(RTC_YEAR);
    } while(
        last_seconds != seconds || last_minute != minute || 
        last_hour != hour || last_day != day || 
        last_month != month || last_year != year
    );

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
    
    kprint_at(time_str, 13, 0);
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
    
    kprint_at(date_str, 13, 0);
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
    
    kprint_at(datetime_str, 13, 0);
}