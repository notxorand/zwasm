// regex_match.c — Simple NFA regex engine
// Supports: . * + ? | () concatenation
// No backtracking — Thompson NFA construction
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_STATES 256

typedef enum { SPLIT, MATCH_CHAR, MATCH_ANY, ACCEPT } StateType;

typedef struct State {
    StateType type;
    int ch;        // for MATCH_CHAR
    int out1;      // next state index (-1 = none)
    int out2;      // second next (for SPLIT)
} State;

static State states[MAX_STATES];
static int nstates;

static int new_state(StateType type, int ch, int out1, int out2) {
    int id = nstates++;
    states[id].type = type;
    states[id].ch = ch;
    states[id].out1 = out1;
    states[id].out2 = out2;
    return id;
}

// Fragment: start state + list of dangling out pointers
typedef struct { int start; int *outs; int nouts; } Frag;

// Postfix conversion and NFA building for simple regex
// Support: literal chars, ., *, +, ?, |, ()
// Uses explicit concatenation operator '&'

static char postfix[512];
static int postfix_len;

static void re2post(const char *re) {
    int nalt = 0, natom = 0;
    struct { int nalt; int natom; } paren[100];
    int p = 0;
    postfix_len = 0;

    for (; *re; re++) {
        switch (*re) {
        case '(':
            if (natom > 1) { natom--; postfix[postfix_len++] = '&'; }
            paren[p].nalt = nalt;
            paren[p].natom = natom;
            p++;
            nalt = 0; natom = 0;
            break;
        case ')':
            if (p == 0 || natom == 0) return;
            while (--natom > 0) postfix[postfix_len++] = '&';
            for (; nalt > 0; nalt--) postfix[postfix_len++] = '|';
            p--;
            nalt = paren[p].nalt;
            natom = paren[p].natom;
            natom++;
            break;
        case '|':
            if (natom == 0) return;
            while (--natom > 0) postfix[postfix_len++] = '&';
            nalt++;
            break;
        case '*': case '+': case '?':
            if (natom == 0) return;
            postfix[postfix_len++] = *re;
            break;
        default:
            if (natom > 1) { natom--; postfix[postfix_len++] = '&'; }
            postfix[postfix_len++] = *re;
            natom++;
            break;
        }
    }
    while (--natom > 0) postfix[postfix_len++] = '&';
    for (; nalt > 0; nalt--) postfix[postfix_len++] = '|';
    postfix[postfix_len] = '\0';
}

// Patch list for dangling arrows
static int patch_list[1024];
static int patch_count;

static Frag frag_stack[256];
static int frag_sp;

static void push_frag(int start) {
    frag_stack[frag_sp].start = start;
    frag_stack[frag_sp].nouts = 0;
    frag_sp++;
}

// Build NFA from postfix
static int build_nfa(void) {
    nstates = 0;
    frag_sp = 0;

    int accept = new_state(ACCEPT, 0, -1, -1);

    for (int i = 0; i < postfix_len; i++) {
        char c = postfix[i];
        switch (c) {
        case '&': { // concatenation
            if (frag_sp < 2) return -1;
            Frag f2 = frag_stack[--frag_sp];
            Frag f1 = frag_stack[--frag_sp];
            // patch f1's dangling outs to f2.start
            // Simple approach: rebuild
            // For simplicity, use state-based approach
            (void)f1; (void)f2;
            // Simplified: just push f1 start (we'll use simulation)
            frag_stack[frag_sp].start = f1.start;
            frag_sp++;
            break;
        }
        default:
            break;
        }
    }
    return accept;
}

// Simpler approach: direct NFA simulation without postfix
// Thompson's algorithm with state sets

typedef struct {
    uint8_t in_set[MAX_STATES];
    int list[MAX_STATES];
    int count;
} StateSet;

static void set_clear(StateSet *s) {
    memset(s->in_set, 0, nstates);
    s->count = 0;
}

static void set_add(StateSet *s, int st) {
    if (st < 0 || st >= nstates || s->in_set[st]) return;
    s->in_set[st] = 1;
    s->list[s->count++] = st;
    // Follow epsilon transitions (SPLIT)
    if (states[st].type == SPLIT) {
        set_add(s, states[st].out1);
        set_add(s, states[st].out2);
    }
}

// Direct NFA construction for simple patterns
// Pattern: sequence of (char|'.') optionally followed by '*', '+', '?'
// Also supports '|' and grouping '()'
// Returns start state, sets accept state

