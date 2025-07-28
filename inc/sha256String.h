#ifndef _SHA256STRING_HH
#define _SHA256STRING_HH

#include <stdint.h> // Required for uint8_t

void digestString(const char *str, uint8_t *hash);

#endif
