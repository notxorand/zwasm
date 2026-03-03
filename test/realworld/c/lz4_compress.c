// lz4_compress.c — Simple LZ4-style compression roundtrip
// Minimal LZ77 with 4-byte hash table for match finding
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define HASH_BITS 12
#define HASH_SIZE (1 << HASH_BITS)
#define MIN_MATCH 4
#define MAX_MATCH 255

static uint32_t hash4(const uint8_t *p) {
    uint32_t v = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                 ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    return (v * 2654435761U) >> (32 - HASH_BITS);
}

// Compressed format:
// [token] [literal_len_extra...] [literals...] [offset_lo] [offset_hi] [match_len_extra...]
// token: high 4 bits = literal length (15 = more follows)
//        low 4 bits = match length - MIN_MATCH (15 = more follows)
// If no match: offset bytes are omitted, low 4 bits = 0, indicates end-of-block

static size_t lz4_compress(const uint8_t *src, size_t src_len,
                            uint8_t *dst, size_t dst_cap) {
    if (src_len < MIN_MATCH || dst_cap < src_len + src_len / 255 + 16)
        goto store_literal;

    int hash_table[HASH_SIZE];
    memset(hash_table, -1, sizeof(hash_table));

    size_t si = 0, di = 0;
    size_t anchor = 0; // start of pending literals

    while (si + MIN_MATCH <= src_len) {
        uint32_t h = hash4(src + si);
        int ref = hash_table[h];
        hash_table[h] = (int)si;

        // Check match
        if (ref >= 0 && si - (size_t)ref <= 65535 &&
            memcmp(src + ref, src + si, MIN_MATCH) == 0) {
            // Found match — extend
            size_t match_len = MIN_MATCH;
            while (si + match_len < src_len && src[ref + match_len] == src[si + match_len]
                   && match_len < MAX_MATCH + MIN_MATCH)
                match_len++;

            size_t lit_len = si - anchor;
            uint16_t offset = (uint16_t)(si - ref);

            // Encode token
            int lit_tok = (lit_len < 15) ? (int)lit_len : 15;
            int match_tok = (match_len - MIN_MATCH < 15) ? (int)(match_len - MIN_MATCH) : 15;
            if (di >= dst_cap) return 0;
            dst[di++] = (uint8_t)((lit_tok << 4) | match_tok);

            // Extra literal length bytes
            if (lit_len >= 15) {
                size_t rem = lit_len - 15;
                while (rem >= 255) { if (di >= dst_cap) return 0; dst[di++] = 255; rem -= 255; }
                if (di >= dst_cap) return 0;
                dst[di++] = (uint8_t)rem;
            }

            // Literals
            if (di + lit_len > dst_cap) return 0;
            memcpy(dst + di, src + anchor, lit_len);
            di += lit_len;

            // Offset (little-endian)
            if (di + 2 > dst_cap) return 0;
            dst[di++] = offset & 0xFF;
            dst[di++] = (offset >> 8) & 0xFF;

            // Extra match length bytes
            if (match_len - MIN_MATCH >= 15) {
                size_t rem = match_len - MIN_MATCH - 15;
                while (rem >= 255) { if (di >= dst_cap) return 0; dst[di++] = 255; rem -= 255; }
                if (di >= dst_cap) return 0;
                dst[di++] = (uint8_t)rem;
            }

            si += match_len;
            anchor = si;
        } else {
            si++;
        }
    }

    // Final literals
    {
        size_t lit_len = src_len - anchor;
        int lit_tok = (lit_len < 15) ? (int)lit_len : 15;
        if (di >= dst_cap) return 0;
        dst[di++] = (uint8_t)(lit_tok << 4); // match_tok = 0 (no match)

        if (lit_len >= 15) {
            size_t rem = lit_len - 15;
            while (rem >= 255) { if (di >= dst_cap) return 0; dst[di++] = 255; rem -= 255; }
            if (di >= dst_cap) return 0;
            dst[di++] = (uint8_t)rem;
        }

        if (di + lit_len > dst_cap) return 0;
        memcpy(dst + di, src + anchor, lit_len);
        di += lit_len;
    }

    return di;

store_literal:
    // Store as single literal block
    {
        size_t lit_len = src_len;
        size_t di2 = 0;
        int lit_tok = (lit_len < 15) ? (int)lit_len : 15;
        dst[di2++] = (uint8_t)(lit_tok << 4);
        if (lit_len >= 15) {
            size_t rem = lit_len - 15;
            while (rem >= 255) { dst[di2++] = 255; rem -= 255; }
            dst[di2++] = (uint8_t)rem;
        }
        memcpy(dst + di2, src, src_len);
        return di2 + src_len;
    }
}