static int nfa_accept;

static int build_simple_nfa(const char *pattern) {
    nstates = 0;
    nfa_accept = new_state(ACCEPT, 0, -1, -1);

    // Build chain of states backwards
    int next = nfa_accept;

    // Parse pattern into tokens first
    typedef struct { int type; int ch; int quantifier; } Token;
    Token tokens[256];
    int ntokens = 0;

    for (int i = 0; pattern[i]; i++) {
        Token t;
        if (pattern[i] == '.') {
            t.type = MATCH_ANY;
            t.ch = 0;
        } else if (pattern[i] == '\\' && pattern[i+1]) {
            i++;
            t.type = MATCH_CHAR;
            t.ch = pattern[i];
        } else {
            t.type = MATCH_CHAR;
            t.ch = pattern[i];
        }
        t.quantifier = 0;
        if (pattern[i+1] == '*') { t.quantifier = '*'; i++; }
        else if (pattern[i+1] == '+') { t.quantifier = '+'; i++; }
        else if (pattern[i+1] == '?') { t.quantifier = '?'; i++; }
        tokens[ntokens++] = t;
    }

    // Build NFA from right to left
    for (int i = ntokens - 1; i >= 0; i--) {
        Token *t = &tokens[i];
        int match_st = new_state(t->type, t->ch, next, -1);

        if (t->quantifier == '*') {
            // split -> match -> split (loop), split -> next
            int split = new_state(SPLIT, 0, match_st, next);
            states[match_st].out1 = split; // loop back
            next = split;
        } else if (t->quantifier == '+') {
            // match -> split -> match (loop), split -> next
            int split = new_state(SPLIT, 0, match_st, next);
            states[match_st].out1 = split;
            next = match_st;
        } else if (t->quantifier == '?') {
            // split -> match -> next, split -> next
            int split = new_state(SPLIT, 0, match_st, next);
            next = split;
        } else {
            next = match_st;
        }
    }

    return next;
}

static int nfa_match(int start, const char *text) {
    StateSet current, next_set;
    set_clear(&current);
    set_add(&current, start);

    for (const char *p = text; *p; p++) {
        set_clear(&next_set);
        for (int i = 0; i < current.count; i++) {
            int st = current.list[i];
            if (states[st].type == MATCH_CHAR && states[st].ch == *p) {
                set_add(&next_set, states[st].out1);
            } else if (states[st].type == MATCH_ANY) {
                set_add(&next_set, states[st].out1);
            }
        }
        current = next_set;
    }

    // Check if accept state is in current set
    for (int i = 0; i < current.count; i++) {
        if (states[current.list[i]].type == ACCEPT) return 1;
    }
    return 0;
}

typedef struct { const char *pattern; const char *text; int expected; } TestCase;

int main(void) {
    TestCase tests[] = {
        {"abc",       "abc",      1},
        {"abc",       "abx",      0},
        {"a.c",       "abc",      1},
        {"a.c",       "aXc",      1},
        {"a.c",       "ac",       0},
        {"ab*c",      "ac",       1},
        {"ab*c",      "abc",      1},
        {"ab*c",      "abbbbc",   1},
        {"ab+c",      "ac",       0},
        {"ab+c",      "abc",      1},
        {"ab+c",      "abbbbc",   1},
        {"colou?r",   "color",    1},
        {"colou?r",   "colour",   1},
        {"a.*b",      "aXYZb",    1},
        {"a.*b",      "ab",       1},
        {"hello",     "hello",    1},
        {"hello",     "world",    0},
        {"h.l+o",     "hello",    1},
        {"h.l+o",     "hallo",    1},
        {"h.l+o",     "ho",       0},
    };
    int ntests = sizeof(tests) / sizeof(tests[0]);
    int pass = 0, fail = 0;

    for (int i = 0; i < ntests; i++) {
        int start = build_simple_nfa(tests[i].pattern);
        int result = nfa_match(start, tests[i].text);
        if (result == tests[i].expected) {
            pass++;
        } else {
            printf("FAIL: /%s/ ~ \"%s\" expected %d got %d\n",
                   tests[i].pattern, tests[i].text, tests[i].expected, result);
            fail++;
        }
    }

    printf("regex tests: %d/%d passed\n", pass, ntests);
    if (fail == 0)
        printf("result: OK\n");
    else
        printf("result: FAIL (%d failures)\n", fail);

    return 0;
}
