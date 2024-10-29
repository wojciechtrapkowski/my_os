#include "string.h"
#include <stdint.h>

/**
 * K&R implementation
 */
void int_to_ascii(int n, char str[]) {
    int i, sign;
    if ((sign = n) < 0) n = -n;
    i = 0;
    do {
        str[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0) str[i++] = '-';
    str[i] = '\0';

    reverse(str);
}

void int_to_ascii_padded(int n, char *str) {
    if (n < 10) {
        str[0] = '0';
        int_to_ascii(n, str + 1);
    } else {
        int_to_ascii(n, str);
    }
}

void hex_to_ascii(int n, char str[]) {
    str[0] = '0';
    str[1] = 'x';
    str[2] = '\0'; // Marks the end of the string for append
     
    char hex_digits[] = "0123456789abcdef";
    int i, digit;
    uint8_t leading_zero = 1;
    
    for (i = 7; i >= 0; i--) {
        digit = (n >> (i * 4)) & 0xF;
        if (digit != 0 || !leading_zero || i == 0) {
            leading_zero = 0;
            append(str, hex_digits[digit]);
        }
    }
}

void reverse(char s[]) {
    int c, i, j;
    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* K&R */
int strlen(char s[]) {
    int i = 0;
    while (s[i] != '\0') ++i;
    return i;
}

void append(char s[], char n) {
    int len = strlen(s);
    s[len] = n;
    s[len+1] = '\0';
}

void backspace(char s[]) {
    int len = strlen(s);
    s[len-1] = '\0';
}

/* K&R 
 * Returns <0 if s1<s2, 0 if s1==s2, >0 if s1>s2 */
int strcmp(char s1[], char s2[]) {
    int i;
    for (i = 0; s1[i] == s2[i]; i++) {
        if (s1[i] == '\0') return 0;
    }
    return s1[i] - s2[i];
}