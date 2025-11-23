#include "cyonstd.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

/* Dot product of two float arrays. */
double cyon_dot(const double *a, const double *b, size_t n) {
    if (!a || !b) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) sum += a[i] * b[i];
    return sum;
}

/* Softmax in-place. Numerically stable implementation.
   Input is array of length n, output normalized probabilities in same array. */
int cyon_softmax(double *x, size_t n) {
    if (!x || n == 0) return EINVAL;
    double maxv = x[0];
    for (size_t i = 1; i < n; ++i) if (x[i] > maxv) maxv = x[i];
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        x[i] = exp(x[i] - maxv);
        sum += x[i];
    }
    if (sum == 0.0) return EDOM;
    for (size_t i = 0; i < n; ++i) x[i] /= sum;
    return 0;
}

/* Initialize array with uniform random doubles in range [lo, hi).
   Uses simple LCG for portability if OS randomness unavailable. */
static uint64_t cyon_lcg_state = 0xDEADBEEFCAFEBABEULL;

static uint64_t cyon_lcg_next(void) {
    cyon_lcg_state = cyon_lcg_state * 6364136223846793005ULL + 1ULL;
    return cyon_lcg_state;
}

void cyon_random_init(uint64_t seed) {
    if (seed == 0) seed = 0xC0FFEE123456789ULL;
    cyon_lcg_state = seed;
}

/* Fill buffer with random doubles in [lo, hi). */
int cyon_random_fill(double *buf, size_t n, double lo, double hi) {
    if (!buf || n == 0 || lo >= hi) return EINVAL;
    for (size_t i = 0; i < n; ++i) {
        uint64_t v = cyon_lcg_next();
        double u = (double)(v & 0xFFFFFFFFFFFFULL) / (double)0xFFFFFFFFFFFFULL;
        buf[i] = lo + u * (hi - lo);
    }
    return 0;
}

/* Single-layer perceptron predict: w dot x + b, returned through out.
   Activation is optional (0 = linear, 1 = sigmoid). */
int cyon_perceptron_predict(const double *w, size_t dim, const double *x, double b, int activation, double *out) {
    if (!w || !x || !out) return EINVAL;
    double z = cyon_dot(w, x, dim) + b;
    if (activation) *out = 1.0 / (1.0 + exp(-z));
    else *out = z;
    return 0;
}

/* Train perceptron with simple gradient descent for a single sample.
   Returns 0 on success. Learning rate should be small (e.g., 0.01). */
int cyon_perceptron_train_step(double *w, size_t dim, const double *x, double y_true, double *b, double lr) {
    if (!w || !x || !b) return EINVAL;
    double y_pred;
    cyon_perceptron_predict(w, dim, x, *b, 1, &y_pred);
    double err = y_pred - y_true;
    /* gradient for sigmoid cross-entropy simplified for demonstration */
    for (size_t i = 0; i < dim; ++i) w[i] -= lr * err * x[i];
    *b -= lr * err;
    return 0;
}

/* Simple mean squared error loss for regression. */
double cyon_mse_loss(const double *pred, const double *target, size_t n) {
    if (!pred || !target) return 0.0;
    double s = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double d = pred[i] - target[i];
        s += d * d;
    }
    return s / (double)n;
}

/* Tiny linear regression trainer with normal equations (X^T X)^{-1} X^T y
   Implemented for small feature count using Gaussian elimination.
   X is row-major n_samples x n_features.
   Returns 0 on success, non-zero on failure. */
int cyon_linear_regression_train(const double *X, const double *y, size_t n_samples, size_t n_features, double *out_weights) {
    if (!X || !y || !out_weights) return EINVAL;
    /* build normal matrix A = X^T X and vector b = X^T y */
    size_t m = n_features;
    double *A = (double*)calloc(m * m, sizeof(double));
    double *bvec = (double*)calloc(m, sizeof(double));
    if (!A || !bvec) { free(A); free(bvec); return ENOMEM; }

    for (size_t i = 0; i < n_samples; ++i) {
        const double *row = X + i * n_features;
        for (size_t j = 0; j < n_features; ++j) {
            for (size_t k = 0; k < n_features; ++k) {
                A[j * m + k] += row[j] * row[k];
            }
            bvec[j] += row[j] * y[i];
        }
    }

    /* Solve linear system A * w = bvec using Gaussian elimination. */
    for (size_t i = 0; i < m; ++i) {
        /* pivot */
        size_t piv = i;
        double maxv = fabs(A[i * m + i]);
        for (size_t r = i + 1; r < m; ++r) {
            double v = fabs(A[r * m + i]);
            if (v > maxv) { maxv = v; piv = r; }
        }
        if (maxv == 0.0) { free(A); free(bvec); return EDOM; }
        if (piv != i) {
            for (size_t c = i; c < m; ++c) {
                double tmp = A[i * m + c]; A[i * m + c] = A[piv * m + c]; A[piv * m + c] = tmp;
            }
            double tt = bvec[i]; bvec[i] = bvec[piv]; bvec[piv] = tt;
        }
        /* normalize pivot row */
        double diag = A[i * m + i];
        for (size_t c = i; c < m; ++c) A[i * m + c] /= diag;
        bvec[i] /= diag;
        /* eliminate */
        for (size_t r = 0; r < m; ++r) {
            if (r == i) continue;
            double factor = A[r * m + i];
            for (size_t c = i; c < m; ++c) A[r * m + c] -= factor * A[i * m + c];
            bvec[r] -= factor * bvec[i];
        }
    }

    for (size_t i = 0; i < m; ++i) out_weights[i] = bvec[i];
    free(A); free(bvec);
    return 0;
}