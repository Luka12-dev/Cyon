#include "cyonstd.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

/* Clamp a double between min and max. Returns clamped value. */
double cyon_clamp_double(double v, double lo, double hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* Linear interpolation between a and b by t in [0,1]. */
double cyon_lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

/* Integer greatest common divisor using binary gcd for speed. */
long long cyon_gcd_ll(long long a, long long b) {
    if (a == 0) return llabs(b);
    if (b == 0) return llabs(a);
    a = llabs(a);
    b = llabs(b);
    int shift = 0;
    while (((a | b) & 1) == 0) { a >>= 1; b >>= 1; ++shift; }
    while ((a & 1) == 0) a >>= 1;
    do {
        while ((b & 1) == 0) b >>= 1;
        if (a > b) { long long t = b; b = a; a = t; }
        b = b - a;
    } while (b != 0);
    return a << shift;
}

/* Least common multiple, returns 0 on overflow or if either is zero. */
long long cyon_lcm_ll(long long a, long long b) {
    if (a == 0 || b == 0) return 0;
    long long g = cyon_gcd_ll(a, b);
    long double tmp = (long double)(a / g) * (long double)b;
    if (tmp > (long double)LLONG_MAX) return 0;
    return (a / g) * b;
}

/* Fast integer power for non-negative exponent. */
long long cyon_ipow(long long base, unsigned int exp) {
    long long result = 1;
    while (exp) {
        if (exp & 1u) result = result * base;
        base = base * base;
        exp >>= 1u;
    }
    return result;
}

/* Factorial using iterative approach, returns 0 on overflow. */
unsigned long long cyon_factorial_u64(unsigned int n) {
    unsigned long long r = 1;
    for (unsigned int i = 2; i <= n; ++i) {
        if (r > ULLONG_MAX / i) return 0;
        r *= i;
    }
    return r;
}

/* Check primality with small deterministic Miller-Rabin bases for 64-bit. */
static unsigned long long modmul_u64(unsigned long long a, unsigned long long b, unsigned long long mod) {
    __uint128_t z = (__uint128_t)a * (__uint128_t)b;
    return (unsigned long long)(z % mod);
}

static unsigned long long modpow_u64(unsigned long long a, unsigned long long d, unsigned long long mod) {
    unsigned long long res = 1;
    while (d) {
        if (d & 1) res = modmul_u64(res, a, mod);
        a = modmul_u64(a, a, mod);
        d >>= 1;
    }
    return res;
}

int cyon_is_prime_u64(unsigned long long n) {
    if (n < 2) return 0;
    static const unsigned long long small_primes[] = {2ULL,3ULL,5ULL,7ULL,11ULL,13ULL,17ULL,19ULL,23ULL,29ULL,0};
    for (int i = 0; small_primes[i]; ++i) if (n == small_primes[i]) return 1;
    for (int i = 0; small_primes[i]; ++i) if (n % small_primes[i] == 0) return 0;

    unsigned long long d = n - 1;
    int s = 0;
    while ((d & 1) == 0) { d >>= 1; ++s; }

    unsigned long long witnesses[] = {2ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL, 9780504ULL, 1795265022ULL};
    for (size_t i = 0; i < sizeof(witnesses)/sizeof(witnesses[0]); ++i) {
        unsigned long long a = witnesses[i] % (n - 2) + 2;
        unsigned long long x = modpow_u64(a, d, n);
        if (x == 1 || x == n-1) continue;
        int cont = 0;
        for (int r = 1; r < s; ++r) {
            x = modmul_u64(x, x, n);
            if (x == n-1) { cont = 1; break; }
        }
        if (cont) continue;
        return 0;
    }
    return 1;
}

/* Sigmoid function useful in simple ML helpers. */
double cyon_sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

/* Fast approximate sqrt via Newton iterations starting from standard sqrt for refinement.
   Returns -1 on negative input. */
double cyon_sqrt_approx(double x) {
    if (x < 0.0) return -1.0;
    if (x == 0.0) return 0.0;
    double y = sqrt(x);
    /* one refinement step */
    y = 0.5 * (y + x / y);
    return y;
}