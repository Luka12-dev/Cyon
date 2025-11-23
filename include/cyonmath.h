#ifndef CYONMATH_H
#define CYONMATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cyonlib.h"
#include <math.h>

/* Basic numeric utilities */
CYON_API double cyon_clamp_double(double v, double lo, double hi);
CYON_API double cyon_lerp(double a, double b, double t);

/* Integer math */
CYON_API long long cyon_gcd_ll(long long a, long long b);
CYON_API long long cyon_lcm_ll(long long a, long long b);
CYON_API long long cyon_ipow(long long base, unsigned int exp);
CYON_API unsigned long long cyon_factorial_u64(unsigned int n);

/* Primality */
CYON_API int cyon_is_prime_u64(unsigned long long n);

/* Small ML helpers */
CYON_API double cyon_sigmoid(double x);
CYON_API double cyon_sqrt_approx(double x);

#ifdef __cplusplus
}
#endif

#endif /* CYONMATH_H */