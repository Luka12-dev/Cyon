#include "cyonstd.h"
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

/* Simple opaque thread handle. */
typedef struct {
    pthread_t thr;
} cyon_thread_t;

/* Create thread, detach flag determines whether to detach.
   start_routine receives the provided arg. Returns 0 on success. */
int cyon_thread_create(cyon_thread_t **out_thread, void *(*start_routine)(void*), void *arg, int detach) {
    if (!out_thread || !start_routine) return EINVAL;
    cyon_thread_t *t = (cyon_thread_t*)malloc(sizeof(cyon_thread_t));
    if (!t) return ENOMEM;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (detach) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int rc = pthread_create(&t->thr, &attr, start_routine, arg);
    pthread_attr_destroy(&attr);
    if (rc != 0) { free(t); return rc; }
    *out_thread = t;
    return 0;
}

/* Join thread and free handle. Returns 0 on success. */
int cyon_thread_join(cyon_thread_t *t) {
    if (!t) return EINVAL;
    int rc = pthread_join(t->thr, NULL);
    free(t);
    return rc;
}

/* Detach an existing thread handle. Returns 0 on success. */
int cyon_thread_detach(cyon_thread_t *t) {
    if (!t) return EINVAL;
    int rc = pthread_detach(t->thr);
    return rc;
}

/* Mutex wrapper. */
typedef struct {
    pthread_mutex_t m;
} cyon_mutex_t;

/* Create mutex. Returns 0 on success. */
int cyon_mutex_create(cyon_mutex_t **out) {
    if (!out) return EINVAL;
    cyon_mutex_t *m = (cyon_mutex_t*)malloc(sizeof(cyon_mutex_t));
    if (!m) return ENOMEM;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    int rc = pthread_mutex_init(&m->m, &attr);
    pthread_mutexattr_destroy(&attr);
    if (rc != 0) { free(m); return rc; }
    *out = m;
    return 0;
}

int cyon_mutex_lock(cyon_mutex_t *m) {
    if (!m) return EINVAL;
    return pthread_mutex_lock(&m->m);
}

int cyon_mutex_unlock(cyon_mutex_t *m) {
    if (!m) return EINVAL;
    return pthread_mutex_unlock(&m->m);
}

int cyon_mutex_destroy(cyon_mutex_t *m) {
    if (!m) return EINVAL;
    int rc = pthread_mutex_destroy(&m->m);
    free(m);
    return rc;
}

/* Condition variable wrapper. */
typedef struct {
    pthread_cond_t cv;
    pthread_mutex_t m;
} cyon_cond_t;

/* Create cond. Returns 0 on success. */
int cyon_cond_create(cyon_cond_t **out) {
    if (!out) return EINVAL;
    cyon_cond_t *c = (cyon_cond_t*)malloc(sizeof(cyon_cond_t));
    if (!c) return ENOMEM;
    int rc = pthread_mutex_init(&c->m, NULL);
    if (rc != 0) { free(c); return rc; }
    rc = pthread_cond_init(&c->cv, NULL);
    if (rc != 0) { pthread_mutex_destroy(&c->m); free(c); return rc; }
    *out = c;
    return 0;
}

int cyon_cond_wait(cyon_cond_t *c) {
    if (!c) return EINVAL;
    int rc = pthread_cond_wait(&c->cv, &c->m);
    return rc;
}

int cyon_cond_timedwait(cyon_cond_t *c, const struct timespec *abstime) {
    if (!c || !abstime) return EINVAL;
    return pthread_cond_timedwait(&c->cv, &c->m, abstime);
}

int cyon_cond_signal(cyon_cond_t *c) {
    if (!c) return EINVAL;
    return pthread_cond_signal(&c->cv);
}

int cyon_cond_broadcast(cyon_cond_t *c) {
    if (!c) return EINVAL;
    return pthread_cond_broadcast(&c->cv);
}

int cyon_cond_destroy(cyon_cond_t *c) {
    if (!c) return EINVAL;
    int rc = pthread_cond_destroy(&c->cv);
    pthread_mutex_destroy(&c->m);
    free(c);
    return rc;
}

/* Simple thread pool worker task structure. */
typedef struct {
    void *(*func)(void*);
    void *arg;
} cyon_task_t;