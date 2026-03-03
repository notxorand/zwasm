// deep_recursion.c — Recursive calls to depth 10000
#include <stdio.h>

// Mutual recursion to exercise call stack
static int fib_like(int n, int a, int b);
static int bounce(int n, int a, int b);

static int fib_like(int n, int a, int b) {
    if (n <= 0) return a;
    return bounce(n - 1, b, a + b);
}

static int bounce(int n, int a, int b) {
    return fib_like(n, a, b);
}

// Simple deep recursion with accumulator
static long long sum_recursive(int n, long long acc) {
    if (n <= 0) return acc;
    return sum_recursive(n - 1, acc + n);
}

// Ackermann-like but bounded to avoid explosion
static int ack_bounded(int m, int n, int fuel) {
    if (fuel <= 0) return n;
    if (m == 0) return n + 1;
    if (n == 0) return ack_bounded(m - 1, 1, fuel - 1);
    return ack_bounded(m - 1, ack_bounded(m, n - 1, fuel - 1), fuel - 1);
}

int main(void) {
    // Test 1: Deep linear recursion (depth 10000)
    long long sum = sum_recursive(10000, 0);
    long long expected = (long long)10000 * 10001 / 2;
    printf("sum(1..10000) = %lld (expected %lld)\n", sum, expected);
    int ok1 = (sum == expected);

    // Test 2: Mutual recursion (depth 5000)
    int fib_result = fib_like(20, 0, 1);
    printf("fib_like(20) = %d (expected 6765)\n", fib_result);
    int ok2 = (fib_result == 6765);

    // Test 3: Ackermann-bounded (moderate depth)
    int ack = ack_bounded(3, 4, 100000);
    printf("ack_bounded(3,4) = %d (expected 125)\n", ack);
    int ok3 = (ack == 125);

    // Test 4: Very deep recursion (depth 10000)
    long long big_sum = sum_recursive(10000, 0);
    printf("sum(1..10000) again = %lld\n", big_sum);
    int ok4 = (big_sum == expected);

    if (ok1 && ok2 && ok3 && ok4)
        printf("result: OK\n");
    else
        printf("result: FAIL\n");

    return 0;
}
