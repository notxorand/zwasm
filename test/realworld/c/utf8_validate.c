// utf8_validate.c — UTF-8 validation and codepoint counting
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    int valid;
    int codepoints;
    int bytes;
    int ascii;
    int multibyte;
} Utf8Stats;

static Utf8Stats utf8_analyze(const uint8_t *data, size_t len) {
    Utf8Stats s = {1, 0, (int)len, 0, 0};
    size_t i = 0;

    while (i < len) {
        uint8_t b = data[i];
        int seq_len;
        uint32_t cp;

        if (b <= 0x7F) {
            seq_len = 1;
            cp = b;
            s.ascii++;
        } else if ((b & 0xE0) == 0xC0) {
            seq_len = 2;
            cp = b & 0x1F;
        } else if ((b & 0xF0) == 0xE0) {
            seq_len = 3;
            cp = b & 0x0F;
        } else if ((b & 0xF8) == 0xF0) {
            seq_len = 4;
            cp = b & 0x07;
        } else {
            s.valid = 0;
            return s;
        }

        if (i + seq_len > len) {
            s.valid = 0;
            return s;
        }

        for (int j = 1; j < seq_len; j++) {
            if ((data[i + j] & 0xC0) != 0x80) {
                s.valid = 0;
                return s;
            }
            cp = (cp << 6) | (data[i + j] & 0x3F);
        }

        // Check overlong encodings
        if (seq_len == 2 && cp < 0x80) { s.valid = 0; return s; }
        if (seq_len == 3 && cp < 0x800) { s.valid = 0; return s; }
        if (seq_len == 4 && cp < 0x10000) { s.valid = 0; return s; }

        // Check surrogate range
        if (cp >= 0xD800 && cp <= 0xDFFF) { s.valid = 0; return s; }

        // Check max codepoint
        if (cp > 0x10FFFF) { s.valid = 0; return s; }

        if (seq_len > 1) s.multibyte++;
        s.codepoints++;
        i += seq_len;
    }

    return s;
}

typedef struct {
    const char *name;
    const uint8_t *data;
    size_t len;
    int expect_valid;
    int expect_codepoints;
} TestCase;

int main(void) {
    // Valid UTF-8 strings
    const uint8_t ascii[] = "Hello, World!";
    const uint8_t japanese[] = {0xE3, 0x81, 0x93, 0xE3, 0x82, 0x93, 0xE3, 0x81,
                                 0xAB, 0xE3, 0x81, 0xA1, 0xE3, 0x81, 0xAF, 0};
    // こんにちは = 5 codepoints
    const uint8_t emoji[] = {0xF0, 0x9F, 0x98, 0x80, 0xF0, 0x9F, 0x8C, 0x8D, 0};
    // 😀🌍 = 2 codepoints
    const uint8_t mixed[] = {0x41, 0xC3, 0xA9, 0xE4, 0xB8, 0xAD, 0xF0, 0x9F, 0x98, 0x80, 0};
    // Aé中😀 = 4 codepoints

    // Invalid UTF-8
    const uint8_t bad_cont[] = {0xC3, 0x28, 0};        // bad continuation
    const uint8_t overlong[] = {0xC0, 0xAF, 0};        // overlong /
    const uint8_t surrogate[] = {0xED, 0xA0, 0x80, 0}; // U+D800 surrogate
    const uint8_t trunc[] = {0xE3, 0x81, 0};           // truncated 3-byte
    const uint8_t bad_start[] = {0xFE, 0};              // invalid start byte
    const uint8_t too_big[] = {0xF4, 0x90, 0x80, 0x80, 0}; // U+110000 > max

    TestCase tests[] = {
        {"ASCII",           ascii,     13, 1, 13},
        {"Japanese",        japanese,  15, 1, 5},
        {"Emoji",           emoji,      8, 1, 2},
        {"Mixed",           mixed,     10, 1, 4},
        {"Bad continuation", bad_cont,  2, 0, -1},
        {"Overlong",        overlong,   2, 0, -1},
        {"Surrogate",       surrogate,  3, 0, -1},
        {"Truncated",       trunc,      2, 0, -1},
        {"Bad start byte",  bad_start,  1, 0, -1},
        {"Too big",         too_big,    4, 0, -1},
        {"Empty",     (const uint8_t *)"", 0, 1, 0},
    };
    int ntests = sizeof(tests) / sizeof(tests[0]);
    int pass = 0, fail = 0;

    for (int i = 0; i < ntests; i++) {
        Utf8Stats st = utf8_analyze(tests[i].data, tests[i].len);
        int ok = (st.valid == tests[i].expect_valid);
        if (ok && tests[i].expect_codepoints >= 0) {
            ok = (st.codepoints == tests[i].expect_codepoints);
        }
        if (ok) {
            pass++;
        } else {
            printf("FAIL: %s — valid=%d (exp %d), cp=%d (exp %d)\n",
                   tests[i].name, st.valid, tests[i].expect_valid,
                   st.codepoints, tests[i].expect_codepoints);
            fail++;
        }
    }

    printf("utf8 tests: %d/%d passed\n", pass, ntests);

    // Print stats for the mixed string
    Utf8Stats ms = utf8_analyze(mixed, 10);
    printf("mixed stats: %d bytes, %d codepoints, %d ascii, %d multibyte\n",
           ms.bytes, ms.codepoints, ms.ascii, ms.multibyte);

    if (fail == 0)
        printf("result: OK\n");
    else
        printf("result: FAIL (%d failures)\n", fail);

    return 0;
}
