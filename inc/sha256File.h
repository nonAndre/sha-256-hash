#ifndef _SHA256FILE_HH
#define _SHA256FILE_HH

#include <stdint.h> // Required for uint8_t

void digestFile(const char *filename, uint8_t *hash);

#endif
