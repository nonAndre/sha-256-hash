#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include "sha256String.h"

void digestString(const char *str, uint8_t *hash)
{
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    SHA256_Update(&ctx, str, strlen(str));

    SHA256_Final(hash, &ctx);
}
