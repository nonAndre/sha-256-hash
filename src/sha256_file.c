#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include "sha256File.h"

void digestFile(const char *filename, uint8_t *hash)
{
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    char buffer[32];
    int file = open(filename, O_RDONLY, 0);
    if (file == -1)
    {
        printf("File %s does not exist \n", filename);
        exit(1);
    }
    ssize_t bR;
    do
    {
        // read the file in chunks of 32 characters
        bR = read(file, buffer, 32);
        if (bR > 0)
        {
            SHA256_Update(&ctx, (uint8_t *)buffer, bR);
        }
        else if (bR < 0)
        {
            printf("Read failed \n");
            exit(1);
        }
    } while (bR > 0);
    SHA256_Final(hash, &ctx);

    close(file);
}