static size_t lz4_decompress(const uint8_t *src, size_t src_len,
                               uint8_t *dst, size_t dst_cap) {
    size_t si = 0, di = 0;

    while (si < src_len) {
        if (si >= src_len) break;
        uint8_t token = src[si++];
        size_t lit_len = (token >> 4) & 0xF;
        size_t match_len_base = token & 0xF;

        // Extended literal length
        if (lit_len == 15) {
            while (si < src_len) {
                uint8_t b = src[si++];
                lit_len += b;
                if (b < 255) break;
            }
        }

        // Copy literals
        if (di + lit_len > dst_cap || si + lit_len > src_len) return 0;
        memcpy(dst + di, src + si, lit_len);
        di += lit_len;
        si += lit_len;

        // Check if this was the last block (match_len_base == 0 and no more data)
        if (match_len_base == 0 && si >= src_len) break;
        if (si + 2 > src_len) break;

        // Read offset
        uint16_t offset = (uint16_t)src[si] | ((uint16_t)src[si + 1] << 8);
        si += 2;
        if (offset == 0 || di < offset) return 0;

        // Extended match length
        size_t match_len = match_len_base + MIN_MATCH;
        if (match_len_base == 15) {
            while (si < src_len) {
                uint8_t b = src[si++];
                match_len += b;
                if (b < 255) break;
            }
        }

        // Copy match (byte-by-byte for overlapping)
        size_t ref = di - offset;
        if (di + match_len > dst_cap) return 0;
        for (size_t j = 0; j < match_len; j++)
            dst[di++] = dst[ref + j];
    }

    return di;
}

static uint32_t checksum(const uint8_t *data, size_t len) {
    uint32_t h = 0x811c9dc5;
    for (size_t i = 0; i < len; i++) {
        h ^= data[i];
        h *= 0x01000193;
    }
    return h;
}

int main(void) {
    // Test data with repetition
    const char *texts[] = {
        "AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHIIIIJJJJ"
        "AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHIIIIJJJJ"
        "AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHIIIIJJJJ",
        "The quick brown fox jumps over the lazy dog. "
        "The quick brown fox jumps over the lazy dog. "
        "The quick brown fox jumps over the lazy dog. "
        "The quick brown fox jumps over the lazy dog. ",
        "abcdefghijklmnopqrstuvwxyz0123456789"
        "abcdefghijklmnopqrstuvwxyz0123456789"
        "abcdefghijklmnopqrstuvwxyz0123456789"
        "abcdefghijklmnopqrstuvwxyz0123456789",
    };
    int ntexts = sizeof(texts) / sizeof(texts[0]);
    int pass = 0;

    for (int t = 0; t < ntexts; t++) {
        const uint8_t *src = (const uint8_t *)texts[t];
        size_t src_len = strlen(texts[t]);
        uint32_t src_chk = checksum(src, src_len);

        uint8_t compressed[4096];
        size_t comp_len = lz4_compress(src, src_len, compressed, sizeof(compressed));

        uint8_t decompressed[4096];
        size_t decomp_len = lz4_decompress(compressed, comp_len,
                                            decompressed, sizeof(decompressed));

        uint32_t decomp_chk = checksum(decompressed, decomp_len);

        printf("test %d: %zu -> %zu bytes (%.1f%%), roundtrip=%s\n",
               t + 1, src_len, comp_len,
               100.0 * (double)comp_len / (double)src_len,
               (decomp_len == src_len && decomp_chk == src_chk) ? "OK" : "FAIL");

        if (decomp_len == src_len && decomp_chk == src_chk) pass++;
    }

    printf("lz4 tests: %d/%d passed\n", pass, ntexts);
    if (pass == ntexts)
        printf("result: OK\n");
    else
        printf("result: FAIL\n");

    return 0;
}
