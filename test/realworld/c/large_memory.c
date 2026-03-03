// large_memory.c — Allocate + fill 16MB to stress memory management
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SIZE_MB 16
#define SIZE_BYTES (SIZE_MB * 1024 * 1024)

static uint32_t checksum(const uint8_t *data, size_t len) {
    uint32_t h = 0x811c9dc5;
    for (size_t i = 0; i < len; i++) {
        h ^= data[i];
        h *= 0x01000193;
    }
    return h;
}

int main(void) {
    // Allocate 16MB
    uint8_t *buf = (uint8_t *)malloc(SIZE_BYTES);
    if (!buf) {
        printf("malloc failed for %d MB\n", SIZE_MB);
        return 1;
    }
    printf("allocated %d MB\n", SIZE_MB);

    // Fill with pattern
    for (size_t i = 0; i < SIZE_BYTES; i++) {
        buf[i] = (uint8_t)((i * 7 + 13) & 0xFF);
    }
    printf("filled with pattern\n");

    // Compute checksum
    uint32_t chk1 = checksum(buf, SIZE_BYTES);
    printf("checksum = %08x\n", chk1);

    // Verify pattern
    int errors = 0;
    for (size_t i = 0; i < SIZE_BYTES; i++) {
        uint8_t expected = (uint8_t)((i * 7 + 13) & 0xFF);
        if (buf[i] != expected) {
            errors++;
            if (errors <= 3)
                printf("  mismatch at %zu: got %02x expected %02x\n",
                       i, buf[i], expected);
        }
    }
    printf("verification errors: %d\n", errors);

    // Recompute and compare
    uint32_t chk2 = checksum(buf, SIZE_BYTES);
    printf("checksum2 = %08x (match=%s)\n", chk2, chk2 == chk1 ? "yes" : "no");

    // Second allocation to test memory growth
    uint8_t *buf2 = (uint8_t *)malloc(4 * 1024 * 1024);
    if (buf2) {
        memset(buf2, 0xAA, 4 * 1024 * 1024);
        uint32_t chk3 = checksum(buf2, 4 * 1024 * 1024);
        printf("second alloc 4MB checksum = %08x\n", chk3);
        free(buf2);
    } else {
        printf("second alloc failed (ok, already have 16MB)\n");
    }

    free(buf);

    if (errors == 0 && chk1 == chk2)
        printf("result: OK\n");
    else
        printf("result: FAIL\n");

    return 0;
}
