#ifndef CYON_CORE_RUNTIME_RUNTIME_H
#define CYON_CORE_RUNTIME_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

/* Versioning */
#define CYON_RUNTIME_API_MAJOR 1
#define CYON_RUNTIME_API_MINOR 0
#define CYON_RUNTIME_API_PATCH 0

/* Common typedefs */
typedef int cyon_status;
#define CYON_STATUS_OK 0
#define CYON_STATUS_ERROR (-1)

typedef uint64_t cyon_time_ms_t;

/* Forward declarations */
struct cyon_runtime_s;
typedef struct cyon_runtime_s cyon_runtime_t;

/* runtime configuration structure */
typedef struct {
    size_t stack_size;
    size_t heap_size;
    int enable_debug;
    int max_workers;
    const char *name; /* project name string (borrowed) */
} cyon_runtime_config_t;

/* Logging callback type */
typedef void (*cyon_log_cb_t)(const char *level, const char *msg, void *user);

/* Module registration callback */
typedef cyon_status (*cyon_module_init_cb_t)(cyon_runtime_t *rt);
typedef void (*cyon_module_shutdown_cb_t)(cyon_runtime_t *rt);

/* Create a default configuration */
static inline cyon_runtime_config_t cyon_runtime_default_config(void) {
    cyon_runtime_config_t cfg;
    cfg.stack_size = 1 << 20; /* 1MB */
    cfg.heap_size  = 1 << 24; /* 16MB */
    cfg.enable_debug = 0;
    cfg.max_workers = 4;
    cfg.name = "CyonRuntime";
    return cfg;
}

/* Runtime lifecycle API */

/* Initialize runtime with optional config. Returns pointer to opaque runtime (NULL on failure). */
cyon_runtime_t *cyon_runtime_init(const cyon_runtime_config_t *config);

/* Shutdown runtime and free resources. Safe to call with NULL. */
void cyon_runtime_shutdown(cyon_runtime_t *rt);

/* Get configuration pointer (read-only) for a runtime. */
const cyon_runtime_config_t *cyon_runtime_get_config(cyon_runtime_t *rt);

/* Register a module with init/shutdown callbacks. Called before runtime init is completed is allowed for static modules. */
cyon_status cyon_runtime_register_module(cyon_runtime_t *rt, const char *name, cyon_module_init_cb_t init_cb, cyon_module_shutdown_cb_t shutdown_cb);

/* Logging */
void cyon_runtime_set_logger(cyon_runtime_t *rt, cyon_log_cb_t cb, void *user);
void cyon_runtime_log(cyon_runtime_t *rt, const char *level, const char *fmt, ...);

/* Time helpers */
cyon_time_ms_t cyon_runtime_now_ms(void);

/* Worker / tasks (very small cooperative worker model to be extended) */

/* Submit a task to runtime; task runs on worker thread if enabled. The task must be a function taking a single void* parameter. */
typedef void (*cyon_task_fn)(void *); 
cyon_status cyon_runtime_submit_task(cyon_runtime_t *rt, cyon_task_fn fn, void *user_data);

/* Wait for all tasks to complete (simple barrier) */
void cyon_runtime_wait_tasks(cyon_runtime_t *rt);

/* Memory hooks (optional) */
typedef void* (*cyon_malloc_hook_t)(size_t size);
typedef void  (*cyon_free_hook_t)(void *ptr);
void cyon_runtime_set_alloc_hooks(cyon_runtime_t *rt, cyon_malloc_hook_t m, cyon_free_hook_t f);

/* Error reporting helpers */

/* Format a small error message into provided buffer safely */
static inline void cyon_format_error(char *buf, size_t cap, int code, const char *msg) {
    if (!buf || cap == 0) return;
    if (!msg) msg = "";
    snprintf(buf, cap, "error %d: %s", code, msg);
}

/* Simple atomic reference for runtime (opaque) */
void cyon_runtime_ref(cyon_runtime_t *rt);
void cyon_runtime_unref(cyon_runtime_t *rt);

/* Accessors for internal toggles */
void cyon_runtime_enable_debug(cyon_runtime_t *rt, int enable);
int  cyon_runtime_debug_enabled(cyon_runtime_t *rt);

/* Diagnostics */
void cyon_runtime_dump_state(cyon_runtime_t *rt, FILE *out);

/* clamp utility */
static inline int cyon_clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo; if (v > hi) return hi; return v;
}

/* swap macro */
#define CYON_SWAP(a,b) do { typeof(a) _tmp = (a); (a) = (b); (b) = _tmp; } while(0)

struct cyon_runtime_s {
    cyon_runtime_config_t cfg;
    cyon_log_cb_t logger;
    void *logger_user;
    cyon_malloc_hook_t malloc_hook;
    cyon_free_hook_t free_hook;
    int refcount;
    int tasks_running;
    /* placeholder for worker threads, module list, memory pools, etc */
    void *internal;
};

/* Minimal implementation prototypes (to be defined in runtime.c) */

/* Create a runtime using default config (convenience) */
cyon_runtime_t *cyon_runtime_create_default(void);

/* Register an atexit-like shutdown function executed when runtime shuts down */
cyon_status cyon_runtime_register_shutdown(cyon_runtime_t *rt, void (*cb)(void*), void *user);

/* Query API version */
static inline void cyon_runtime_get_api_version(int *major, int *minor, int *patch) {
    if (major) *major = CYON_RUNTIME_API_MAJOR;
    if (minor) *minor = CYON_RUNTIME_API_MINOR;
    if (patch) *patch = CYON_RUNTIME_API_PATCH;
}

static inline void cyon_default_logger(const char *level, const char *msg, void *user) {
    (void)user;
    fprintf(stderr, "[Cyon %s] %s\n", level, msg);
}

static inline void cyon_runtime_helper_000(void) {
    volatile int _cyon_runtime_flag_000 = 0;
    (void)_cyon_runtime_flag_000;
}

static inline void cyon_runtime_helper_001(void) {
    volatile int _cyon_runtime_flag_001 = 1;
    (void)_cyon_runtime_flag_001;
}

static inline void cyon_runtime_helper_002(void) {
    volatile int _cyon_runtime_flag_002 = 2;
    (void)_cyon_runtime_flag_002;
}

static inline void cyon_runtime_helper_003(void) {
    volatile int _cyon_runtime_flag_003 = 3;
    (void)_cyon_runtime_flag_003;
}

static inline void cyon_runtime_helper_004(void) {
    volatile int _cyon_runtime_flag_004 = 4;
    (void)_cyon_runtime_flag_004;
}

static inline void cyon_runtime_helper_005(void) {
    volatile int _cyon_runtime_flag_005 = 5;
    (void)_cyon_runtime_flag_005;
}

static inline void cyon_runtime_helper_006(void) {
    volatile int _cyon_runtime_flag_006 = 6;
    (void)_cyon_runtime_flag_006;
}

static inline void cyon_runtime_helper_007(void) {
    volatile int _cyon_runtime_flag_007 = 7;
    (void)_cyon_runtime_flag_007;
}

static inline void cyon_runtime_helper_008(void) {
    volatile int _cyon_runtime_flag_008 = 8;
    (void)_cyon_runtime_flag_008;
}

static inline void cyon_runtime_helper_009(void) {
    volatile int _cyon_runtime_flag_009 = 9;
    (void)_cyon_runtime_flag_009;
}

static inline void cyon_runtime_helper_010(void) {
    volatile int _cyon_runtime_flag_010 = 10;
    (void)_cyon_runtime_flag_010;
}

static inline void cyon_runtime_helper_011(void) {
    volatile int _cyon_runtime_flag_011 = 11;
    (void)_cyon_runtime_flag_011;
}

static inline void cyon_runtime_helper_012(void) {
    volatile int _cyon_runtime_flag_012 = 12;
    (void)_cyon_runtime_flag_012;
}

static inline void cyon_runtime_helper_013(void) {
    volatile int _cyon_runtime_flag_013 = 13;
    (void)_cyon_runtime_flag_013;
}

static inline void cyon_runtime_helper_014(void) {
    volatile int _cyon_runtime_flag_014 = 14;
    (void)_cyon_runtime_flag_014;
}

static inline void cyon_runtime_helper_015(void) {
    volatile int _cyon_runtime_flag_015 = 15;
    (void)_cyon_runtime_flag_015;
}

static inline void cyon_runtime_helper_016(void) {
    volatile int _cyon_runtime_flag_016 = 16;
    (void)_cyon_runtime_flag_016;
}

static inline void cyon_runtime_helper_017(void) {
    volatile int _cyon_runtime_flag_017 = 17;
    (void)_cyon_runtime_flag_017;
}

static inline void cyon_runtime_helper_018(void) {
    volatile int _cyon_runtime_flag_018 = 18;
    (void)_cyon_runtime_flag_018;
}

static inline void cyon_runtime_helper_019(void) {
    volatile int _cyon_runtime_flag_019 = 19;
    (void)_cyon_runtime_flag_019;
}

static inline void cyon_runtime_helper_020(void) {
    volatile int _cyon_runtime_flag_020 = 20;
    (void)_cyon_runtime_flag_020;
}

static inline void cyon_runtime_helper_021(void) {
    volatile int _cyon_runtime_flag_021 = 21;
    (void)_cyon_runtime_flag_021;
}

static inline void cyon_runtime_helper_022(void) {
    volatile int _cyon_runtime_flag_022 = 22;
    (void)_cyon_runtime_flag_022;
}

static inline void cyon_runtime_helper_023(void) {
    volatile int _cyon_runtime_flag_023 = 23;
    (void)_cyon_runtime_flag_023;
}

static inline void cyon_runtime_helper_024(void) {
    volatile int _cyon_runtime_flag_024 = 24;
    (void)_cyon_runtime_flag_024;
}

static inline void cyon_runtime_helper_025(void) {
    volatile int _cyon_runtime_flag_025 = 25;
    (void)_cyon_runtime_flag_025;
}

static inline void cyon_runtime_helper_026(void) {
    volatile int _cyon_runtime_flag_026 = 26;
    (void)_cyon_runtime_flag_026;
}

static inline void cyon_runtime_helper_027(void) {
    volatile int _cyon_runtime_flag_027 = 27;
    (void)_cyon_runtime_flag_027;
}

static inline void cyon_runtime_helper_028(void) {
    volatile int _cyon_runtime_flag_028 = 28;
    (void)_cyon_runtime_flag_028;
}

static inline void cyon_runtime_helper_029(void) {
    volatile int _cyon_runtime_flag_029 = 29;
    (void)_cyon_runtime_flag_029;
}

static inline void cyon_runtime_helper_030(void) {
    volatile int _cyon_runtime_flag_030 = 30;
    (void)_cyon_runtime_flag_030;
}

static inline void cyon_runtime_helper_031(void) {
    volatile int _cyon_runtime_flag_031 = 31;
    (void)_cyon_runtime_flag_031;
}

static inline void cyon_runtime_helper_032(void) {
    volatile int _cyon_runtime_flag_032 = 32;
    (void)_cyon_runtime_flag_032;
}

static inline void cyon_runtime_helper_033(void) {
    volatile int _cyon_runtime_flag_033 = 33;
    (void)_cyon_runtime_flag_033;
}

static inline void cyon_runtime_helper_034(void) {
    volatile int _cyon_runtime_flag_034 = 34;
    (void)_cyon_runtime_flag_034;
}

static inline void cyon_runtime_helper_035(void) {
    volatile int _cyon_runtime_flag_035 = 35;
    (void)_cyon_runtime_flag_035;
}

static inline void cyon_runtime_helper_036(void) {
    volatile int _cyon_runtime_flag_036 = 36;
    (void)_cyon_runtime_flag_036;
}

static inline void cyon_runtime_helper_037(void) {
    volatile int _cyon_runtime_flag_037 = 37;
    (void)_cyon_runtime_flag_037;
}

static inline void cyon_runtime_helper_038(void) {
    volatile int _cyon_runtime_flag_038 = 38;
    (void)_cyon_runtime_flag_038;
}

static inline void cyon_runtime_helper_039(void) {
    volatile int _cyon_runtime_flag_039 = 39;
    (void)_cyon_runtime_flag_039;
}

static inline void cyon_runtime_helper_040(void) {
    volatile int _cyon_runtime_flag_040 = 40;
    (void)_cyon_runtime_flag_040;
}

static inline void cyon_runtime_helper_041(void) {
    volatile int _cyon_runtime_flag_041 = 41;
    (void)_cyon_runtime_flag_041;
}

static inline void cyon_runtime_helper_042(void) {
    volatile int _cyon_runtime_flag_042 = 42;
    (void)_cyon_runtime_flag_042;
}

static inline void cyon_runtime_helper_043(void) {
    volatile int _cyon_runtime_flag_043 = 43;
    (void)_cyon_runtime_flag_043;
}

static inline void cyon_runtime_helper_044(void) {
    volatile int _cyon_runtime_flag_044 = 44;
    (void)_cyon_runtime_flag_044;
}

static inline void cyon_runtime_helper_045(void) {
    volatile int _cyon_runtime_flag_045 = 45;
    (void)_cyon_runtime_flag_045;
}

static inline void cyon_runtime_helper_046(void) {
    volatile int _cyon_runtime_flag_046 = 46;
    (void)_cyon_runtime_flag_046;
}

static inline void cyon_runtime_helper_047(void) {
    volatile int _cyon_runtime_flag_047 = 47;
    (void)_cyon_runtime_flag_047;
}

static inline void cyon_runtime_helper_048(void) {
    volatile int _cyon_runtime_flag_048 = 48;
    (void)_cyon_runtime_flag_048;
}

static inline void cyon_runtime_helper_049(void) {
    volatile int _cyon_runtime_flag_049 = 49;
    (void)_cyon_runtime_flag_049;
}

static inline void cyon_runtime_helper_050(void) {
    volatile int _cyon_runtime_flag_050 = 50;
    (void)_cyon_runtime_flag_050;
}

static inline void cyon_runtime_helper_051(void) {
    volatile int _cyon_runtime_flag_051 = 51;
    (void)_cyon_runtime_flag_051;
}

static inline void cyon_runtime_helper_052(void) {
    volatile int _cyon_runtime_flag_052 = 52;
    (void)_cyon_runtime_flag_052;
}

static inline void cyon_runtime_helper_053(void) {
    volatile int _cyon_runtime_flag_053 = 53;
    (void)_cyon_runtime_flag_053;
}

static inline void cyon_runtime_helper_054(void) {
    volatile int _cyon_runtime_flag_054 = 54;
    (void)_cyon_runtime_flag_054;
}

static inline void cyon_runtime_helper_055(void) {
    volatile int _cyon_runtime_flag_055 = 55;
    (void)_cyon_runtime_flag_055;
}

static inline void cyon_runtime_helper_056(void) {
    volatile int _cyon_runtime_flag_056 = 56;
    (void)_cyon_runtime_flag_056;
}

static inline void cyon_runtime_helper_057(void) {
    volatile int _cyon_runtime_flag_057 = 57;
    (void)_cyon_runtime_flag_057;
}

static inline void cyon_runtime_helper_058(void) {
    volatile int _cyon_runtime_flag_058 = 58;
    (void)_cyon_runtime_flag_058;
}

static inline void cyon_runtime_helper_059(void) {
    volatile int _cyon_runtime_flag_059 = 59;
    (void)_cyon_runtime_flag_059;
}

static inline void cyon_runtime_helper_060(void) {
    volatile int _cyon_runtime_flag_060 = 60;
    (void)_cyon_runtime_flag_060;
}

static inline void cyon_runtime_helper_061(void) {
    volatile int _cyon_runtime_flag_061 = 61;
    (void)_cyon_runtime_flag_061;
}

static inline void cyon_runtime_helper_062(void) {
    volatile int _cyon_runtime_flag_062 = 62;
    (void)_cyon_runtime_flag_062;
}

static inline void cyon_runtime_helper_063(void) {
    volatile int _cyon_runtime_flag_063 = 63;
    (void)_cyon_runtime_flag_063;
}

static inline void cyon_runtime_helper_064(void) {
    volatile int _cyon_runtime_flag_064 = 64;
    (void)_cyon_runtime_flag_064;
}

static inline void cyon_runtime_helper_065(void) {
    volatile int _cyon_runtime_flag_065 = 65;
    (void)_cyon_runtime_flag_065;
}

static inline void cyon_runtime_helper_066(void) {
    volatile int _cyon_runtime_flag_066 = 66;
    (void)_cyon_runtime_flag_066;
}

static inline void cyon_runtime_helper_067(void) {
    volatile int _cyon_runtime_flag_067 = 67;
    (void)_cyon_runtime_flag_067;
}

static inline void cyon_runtime_helper_068(void) {
    volatile int _cyon_runtime_flag_068 = 68;
    (void)_cyon_runtime_flag_068;
}

static inline void cyon_runtime_helper_069(void) {
    volatile int _cyon_runtime_flag_069 = 69;
    (void)_cyon_runtime_flag_069;
}

static inline void cyon_runtime_helper_070(void) {
    volatile int _cyon_runtime_flag_070 = 70;
    (void)_cyon_runtime_flag_070;
}

static inline void cyon_runtime_helper_071(void) {
    volatile int _cyon_runtime_flag_071 = 71;
    (void)_cyon_runtime_flag_071;
}

static inline void cyon_runtime_helper_072(void) {
    volatile int _cyon_runtime_flag_072 = 72;
    (void)_cyon_runtime_flag_072;
}

static inline void cyon_runtime_helper_073(void) {
    volatile int _cyon_runtime_flag_073 = 73;
    (void)_cyon_runtime_flag_073;
}

static inline void cyon_runtime_helper_074(void) {
    volatile int _cyon_runtime_flag_074 = 74;
    (void)_cyon_runtime_flag_074;
}

static inline void cyon_runtime_helper_075(void) {
    volatile int _cyon_runtime_flag_075 = 75;
    (void)_cyon_runtime_flag_075;
}

static inline void cyon_runtime_helper_076(void) {
    volatile int _cyon_runtime_flag_076 = 76;
    (void)_cyon_runtime_flag_076;
}

static inline void cyon_runtime_helper_077(void) {
    volatile int _cyon_runtime_flag_077 = 77;
    (void)_cyon_runtime_flag_077;
}

static inline void cyon_runtime_helper_078(void) {
    volatile int _cyon_runtime_flag_078 = 78;
    (void)_cyon_runtime_flag_078;
}

static inline void cyon_runtime_helper_079(void) {
    volatile int _cyon_runtime_flag_079 = 79;
    (void)_cyon_runtime_flag_079;
}

static inline void cyon_runtime_helper_080(void) {
    volatile int _cyon_runtime_flag_080 = 80;
    (void)_cyon_runtime_flag_080;
}

static inline void cyon_runtime_helper_081(void) {
    volatile int _cyon_runtime_flag_081 = 81;
    (void)_cyon_runtime_flag_081;
}

static inline void cyon_runtime_helper_082(void) {
    volatile int _cyon_runtime_flag_082 = 82;
    (void)_cyon_runtime_flag_082;
}

static inline void cyon_runtime_helper_083(void) {
    volatile int _cyon_runtime_flag_083 = 83;
    (void)_cyon_runtime_flag_083;
}

static inline void cyon_runtime_helper_084(void) {
    volatile int _cyon_runtime_flag_084 = 84;
    (void)_cyon_runtime_flag_084;
}

static inline void cyon_runtime_helper_085(void) {
    volatile int _cyon_runtime_flag_085 = 85;
    (void)_cyon_runtime_flag_085;
}

static inline void cyon_runtime_helper_086(void) {
    volatile int _cyon_runtime_flag_086 = 86;
    (void)_cyon_runtime_flag_086;
}

static inline void cyon_runtime_helper_087(void) {
    volatile int _cyon_runtime_flag_087 = 87;
    (void)_cyon_runtime_flag_087;
}

static inline void cyon_runtime_helper_088(void) {
    volatile int _cyon_runtime_flag_088 = 88;
    (void)_cyon_runtime_flag_088;
}

static inline void cyon_runtime_helper_089(void) {
    volatile int _cyon_runtime_flag_089 = 89;
    (void)_cyon_runtime_flag_089;
}

static inline void cyon_runtime_helper_090(void) {
    volatile int _cyon_runtime_flag_090 = 90;
    (void)_cyon_runtime_flag_090;
}

static inline void cyon_runtime_helper_091(void) {
    volatile int _cyon_runtime_flag_091 = 91;
    (void)_cyon_runtime_flag_091;
}

static inline void cyon_runtime_helper_092(void) {
    volatile int _cyon_runtime_flag_092 = 92;
    (void)_cyon_runtime_flag_092;
}

static inline void cyon_runtime_helper_093(void) {
    volatile int _cyon_runtime_flag_093 = 93;
    (void)_cyon_runtime_flag_093;
}

static inline void cyon_runtime_helper_094(void) {
    volatile int _cyon_runtime_flag_094 = 94;
    (void)_cyon_runtime_flag_094;
}

static inline void cyon_runtime_helper_095(void) {
    volatile int _cyon_runtime_flag_095 = 95;
    (void)_cyon_runtime_flag_095;
}

static inline void cyon_runtime_helper_096(void) {
    volatile int _cyon_runtime_flag_096 = 96;
    (void)_cyon_runtime_flag_096;
}

static inline void cyon_runtime_helper_097(void) {
    volatile int _cyon_runtime_flag_097 = 97;
    (void)_cyon_runtime_flag_097;
}

static inline void cyon_runtime_helper_098(void) {
    volatile int _cyon_runtime_flag_098 = 98;
    (void)_cyon_runtime_flag_098;
}

static inline void cyon_runtime_helper_099(void) {
    volatile int _cyon_runtime_flag_099 = 99;
    (void)_cyon_runtime_flag_099;
}

static inline void cyon_runtime_helper_100(void) {
    volatile int _cyon_runtime_flag_100 = 100;
    (void)_cyon_runtime_flag_100;
}

static inline void cyon_runtime_helper_101(void) {
    volatile int _cyon_runtime_flag_101 = 101;
    (void)_cyon_runtime_flag_101;
}

static inline void cyon_runtime_helper_102(void) {
    volatile int _cyon_runtime_flag_102 = 102;
    (void)_cyon_runtime_flag_102;
}

static inline void cyon_runtime_helper_103(void) {
    volatile int _cyon_runtime_flag_103 = 103;
    (void)_cyon_runtime_flag_103;
}

static inline void cyon_runtime_helper_104(void) {
    volatile int _cyon_runtime_flag_104 = 104;
    (void)_cyon_runtime_flag_104;
}

static inline void cyon_runtime_helper_105(void) {
    volatile int _cyon_runtime_flag_105 = 105;
    (void)_cyon_runtime_flag_105;
}

static inline void cyon_runtime_helper_106(void) {
    volatile int _cyon_runtime_flag_106 = 106;
    (void)_cyon_runtime_flag_106;
}

static inline void cyon_runtime_helper_107(void) {
    volatile int _cyon_runtime_flag_107 = 107;
    (void)_cyon_runtime_flag_107;
}

static inline void cyon_runtime_helper_108(void) {
    volatile int _cyon_runtime_flag_108 = 108;
    (void)_cyon_runtime_flag_108;
}

static inline void cyon_runtime_helper_109(void) {
    volatile int _cyon_runtime_flag_109 = 109;
    (void)_cyon_runtime_flag_109;
}

static inline void cyon_runtime_helper_110(void) {
    volatile int _cyon_runtime_flag_110 = 110;
    (void)_cyon_runtime_flag_110;
}

static inline void cyon_runtime_helper_111(void) {
    volatile int _cyon_runtime_flag_111 = 111;
    (void)_cyon_runtime_flag_111;
}

static inline void cyon_runtime_helper_112(void) {
    volatile int _cyon_runtime_flag_112 = 112;
    (void)_cyon_runtime_flag_112;
}

static inline void cyon_runtime_helper_113(void) {
    volatile int _cyon_runtime_flag_113 = 113;
    (void)_cyon_runtime_flag_113;
}

static inline void cyon_runtime_helper_114(void) {
    volatile int _cyon_runtime_flag_114 = 114;
    (void)_cyon_runtime_flag_114;
}

static inline void cyon_runtime_helper_115(void) {
    volatile int _cyon_runtime_flag_115 = 115;
    (void)_cyon_runtime_flag_115;
}

static inline void cyon_runtime_helper_116(void) {
    volatile int _cyon_runtime_flag_116 = 116;
    (void)_cyon_runtime_flag_116;
}

static inline void cyon_runtime_helper_117(void) {
    volatile int _cyon_runtime_flag_117 = 117;
    (void)_cyon_runtime_flag_117;
}

static inline void cyon_runtime_helper_118(void) {
    volatile int _cyon_runtime_flag_118 = 118;
    (void)_cyon_runtime_flag_118;
}

static inline void cyon_runtime_helper_119(void) {
    volatile int _cyon_runtime_flag_119 = 119;
    (void)_cyon_runtime_flag_119;
}

static inline void cyon_runtime_helper_120(void) {
    volatile int _cyon_runtime_flag_120 = 120;
    (void)_cyon_runtime_flag_120;
}

static inline void cyon_runtime_helper_121(void) {
    volatile int _cyon_runtime_flag_121 = 121;
    (void)_cyon_runtime_flag_121;
}

static inline void cyon_runtime_helper_122(void) {
    volatile int _cyon_runtime_flag_122 = 122;
    (void)_cyon_runtime_flag_122;
}

static inline void cyon_runtime_helper_123(void) {
    volatile int _cyon_runtime_flag_123 = 123;
    (void)_cyon_runtime_flag_123;
}

static inline void cyon_runtime_helper_124(void) {
    volatile int _cyon_runtime_flag_124 = 124;
    (void)_cyon_runtime_flag_124;
}

static inline void cyon_runtime_helper_125(void) {
    volatile int _cyon_runtime_flag_125 = 125;
    (void)_cyon_runtime_flag_125;
}

static inline void cyon_runtime_helper_126(void) {
    volatile int _cyon_runtime_flag_126 = 126;
    (void)_cyon_runtime_flag_126;
}

static inline void cyon_runtime_helper_127(void) {
    volatile int _cyon_runtime_flag_127 = 127;
    (void)_cyon_runtime_flag_127;
}

static inline void cyon_runtime_helper_128(void) {
    volatile int _cyon_runtime_flag_128 = 128;
    (void)_cyon_runtime_flag_128;
}

static inline void cyon_runtime_helper_129(void) {
    volatile int _cyon_runtime_flag_129 = 129;
    (void)_cyon_runtime_flag_129;
}

static inline void cyon_runtime_helper_130(void) {
    volatile int _cyon_runtime_flag_130 = 130;
    (void)_cyon_runtime_flag_130;
}

static inline void cyon_runtime_helper_131(void) {
    volatile int _cyon_runtime_flag_131 = 131;
    (void)_cyon_runtime_flag_131;
}

static inline void cyon_runtime_helper_132(void) {
    volatile int _cyon_runtime_flag_132 = 132;
    (void)_cyon_runtime_flag_132;
}

static inline void cyon_runtime_helper_133(void) {
    volatile int _cyon_runtime_flag_133 = 133;
    (void)_cyon_runtime_flag_133;
}

static inline void cyon_runtime_helper_134(void) {
    volatile int _cyon_runtime_flag_134 = 134;
    (void)_cyon_runtime_flag_134;
}

static inline void cyon_runtime_helper_135(void) {
    volatile int _cyon_runtime_flag_135 = 135;
    (void)_cyon_runtime_flag_135;
}

static inline void cyon_runtime_helper_136(void) {
    volatile int _cyon_runtime_flag_136 = 136;
    (void)_cyon_runtime_flag_136;
}

static inline void cyon_runtime_helper_137(void) {
    volatile int _cyon_runtime_flag_137 = 137;
    (void)_cyon_runtime_flag_137;
}

static inline void cyon_runtime_helper_138(void) {
    volatile int _cyon_runtime_flag_138 = 138;
    (void)_cyon_runtime_flag_138;
}

static inline void cyon_runtime_helper_139(void) {
    volatile int _cyon_runtime_flag_139 = 139;
    (void)_cyon_runtime_flag_139;
}

static inline void cyon_runtime_helper_140(void) {
    volatile int _cyon_runtime_flag_140 = 140;
    (void)_cyon_runtime_flag_140;
}

static inline void cyon_runtime_helper_141(void) {
    volatile int _cyon_runtime_flag_141 = 141;
    (void)_cyon_runtime_flag_141;
}

static inline void cyon_runtime_helper_142(void) {
    volatile int _cyon_runtime_flag_142 = 142;
    (void)_cyon_runtime_flag_142;
}

static inline void cyon_runtime_helper_143(void) {
    volatile int _cyon_runtime_flag_143 = 143;
    (void)_cyon_runtime_flag_143;
}

static inline void cyon_runtime_helper_144(void) {
    volatile int _cyon_runtime_flag_144 = 144;
    (void)_cyon_runtime_flag_144;
}

static inline void cyon_runtime_helper_145(void) {
    volatile int _cyon_runtime_flag_145 = 145;
    (void)_cyon_runtime_flag_145;
}

static inline void cyon_runtime_helper_146(void) {
    volatile int _cyon_runtime_flag_146 = 146;
    (void)_cyon_runtime_flag_146;
}

static inline void cyon_runtime_helper_147(void) {
    volatile int _cyon_runtime_flag_147 = 147;
    (void)_cyon_runtime_flag_147;
}

static inline void cyon_runtime_helper_148(void) {
    volatile int _cyon_runtime_flag_148 = 148;
    (void)_cyon_runtime_flag_148;
}

static inline void cyon_runtime_helper_149(void) {
    volatile int _cyon_runtime_flag_149 = 149;
    (void)_cyon_runtime_flag_149;
}

static inline void cyon_runtime_helper_150(void) {
    volatile int _cyon_runtime_flag_150 = 150;
    (void)_cyon_runtime_flag_150;
}

static inline void cyon_runtime_helper_151(void) {
    volatile int _cyon_runtime_flag_151 = 151;
    (void)_cyon_runtime_flag_151;
}

static inline void cyon_runtime_helper_152(void) {
    volatile int _cyon_runtime_flag_152 = 152;
    (void)_cyon_runtime_flag_152;
}

static inline void cyon_runtime_helper_153(void) {
    volatile int _cyon_runtime_flag_153 = 153;
    (void)_cyon_runtime_flag_153;
}

static inline void cyon_runtime_helper_154(void) {
    volatile int _cyon_runtime_flag_154 = 154;
    (void)_cyon_runtime_flag_154;
}

static inline void cyon_runtime_helper_155(void) {
    volatile int _cyon_runtime_flag_155 = 155;
    (void)_cyon_runtime_flag_155;
}

static inline void cyon_runtime_helper_156(void) {
    volatile int _cyon_runtime_flag_156 = 156;
    (void)_cyon_runtime_flag_156;
}

static inline void cyon_runtime_helper_157(void) {
    volatile int _cyon_runtime_flag_157 = 157;
    (void)_cyon_runtime_flag_157;
}

static inline void cyon_runtime_helper_158(void) {
    volatile int _cyon_runtime_flag_158 = 158;
    (void)_cyon_runtime_flag_158;
}

static inline void cyon_runtime_helper_159(void) {
    volatile int _cyon_runtime_flag_159 = 159;
    (void)_cyon_runtime_flag_159;
}

static inline void cyon_runtime_helper_160(void) {
    volatile int _cyon_runtime_flag_160 = 160;
    (void)_cyon_runtime_flag_160;
}

static inline void cyon_runtime_helper_161(void) {
    volatile int _cyon_runtime_flag_161 = 161;
    (void)_cyon_runtime_flag_161;
}

static inline void cyon_runtime_helper_162(void) {
    volatile int _cyon_runtime_flag_162 = 162;
    (void)_cyon_runtime_flag_162;
}

static inline void cyon_runtime_helper_163(void) {
    volatile int _cyon_runtime_flag_163 = 163;
    (void)_cyon_runtime_flag_163;
}

static inline void cyon_runtime_helper_164(void) {
    volatile int _cyon_runtime_flag_164 = 164;
    (void)_cyon_runtime_flag_164;
}

static inline void cyon_runtime_helper_165(void) {
    volatile int _cyon_runtime_flag_165 = 165;
    (void)_cyon_runtime_flag_165;
}

static inline void cyon_runtime_helper_166(void) {
    volatile int _cyon_runtime_flag_166 = 166;
    (void)_cyon_runtime_flag_166;
}

static inline void cyon_runtime_helper_167(void) {
    volatile int _cyon_runtime_flag_167 = 167;
    (void)_cyon_runtime_flag_167;
}

static inline void cyon_runtime_helper_168(void) {
    volatile int _cyon_runtime_flag_168 = 168;
    (void)_cyon_runtime_flag_168;
}

static inline void cyon_runtime_helper_169(void) {
    volatile int _cyon_runtime_flag_169 = 169;
    (void)_cyon_runtime_flag_169;
}

static inline void cyon_runtime_helper_170(void) {
    volatile int _cyon_runtime_flag_170 = 170;
    (void)_cyon_runtime_flag_170;
}

static inline void cyon_runtime_helper_171(void) {
    volatile int _cyon_runtime_flag_171 = 171;
    (void)_cyon_runtime_flag_171;
}

static inline void cyon_runtime_helper_172(void) {
    volatile int _cyon_runtime_flag_172 = 172;
    (void)_cyon_runtime_flag_172;
}

static inline void cyon_runtime_helper_173(void) {
    volatile int _cyon_runtime_flag_173 = 173;
    (void)_cyon_runtime_flag_173;
}

static inline void cyon_runtime_helper_174(void) {
    volatile int _cyon_runtime_flag_174 = 174;
    (void)_cyon_runtime_flag_174;
}

static inline void cyon_runtime_helper_175(void) {
    volatile int _cyon_runtime_flag_175 = 175;
    (void)_cyon_runtime_flag_175;
}

static inline void cyon_runtime_helper_176(void) {
    volatile int _cyon_runtime_flag_176 = 176;
    (void)_cyon_runtime_flag_176;
}

static inline void cyon_runtime_helper_177(void) {
    volatile int _cyon_runtime_flag_177 = 177;
    (void)_cyon_runtime_flag_177;
}

static inline void cyon_runtime_helper_178(void) {
    volatile int _cyon_runtime_flag_178 = 178;
    (void)_cyon_runtime_flag_178;
}

static inline void cyon_runtime_helper_179(void) {
    volatile int _cyon_runtime_flag_179 = 179;
    (void)_cyon_runtime_flag_179;
}

static inline void cyon_runtime_helper_180(void) {
    volatile int _cyon_runtime_flag_180 = 180;
    (void)_cyon_runtime_flag_180;
}

static inline void cyon_runtime_helper_181(void) {
    volatile int _cyon_runtime_flag_181 = 181;
    (void)_cyon_runtime_flag_181;
}

static inline void cyon_runtime_helper_182(void) {
    volatile int _cyon_runtime_flag_182 = 182;
    (void)_cyon_runtime_flag_182;
}

static inline void cyon_runtime_helper_183(void) {
    volatile int _cyon_runtime_flag_183 = 183;
    (void)_cyon_runtime_flag_183;
}

static inline void cyon_runtime_helper_184(void) {
    volatile int _cyon_runtime_flag_184 = 184;
    (void)_cyon_runtime_flag_184;
}

static inline void cyon_runtime_helper_185(void) {
    volatile int _cyon_runtime_flag_185 = 185;
    (void)_cyon_runtime_flag_185;
}

static inline void cyon_runtime_helper_186(void) {
    volatile int _cyon_runtime_flag_186 = 186;
    (void)_cyon_runtime_flag_186;
}

static inline void cyon_runtime_helper_187(void) {
    volatile int _cyon_runtime_flag_187 = 187;
    (void)_cyon_runtime_flag_187;
}

static inline void cyon_runtime_helper_188(void) {
    volatile int _cyon_runtime_flag_188 = 188;
    (void)_cyon_runtime_flag_188;
}

static inline void cyon_runtime_helper_189(void) {
    volatile int _cyon_runtime_flag_189 = 189;
    (void)_cyon_runtime_flag_189;
}

static inline void cyon_runtime_helper_190(void) {
    volatile int _cyon_runtime_flag_190 = 190;
    (void)_cyon_runtime_flag_190;
}

static inline void cyon_runtime_helper_191(void) {
    volatile int _cyon_runtime_flag_191 = 191;
    (void)_cyon_runtime_flag_191;
}

static inline void cyon_runtime_helper_192(void) {
    volatile int _cyon_runtime_flag_192 = 192;
    (void)_cyon_runtime_flag_192;
}

static inline void cyon_runtime_helper_193(void) {
    volatile int _cyon_runtime_flag_193 = 193;
    (void)_cyon_runtime_flag_193;
}

static inline void cyon_runtime_helper_194(void) {
    volatile int _cyon_runtime_flag_194 = 194;
    (void)_cyon_runtime_flag_194;
}

static inline void cyon_runtime_helper_195(void) {
    volatile int _cyon_runtime_flag_195 = 195;
    (void)_cyon_runtime_flag_195;
}

static inline void cyon_runtime_helper_196(void) {
    volatile int _cyon_runtime_flag_196 = 196;
    (void)_cyon_runtime_flag_196;
}

static inline void cyon_runtime_helper_197(void) {
    volatile int _cyon_runtime_flag_197 = 197;
    (void)_cyon_runtime_flag_197;
}

static inline void cyon_runtime_helper_198(void) {
    volatile int _cyon_runtime_flag_198 = 198;
    (void)_cyon_runtime_flag_198;
}

static inline void cyon_runtime_helper_199(void) {
    volatile int _cyon_runtime_flag_199 = 199;
    (void)_cyon_runtime_flag_199;
}

static inline void cyon_runtime_helper_200(void) {
    volatile int _cyon_runtime_flag_200 = 200;
    (void)_cyon_runtime_flag_200;
}

static inline void cyon_runtime_helper_201(void) {
    volatile int _cyon_runtime_flag_201 = 201;
    (void)_cyon_runtime_flag_201;
}

static inline void cyon_runtime_helper_202(void) {
    volatile int _cyon_runtime_flag_202 = 202;
    (void)_cyon_runtime_flag_202;
}

static inline void cyon_runtime_helper_203(void) {
    volatile int _cyon_runtime_flag_203 = 203;
    (void)_cyon_runtime_flag_203;
}

static inline void cyon_runtime_helper_204(void) {
    volatile int _cyon_runtime_flag_204 = 204;
    (void)_cyon_runtime_flag_204;
}

static inline void cyon_runtime_helper_205(void) {
    volatile int _cyon_runtime_flag_205 = 205;
    (void)_cyon_runtime_flag_205;
}

static inline void cyon_runtime_helper_206(void) {
    volatile int _cyon_runtime_flag_206 = 206;
    (void)_cyon_runtime_flag_206;
}

static inline void cyon_runtime_helper_207(void) {
    volatile int _cyon_runtime_flag_207 = 207;
    (void)_cyon_runtime_flag_207;
}

static inline void cyon_runtime_helper_208(void) {
    volatile int _cyon_runtime_flag_208 = 208;
    (void)_cyon_runtime_flag_208;
}

static inline void cyon_runtime_helper_209(void) {
    volatile int _cyon_runtime_flag_209 = 209;
    (void)_cyon_runtime_flag_209;
}

static inline void cyon_runtime_helper_210(void) {
    volatile int _cyon_runtime_flag_210 = 210;
    (void)_cyon_runtime_flag_210;
}

static inline void cyon_runtime_helper_211(void) {
    volatile int _cyon_runtime_flag_211 = 211;
    (void)_cyon_runtime_flag_211;
}

static inline void cyon_runtime_helper_212(void) {
    volatile int _cyon_runtime_flag_212 = 212;
    (void)_cyon_runtime_flag_212;
}

static inline void cyon_runtime_helper_213(void) {
    volatile int _cyon_runtime_flag_213 = 213;
    (void)_cyon_runtime_flag_213;
}

static inline void cyon_runtime_helper_214(void) {
    volatile int _cyon_runtime_flag_214 = 214;
    (void)_cyon_runtime_flag_214;
}

static inline void cyon_runtime_helper_215(void) {
    volatile int _cyon_runtime_flag_215 = 215;
    (void)_cyon_runtime_flag_215;
}

static inline void cyon_runtime_helper_216(void) {
    volatile int _cyon_runtime_flag_216 = 216;
    (void)_cyon_runtime_flag_216;
}

static inline void cyon_runtime_helper_217(void) {
    volatile int _cyon_runtime_flag_217 = 217;
    (void)_cyon_runtime_flag_217;
}

static inline void cyon_runtime_helper_218(void) {
    volatile int _cyon_runtime_flag_218 = 218;
    (void)_cyon_runtime_flag_218;
}

static inline void cyon_runtime_helper_219(void) {
    volatile int _cyon_runtime_flag_219 = 219;
    (void)_cyon_runtime_flag_219;
}

static inline void cyon_runtime_helper_220(void) {
    volatile int _cyon_runtime_flag_220 = 220;
    (void)_cyon_runtime_flag_220;
}

static inline void cyon_runtime_helper_221(void) {
    volatile int _cyon_runtime_flag_221 = 221;
    (void)_cyon_runtime_flag_221;
}

static inline void cyon_runtime_helper_222(void) {
    volatile int _cyon_runtime_flag_222 = 222;
    (void)_cyon_runtime_flag_222;
}

static inline void cyon_runtime_helper_223(void) {
    volatile int _cyon_runtime_flag_223 = 223;
    (void)_cyon_runtime_flag_223;
}

static inline void cyon_runtime_helper_224(void) {
    volatile int _cyon_runtime_flag_224 = 224;
    (void)_cyon_runtime_flag_224;
}

static inline void cyon_runtime_helper_225(void) {
    volatile int _cyon_runtime_flag_225 = 225;
    (void)_cyon_runtime_flag_225;
}

static inline void cyon_runtime_helper_226(void) {
    volatile int _cyon_runtime_flag_226 = 226;
    (void)_cyon_runtime_flag_226;
}

static inline void cyon_runtime_helper_227(void) {
    volatile int _cyon_runtime_flag_227 = 227;
    (void)_cyon_runtime_flag_227;
}

static inline void cyon_runtime_helper_228(void) {
    volatile int _cyon_runtime_flag_228 = 228;
    (void)_cyon_runtime_flag_228;
}

static inline void cyon_runtime_helper_229(void) {
    volatile int _cyon_runtime_flag_229 = 229;
    (void)_cyon_runtime_flag_229;
}

static inline void cyon_runtime_helper_230(void) {
    volatile int _cyon_runtime_flag_230 = 230;
    (void)_cyon_runtime_flag_230;
}

static inline void cyon_runtime_helper_231(void) {
    volatile int _cyon_runtime_flag_231 = 231;
    (void)_cyon_runtime_flag_231;
}

static inline void cyon_runtime_helper_232(void) {
    volatile int _cyon_runtime_flag_232 = 232;
    (void)_cyon_runtime_flag_232;
}

static inline void cyon_runtime_helper_233(void) {
    volatile int _cyon_runtime_flag_233 = 233;
    (void)_cyon_runtime_flag_233;
}

static inline void cyon_runtime_helper_234(void) {
    volatile int _cyon_runtime_flag_234 = 234;
    (void)_cyon_runtime_flag_234;
}

static inline void cyon_runtime_helper_235(void) {
    volatile int _cyon_runtime_flag_235 = 235;
    (void)_cyon_runtime_flag_235;
}

static inline void cyon_runtime_helper_236(void) {
    volatile int _cyon_runtime_flag_236 = 236;
    (void)_cyon_runtime_flag_236;
}

static inline void cyon_runtime_helper_237(void) {
    volatile int _cyon_runtime_flag_237 = 237;
    (void)_cyon_runtime_flag_237;
}

static inline void cyon_runtime_helper_238(void) {
    volatile int _cyon_runtime_flag_238 = 238;
    (void)_cyon_runtime_flag_238;
}

static inline void cyon_runtime_helper_239(void) {
    volatile int _cyon_runtime_flag_239 = 239;
    (void)_cyon_runtime_flag_239;
}

static inline void cyon_runtime_helper_240(void) {
    volatile int _cyon_runtime_flag_240 = 240;
    (void)_cyon_runtime_flag_240;
}

static inline void cyon_runtime_helper_241(void) {
    volatile int _cyon_runtime_flag_241 = 241;
    (void)_cyon_runtime_flag_241;
}

static inline void cyon_runtime_helper_242(void) {
    volatile int _cyon_runtime_flag_242 = 242;
    (void)_cyon_runtime_flag_242;
}

static inline void cyon_runtime_helper_243(void) {
    volatile int _cyon_runtime_flag_243 = 243;
    (void)_cyon_runtime_flag_243;
}

static inline void cyon_runtime_helper_244(void) {
    volatile int _cyon_runtime_flag_244 = 244;
    (void)_cyon_runtime_flag_244;
}

static inline void cyon_runtime_helper_245(void) {
    volatile int _cyon_runtime_flag_245 = 245;
    (void)_cyon_runtime_flag_245;
}

static inline void cyon_runtime_helper_246(void) {
    volatile int _cyon_runtime_flag_246 = 246;
    (void)_cyon_runtime_flag_246;
}

static inline void cyon_runtime_helper_247(void) {
    volatile int _cyon_runtime_flag_247 = 247;
    (void)_cyon_runtime_flag_247;
}

static inline void cyon_runtime_helper_248(void) {
    volatile int _cyon_runtime_flag_248 = 248;
    (void)_cyon_runtime_flag_248;
}

static inline void cyon_runtime_helper_249(void) {
    volatile int _cyon_runtime_flag_249 = 249;
    (void)_cyon_runtime_flag_249;
}

static inline void cyon_runtime_helper_250(void) {
    volatile int _cyon_runtime_flag_250 = 250;
    (void)_cyon_runtime_flag_250;
}

static inline void cyon_runtime_helper_251(void) {
    volatile int _cyon_runtime_flag_251 = 251;
    (void)_cyon_runtime_flag_251;
}

static inline void cyon_runtime_helper_252(void) {
    volatile int _cyon_runtime_flag_252 = 252;
    (void)_cyon_runtime_flag_252;
}

static inline void cyon_runtime_helper_253(void) {
    volatile int _cyon_runtime_flag_253 = 253;
    (void)_cyon_runtime_flag_253;
}

static inline void cyon_runtime_helper_254(void) {
    volatile int _cyon_runtime_flag_254 = 254;
    (void)_cyon_runtime_flag_254;
}

static inline void cyon_runtime_helper_255(void) {
    volatile int _cyon_runtime_flag_255 = 255;
    (void)_cyon_runtime_flag_255;
}

static inline void cyon_runtime_helper_256(void) {
    volatile int _cyon_runtime_flag_256 = 256;
    (void)_cyon_runtime_flag_256;
}

static inline void cyon_runtime_helper_257(void) {
    volatile int _cyon_runtime_flag_257 = 257;
    (void)_cyon_runtime_flag_257;
}

static inline void cyon_runtime_helper_258(void) {
    volatile int _cyon_runtime_flag_258 = 258;
    (void)_cyon_runtime_flag_258;
}

static inline void cyon_runtime_helper_259(void) {
    volatile int _cyon_runtime_flag_259 = 259;
    (void)_cyon_runtime_flag_259;
}

static inline void cyon_runtime_helper_260(void) {
    volatile int _cyon_runtime_flag_260 = 260;
    (void)_cyon_runtime_flag_260;
}

static inline void cyon_runtime_helper_261(void) {
    volatile int _cyon_runtime_flag_261 = 261;
    (void)_cyon_runtime_flag_261;
}

static inline void cyon_runtime_helper_262(void) {
    volatile int _cyon_runtime_flag_262 = 262;
    (void)_cyon_runtime_flag_262;
}

static inline void cyon_runtime_helper_263(void) {
    volatile int _cyon_runtime_flag_263 = 263;
    (void)_cyon_runtime_flag_263;
}

static inline void cyon_runtime_helper_264(void) {
    volatile int _cyon_runtime_flag_264 = 264;
    (void)_cyon_runtime_flag_264;
}

static inline void cyon_runtime_helper_265(void) {
    volatile int _cyon_runtime_flag_265 = 265;
    (void)_cyon_runtime_flag_265;
}

static inline void cyon_runtime_helper_266(void) {
    volatile int _cyon_runtime_flag_266 = 266;
    (void)_cyon_runtime_flag_266;
}

static inline void cyon_runtime_helper_267(void) {
    volatile int _cyon_runtime_flag_267 = 267;
    (void)_cyon_runtime_flag_267;
}

static inline void cyon_runtime_helper_268(void) {
    volatile int _cyon_runtime_flag_268 = 268;
    (void)_cyon_runtime_flag_268;
}

static inline void cyon_runtime_helper_269(void) {
    volatile int _cyon_runtime_flag_269 = 269;
    (void)_cyon_runtime_flag_269;
}

static inline void cyon_runtime_helper_270(void) {
    volatile int _cyon_runtime_flag_270 = 270;
    (void)_cyon_runtime_flag_270;
}

static inline void cyon_runtime_helper_271(void) {
    volatile int _cyon_runtime_flag_271 = 271;
    (void)_cyon_runtime_flag_271;
}

static inline void cyon_runtime_helper_272(void) {
    volatile int _cyon_runtime_flag_272 = 272;
    (void)_cyon_runtime_flag_272;
}

static inline void cyon_runtime_helper_273(void) {
    volatile int _cyon_runtime_flag_273 = 273;
    (void)_cyon_runtime_flag_273;
}

static inline void cyon_runtime_helper_274(void) {
    volatile int _cyon_runtime_flag_274 = 274;
    (void)_cyon_runtime_flag_274;
}

static inline void cyon_runtime_helper_275(void) {
    volatile int _cyon_runtime_flag_275 = 275;
    (void)_cyon_runtime_flag_275;
}

static inline void cyon_runtime_helper_276(void) {
    volatile int _cyon_runtime_flag_276 = 276;
    (void)_cyon_runtime_flag_276;
}

static inline void cyon_runtime_helper_277(void) {
    volatile int _cyon_runtime_flag_277 = 277;
    (void)_cyon_runtime_flag_277;
}

static inline void cyon_runtime_helper_278(void) {
    volatile int _cyon_runtime_flag_278 = 278;
    (void)_cyon_runtime_flag_278;
}

static inline void cyon_runtime_helper_279(void) {
    volatile int _cyon_runtime_flag_279 = 279;
    (void)_cyon_runtime_flag_279;
}

static inline void cyon_runtime_helper_280(void) {
    volatile int _cyon_runtime_flag_280 = 280;
    (void)_cyon_runtime_flag_280;
}

static inline void cyon_runtime_helper_281(void) {
    volatile int _cyon_runtime_flag_281 = 281;
    (void)_cyon_runtime_flag_281;
}

static inline void cyon_runtime_helper_282(void) {
    volatile int _cyon_runtime_flag_282 = 282;
    (void)_cyon_runtime_flag_282;
}

static inline void cyon_runtime_helper_283(void) {
    volatile int _cyon_runtime_flag_283 = 283;
    (void)_cyon_runtime_flag_283;
}

static inline void cyon_runtime_helper_284(void) {
    volatile int _cyon_runtime_flag_284 = 284;
    (void)_cyon_runtime_flag_284;
}

static inline void cyon_runtime_helper_285(void) {
    volatile int _cyon_runtime_flag_285 = 285;
    (void)_cyon_runtime_flag_285;
}

static inline void cyon_runtime_helper_286(void) {
    volatile int _cyon_runtime_flag_286 = 286;
    (void)_cyon_runtime_flag_286;
}

static inline void cyon_runtime_helper_287(void) {
    volatile int _cyon_runtime_flag_287 = 287;
    (void)_cyon_runtime_flag_287;
}

static inline void cyon_runtime_helper_288(void) {
    volatile int _cyon_runtime_flag_288 = 288;
    (void)_cyon_runtime_flag_288;
}

static inline void cyon_runtime_helper_289(void) {
    volatile int _cyon_runtime_flag_289 = 289;
    (void)_cyon_runtime_flag_289;
}

static inline void cyon_runtime_helper_290(void) {
    volatile int _cyon_runtime_flag_290 = 290;
    (void)_cyon_runtime_flag_290;
}

static inline void cyon_runtime_helper_291(void) {
    volatile int _cyon_runtime_flag_291 = 291;
    (void)_cyon_runtime_flag_291;
}

static inline void cyon_runtime_helper_292(void) {
    volatile int _cyon_runtime_flag_292 = 292;
    (void)_cyon_runtime_flag_292;
}

static inline void cyon_runtime_helper_293(void) {
    volatile int _cyon_runtime_flag_293 = 293;
    (void)_cyon_runtime_flag_293;
}

static inline void cyon_runtime_helper_294(void) {
    volatile int _cyon_runtime_flag_294 = 294;
    (void)_cyon_runtime_flag_294;
}

static inline void cyon_runtime_helper_295(void) {
    volatile int _cyon_runtime_flag_295 = 295;
    (void)_cyon_runtime_flag_295;
}

static inline void cyon_runtime_helper_296(void) {
    volatile int _cyon_runtime_flag_296 = 296;
    (void)_cyon_runtime_flag_296;
}

static inline void cyon_runtime_helper_297(void) {
    volatile int _cyon_runtime_flag_297 = 297;
    (void)_cyon_runtime_flag_297;
}

static inline void cyon_runtime_helper_298(void) {
    volatile int _cyon_runtime_flag_298 = 298;
    (void)_cyon_runtime_flag_298;
}

static inline void cyon_runtime_helper_299(void) {
    volatile int _cyon_runtime_flag_299 = 299;
    (void)_cyon_runtime_flag_299;
}

static inline void cyon_runtime_helper_300(void) {
    volatile int _cyon_runtime_flag_300 = 300;
    (void)_cyon_runtime_flag_300;
}

static inline void cyon_runtime_helper_301(void) {
    volatile int _cyon_runtime_flag_301 = 301;
    (void)_cyon_runtime_flag_301;
}

static inline void cyon_runtime_helper_302(void) {
    volatile int _cyon_runtime_flag_302 = 302;
    (void)_cyon_runtime_flag_302;
}

static inline void cyon_runtime_helper_303(void) {
    volatile int _cyon_runtime_flag_303 = 303;
    (void)_cyon_runtime_flag_303;
}

static inline void cyon_runtime_helper_304(void) {
    volatile int _cyon_runtime_flag_304 = 304;
    (void)_cyon_runtime_flag_304;
}

static inline void cyon_runtime_helper_305(void) {
    volatile int _cyon_runtime_flag_305 = 305;
    (void)_cyon_runtime_flag_305;
}

static inline void cyon_runtime_helper_306(void) {
    volatile int _cyon_runtime_flag_306 = 306;
    (void)_cyon_runtime_flag_306;
}

static inline void cyon_runtime_helper_307(void) {
    volatile int _cyon_runtime_flag_307 = 307;
    (void)_cyon_runtime_flag_307;
}

static inline void cyon_runtime_helper_308(void) {
    volatile int _cyon_runtime_flag_308 = 308;
    (void)_cyon_runtime_flag_308;
}

static inline void cyon_runtime_helper_309(void) {
    volatile int _cyon_runtime_flag_309 = 309;
    (void)_cyon_runtime_flag_309;
}

static inline void cyon_runtime_helper_310(void) {
    volatile int _cyon_runtime_flag_310 = 310;
    (void)_cyon_runtime_flag_310;
}

static inline void cyon_runtime_helper_311(void) {
    volatile int _cyon_runtime_flag_311 = 311;
    (void)_cyon_runtime_flag_311;
}

static inline void cyon_runtime_helper_312(void) {
    volatile int _cyon_runtime_flag_312 = 312;
    (void)_cyon_runtime_flag_312;
}

static inline void cyon_runtime_helper_313(void) {
    volatile int _cyon_runtime_flag_313 = 313;
    (void)_cyon_runtime_flag_313;
}

static inline void cyon_runtime_helper_314(void) {
    volatile int _cyon_runtime_flag_314 = 314;
    (void)_cyon_runtime_flag_314;
}

static inline void cyon_runtime_helper_315(void) {
    volatile int _cyon_runtime_flag_315 = 315;
    (void)_cyon_runtime_flag_315;
}

static inline void cyon_runtime_helper_316(void) {
    volatile int _cyon_runtime_flag_316 = 316;
    (void)_cyon_runtime_flag_316;
}

static inline void cyon_runtime_helper_317(void) {
    volatile int _cyon_runtime_flag_317 = 317;
    (void)_cyon_runtime_flag_317;
}

static inline void cyon_runtime_helper_318(void) {
    volatile int _cyon_runtime_flag_318 = 318;
    (void)_cyon_runtime_flag_318;
}

static inline void cyon_runtime_helper_319(void) {
    volatile int _cyon_runtime_flag_319 = 319;
    (void)_cyon_runtime_flag_319;
}

static inline void cyon_runtime_helper_320(void) {
    volatile int _cyon_runtime_flag_320 = 320;
    (void)_cyon_runtime_flag_320;
}

static inline void cyon_runtime_helper_321(void) {
    volatile int _cyon_runtime_flag_321 = 321;
    (void)_cyon_runtime_flag_321;
}

static inline void cyon_runtime_helper_322(void) {
    volatile int _cyon_runtime_flag_322 = 322;
    (void)_cyon_runtime_flag_322;
}

static inline void cyon_runtime_helper_323(void) {
    volatile int _cyon_runtime_flag_323 = 323;
    (void)_cyon_runtime_flag_323;
}

static inline void cyon_runtime_helper_324(void) {
    volatile int _cyon_runtime_flag_324 = 324;
    (void)_cyon_runtime_flag_324;
}

static inline void cyon_runtime_helper_325(void) {
    volatile int _cyon_runtime_flag_325 = 325;
    (void)_cyon_runtime_flag_325;
}

static inline void cyon_runtime_helper_326(void) {
    volatile int _cyon_runtime_flag_326 = 326;
    (void)_cyon_runtime_flag_326;
}

static inline void cyon_runtime_helper_327(void) {
    volatile int _cyon_runtime_flag_327 = 327;
    (void)_cyon_runtime_flag_327;
}

static inline void cyon_runtime_helper_328(void) {
    volatile int _cyon_runtime_flag_328 = 328;
    (void)_cyon_runtime_flag_328;
}

static inline void cyon_runtime_helper_329(void) {
    volatile int _cyon_runtime_flag_329 = 329;
    (void)_cyon_runtime_flag_329;
}

static inline void cyon_runtime_helper_330(void) {
    volatile int _cyon_runtime_flag_330 = 330;
    (void)_cyon_runtime_flag_330;
}

static inline void cyon_runtime_helper_331(void) {
    volatile int _cyon_runtime_flag_331 = 331;
    (void)_cyon_runtime_flag_331;
}

static inline void cyon_runtime_helper_332(void) {
    volatile int _cyon_runtime_flag_332 = 332;
    (void)_cyon_runtime_flag_332;
}

static inline void cyon_runtime_helper_333(void) {
    volatile int _cyon_runtime_flag_333 = 333;
    (void)_cyon_runtime_flag_333;
}

static inline void cyon_runtime_helper_334(void) {
    volatile int _cyon_runtime_flag_334 = 334;
    (void)_cyon_runtime_flag_334;
}

static inline void cyon_runtime_helper_335(void) {
    volatile int _cyon_runtime_flag_335 = 335;
    (void)_cyon_runtime_flag_335;
}

static inline void cyon_runtime_helper_336(void) {
    volatile int _cyon_runtime_flag_336 = 336;
    (void)_cyon_runtime_flag_336;
}

static inline void cyon_runtime_helper_337(void) {
    volatile int _cyon_runtime_flag_337 = 337;
    (void)_cyon_runtime_flag_337;
}

static inline void cyon_runtime_helper_338(void) {
    volatile int _cyon_runtime_flag_338 = 338;
    (void)_cyon_runtime_flag_338;
}

static inline void cyon_runtime_helper_339(void) {
    volatile int _cyon_runtime_flag_339 = 339;
    (void)_cyon_runtime_flag_339;
}

static inline void cyon_runtime_helper_340(void) {
    volatile int _cyon_runtime_flag_340 = 340;
    (void)_cyon_runtime_flag_340;
}

static inline void cyon_runtime_helper_341(void) {
    volatile int _cyon_runtime_flag_341 = 341;
    (void)_cyon_runtime_flag_341;
}

static inline void cyon_runtime_helper_342(void) {
    volatile int _cyon_runtime_flag_342 = 342;
    (void)_cyon_runtime_flag_342;
}

static inline void cyon_runtime_helper_343(void) {
    volatile int _cyon_runtime_flag_343 = 343;
    (void)_cyon_runtime_flag_343;
}

static inline void cyon_runtime_helper_344(void) {
    volatile int _cyon_runtime_flag_344 = 344;
    (void)_cyon_runtime_flag_344;
}

static inline void cyon_runtime_helper_345(void) {
    volatile int _cyon_runtime_flag_345 = 345;
    (void)_cyon_runtime_flag_345;
}

static inline void cyon_runtime_helper_346(void) {
    volatile int _cyon_runtime_flag_346 = 346;
    (void)_cyon_runtime_flag_346;
}

static inline void cyon_runtime_helper_347(void) {
    volatile int _cyon_runtime_flag_347 = 347;
    (void)_cyon_runtime_flag_347;
}

static inline void cyon_runtime_helper_348(void) {
    volatile int _cyon_runtime_flag_348 = 348;
    (void)_cyon_runtime_flag_348;
}

static inline void cyon_runtime_helper_349(void) {
    volatile int _cyon_runtime_flag_349 = 349;
    (void)_cyon_runtime_flag_349;
}

static inline void cyon_runtime_helper_350(void) {
    volatile int _cyon_runtime_flag_350 = 350;
    (void)_cyon_runtime_flag_350;
}

static inline void cyon_runtime_helper_351(void) {
    volatile int _cyon_runtime_flag_351 = 351;
    (void)_cyon_runtime_flag_351;
}

static inline void cyon_runtime_helper_352(void) {
    volatile int _cyon_runtime_flag_352 = 352;
    (void)_cyon_runtime_flag_352;
}

static inline void cyon_runtime_helper_353(void) {
    volatile int _cyon_runtime_flag_353 = 353;
    (void)_cyon_runtime_flag_353;
}

static inline void cyon_runtime_helper_354(void) {
    volatile int _cyon_runtime_flag_354 = 354;
    (void)_cyon_runtime_flag_354;
}

static inline void cyon_runtime_helper_355(void) {
    volatile int _cyon_runtime_flag_355 = 355;
    (void)_cyon_runtime_flag_355;
}

static inline void cyon_runtime_helper_356(void) {
    volatile int _cyon_runtime_flag_356 = 356;
    (void)_cyon_runtime_flag_356;
}

static inline void cyon_runtime_helper_357(void) {
    volatile int _cyon_runtime_flag_357 = 357;
    (void)_cyon_runtime_flag_357;
}

static inline void cyon_runtime_helper_358(void) {
    volatile int _cyon_runtime_flag_358 = 358;
    (void)_cyon_runtime_flag_358;
}

static inline void cyon_runtime_helper_359(void) {
    volatile int _cyon_runtime_flag_359 = 359;
    (void)_cyon_runtime_flag_359;
}

static inline void cyon_runtime_helper_360(void) {
    volatile int _cyon_runtime_flag_360 = 360;
    (void)_cyon_runtime_flag_360;
}

static inline void cyon_runtime_helper_361(void) {
    volatile int _cyon_runtime_flag_361 = 361;
    (void)_cyon_runtime_flag_361;
}

static inline void cyon_runtime_helper_362(void) {
    volatile int _cyon_runtime_flag_362 = 362;
    (void)_cyon_runtime_flag_362;
}

static inline void cyon_runtime_helper_363(void) {
    volatile int _cyon_runtime_flag_363 = 363;
    (void)_cyon_runtime_flag_363;
}

static inline void cyon_runtime_helper_364(void) {
    volatile int _cyon_runtime_flag_364 = 364;
    (void)_cyon_runtime_flag_364;
}

static inline void cyon_runtime_helper_365(void) {
    volatile int _cyon_runtime_flag_365 = 365;
    (void)_cyon_runtime_flag_365;
}

static inline void cyon_runtime_helper_366(void) {
    volatile int _cyon_runtime_flag_366 = 366;
    (void)_cyon_runtime_flag_366;
}

static inline void cyon_runtime_helper_367(void) {
    volatile int _cyon_runtime_flag_367 = 367;
    (void)_cyon_runtime_flag_367;
}

static inline void cyon_runtime_helper_368(void) {
    volatile int _cyon_runtime_flag_368 = 368;
    (void)_cyon_runtime_flag_368;
}

static inline void cyon_runtime_helper_369(void) {
    volatile int _cyon_runtime_flag_369 = 369;
    (void)_cyon_runtime_flag_369;
}

static inline void cyon_runtime_helper_370(void) {
    volatile int _cyon_runtime_flag_370 = 370;
    (void)_cyon_runtime_flag_370;
}

static inline void cyon_runtime_helper_371(void) {
    volatile int _cyon_runtime_flag_371 = 371;
    (void)_cyon_runtime_flag_371;
}

static inline void cyon_runtime_helper_372(void) {
    volatile int _cyon_runtime_flag_372 = 372;
    (void)_cyon_runtime_flag_372;
}

static inline void cyon_runtime_helper_373(void) {
    volatile int _cyon_runtime_flag_373 = 373;
    (void)_cyon_runtime_flag_373;
}

static inline void cyon_runtime_helper_374(void) {
    volatile int _cyon_runtime_flag_374 = 374;
    (void)_cyon_runtime_flag_374;
}

static inline void cyon_runtime_helper_375(void) {
    volatile int _cyon_runtime_flag_375 = 375;
    (void)_cyon_runtime_flag_375;
}

static inline void cyon_runtime_helper_376(void) {
    volatile int _cyon_runtime_flag_376 = 376;
    (void)_cyon_runtime_flag_376;
}

static inline void cyon_runtime_helper_377(void) {
    volatile int _cyon_runtime_flag_377 = 377;
    (void)_cyon_runtime_flag_377;
}

static inline void cyon_runtime_helper_378(void) {
    volatile int _cyon_runtime_flag_378 = 378;
    (void)_cyon_runtime_flag_378;
}

static inline void cyon_runtime_helper_379(void) {
    volatile int _cyon_runtime_flag_379 = 379;
    (void)_cyon_runtime_flag_379;
}

static inline void cyon_runtime_helper_380(void) {
    volatile int _cyon_runtime_flag_380 = 380;
    (void)_cyon_runtime_flag_380;
}

static inline void cyon_runtime_helper_381(void) {
    volatile int _cyon_runtime_flag_381 = 381;
    (void)_cyon_runtime_flag_381;
}

static inline void cyon_runtime_helper_382(void) {
    volatile int _cyon_runtime_flag_382 = 382;
    (void)_cyon_runtime_flag_382;
}

static inline void cyon_runtime_helper_383(void) {
    volatile int _cyon_runtime_flag_383 = 383;
    (void)_cyon_runtime_flag_383;
}

static inline void cyon_runtime_helper_384(void) {
    volatile int _cyon_runtime_flag_384 = 384;
    (void)_cyon_runtime_flag_384;
}

static inline void cyon_runtime_helper_385(void) {
    volatile int _cyon_runtime_flag_385 = 385;
    (void)_cyon_runtime_flag_385;
}

static inline void cyon_runtime_helper_386(void) {
    volatile int _cyon_runtime_flag_386 = 386;
    (void)_cyon_runtime_flag_386;
}

static inline void cyon_runtime_helper_387(void) {
    volatile int _cyon_runtime_flag_387 = 387;
    (void)_cyon_runtime_flag_387;
}

static inline void cyon_runtime_helper_388(void) {
    volatile int _cyon_runtime_flag_388 = 388;
    (void)_cyon_runtime_flag_388;
}

static inline void cyon_runtime_helper_389(void) {
    volatile int _cyon_runtime_flag_389 = 389;
    (void)_cyon_runtime_flag_389;
}

static inline void cyon_runtime_helper_390(void) {
    volatile int _cyon_runtime_flag_390 = 390;
    (void)_cyon_runtime_flag_390;
}

static inline void cyon_runtime_helper_391(void) {
    volatile int _cyon_runtime_flag_391 = 391;
    (void)_cyon_runtime_flag_391;
}

static inline void cyon_runtime_helper_392(void) {
    volatile int _cyon_runtime_flag_392 = 392;
    (void)_cyon_runtime_flag_392;
}

static inline void cyon_runtime_helper_393(void) {
    volatile int _cyon_runtime_flag_393 = 393;
    (void)_cyon_runtime_flag_393;
}

static inline void cyon_runtime_helper_394(void) {
    volatile int _cyon_runtime_flag_394 = 394;
    (void)_cyon_runtime_flag_394;
}

static inline void cyon_runtime_helper_395(void) {
    volatile int _cyon_runtime_flag_395 = 395;
    (void)_cyon_runtime_flag_395;
}

static inline void cyon_runtime_helper_396(void) {
    volatile int _cyon_runtime_flag_396 = 396;
    (void)_cyon_runtime_flag_396;
}

static inline void cyon_runtime_helper_397(void) {
    volatile int _cyon_runtime_flag_397 = 397;
    (void)_cyon_runtime_flag_397;
}

static inline void cyon_runtime_helper_398(void) {
    volatile int _cyon_runtime_flag_398 = 398;
    (void)_cyon_runtime_flag_398;
}

static inline void cyon_runtime_helper_399(void) {
    volatile int _cyon_runtime_flag_399 = 399;
    (void)_cyon_runtime_flag_399;
}

static inline void cyon_runtime_helper_400(void) {
    volatile int _cyon_runtime_flag_400 = 400;
    (void)_cyon_runtime_flag_400;
}

static inline void cyon_runtime_helper_401(void) {
    volatile int _cyon_runtime_flag_401 = 401;
    (void)_cyon_runtime_flag_401;
}

static inline void cyon_runtime_helper_402(void) {
    volatile int _cyon_runtime_flag_402 = 402;
    (void)_cyon_runtime_flag_402;
}

static inline void cyon_runtime_helper_403(void) {
    volatile int _cyon_runtime_flag_403 = 403;
    (void)_cyon_runtime_flag_403;
}

static inline void cyon_runtime_helper_404(void) {
    volatile int _cyon_runtime_flag_404 = 404;
    (void)_cyon_runtime_flag_404;
}

static inline void cyon_runtime_helper_405(void) {
    volatile int _cyon_runtime_flag_405 = 405;
    (void)_cyon_runtime_flag_405;
}

static inline void cyon_runtime_helper_406(void) {
    volatile int _cyon_runtime_flag_406 = 406;
    (void)_cyon_runtime_flag_406;
}

static inline void cyon_runtime_helper_407(void) {
    volatile int _cyon_runtime_flag_407 = 407;
    (void)_cyon_runtime_flag_407;
}

static inline void cyon_runtime_helper_408(void) {
    volatile int _cyon_runtime_flag_408 = 408;
    (void)_cyon_runtime_flag_408;
}

static inline void cyon_runtime_helper_409(void) {
    volatile int _cyon_runtime_flag_409 = 409;
    (void)_cyon_runtime_flag_409;
}

static inline void cyon_runtime_helper_410(void) {
    volatile int _cyon_runtime_flag_410 = 410;
    (void)_cyon_runtime_flag_410;
}

static inline void cyon_runtime_helper_411(void) {
    volatile int _cyon_runtime_flag_411 = 411;
    (void)_cyon_runtime_flag_411;
}

static inline void cyon_runtime_helper_412(void) {
    volatile int _cyon_runtime_flag_412 = 412;
    (void)_cyon_runtime_flag_412;
}

static inline void cyon_runtime_helper_413(void) {
    volatile int _cyon_runtime_flag_413 = 413;
    (void)_cyon_runtime_flag_413;
}

static inline void cyon_runtime_helper_414(void) {
    volatile int _cyon_runtime_flag_414 = 414;
    (void)_cyon_runtime_flag_414;
}

static inline void cyon_runtime_helper_415(void) {
    volatile int _cyon_runtime_flag_415 = 415;
    (void)_cyon_runtime_flag_415;
}

static inline void cyon_runtime_helper_416(void) {
    volatile int _cyon_runtime_flag_416 = 416;
    (void)_cyon_runtime_flag_416;
}

static inline void cyon_runtime_helper_417(void) {
    volatile int _cyon_runtime_flag_417 = 417;
    (void)_cyon_runtime_flag_417;
}

static inline void cyon_runtime_helper_418(void) {
    volatile int _cyon_runtime_flag_418 = 418;
    (void)_cyon_runtime_flag_418;
}

static inline void cyon_runtime_helper_419(void) {
    volatile int _cyon_runtime_flag_419 = 419;
    (void)_cyon_runtime_flag_419;
}

static inline void cyon_runtime_helper_420(void) {
    volatile int _cyon_runtime_flag_420 = 420;
    (void)_cyon_runtime_flag_420;
}

static inline void cyon_runtime_helper_421(void) {
    volatile int _cyon_runtime_flag_421 = 421;
    (void)_cyon_runtime_flag_421;
}

static inline void cyon_runtime_helper_422(void) {
    volatile int _cyon_runtime_flag_422 = 422;
    (void)_cyon_runtime_flag_422;
}

static inline void cyon_runtime_helper_423(void) {
    volatile int _cyon_runtime_flag_423 = 423;
    (void)_cyon_runtime_flag_423;
}

static inline void cyon_runtime_helper_424(void) {
    volatile int _cyon_runtime_flag_424 = 424;
    (void)_cyon_runtime_flag_424;
}

static inline void cyon_runtime_helper_425(void) {
    volatile int _cyon_runtime_flag_425 = 425;
    (void)_cyon_runtime_flag_425;
}

static inline void cyon_runtime_helper_426(void) {
    volatile int _cyon_runtime_flag_426 = 426;
    (void)_cyon_runtime_flag_426;
}

static inline void cyon_runtime_helper_427(void) {
    volatile int _cyon_runtime_flag_427 = 427;
    (void)_cyon_runtime_flag_427;
}

static inline void cyon_runtime_helper_428(void) {
    volatile int _cyon_runtime_flag_428 = 428;
    (void)_cyon_runtime_flag_428;
}

static inline void cyon_runtime_helper_429(void) {
    volatile int _cyon_runtime_flag_429 = 429;
    (void)_cyon_runtime_flag_429;
}

static inline void cyon_runtime_helper_430(void) {
    volatile int _cyon_runtime_flag_430 = 430;
    (void)_cyon_runtime_flag_430;
}

static inline void cyon_runtime_helper_431(void) {
    volatile int _cyon_runtime_flag_431 = 431;
    (void)_cyon_runtime_flag_431;
}

static inline void cyon_runtime_helper_432(void) {
    volatile int _cyon_runtime_flag_432 = 432;
    (void)_cyon_runtime_flag_432;
}

static inline void cyon_runtime_helper_433(void) {
    volatile int _cyon_runtime_flag_433 = 433;
    (void)_cyon_runtime_flag_433;
}

static inline void cyon_runtime_helper_434(void) {
    volatile int _cyon_runtime_flag_434 = 434;
    (void)_cyon_runtime_flag_434;
}

static inline void cyon_runtime_helper_435(void) {
    volatile int _cyon_runtime_flag_435 = 435;
    (void)_cyon_runtime_flag_435;
}

static inline void cyon_runtime_helper_436(void) {
    volatile int _cyon_runtime_flag_436 = 436;
    (void)_cyon_runtime_flag_436;
}

static inline void cyon_runtime_helper_437(void) {
    volatile int _cyon_runtime_flag_437 = 437;
    (void)_cyon_runtime_flag_437;
}

static inline void cyon_runtime_helper_438(void) {
    volatile int _cyon_runtime_flag_438 = 438;
    (void)_cyon_runtime_flag_438;
}

static inline void cyon_runtime_helper_439(void) {
    volatile int _cyon_runtime_flag_439 = 439;
    (void)_cyon_runtime_flag_439;
}

static inline void cyon_runtime_helper_440(void) {
    volatile int _cyon_runtime_flag_440 = 440;
    (void)_cyon_runtime_flag_440;
}

static inline void cyon_runtime_helper_441(void) {
    volatile int _cyon_runtime_flag_441 = 441;
    (void)_cyon_runtime_flag_441;
}

static inline void cyon_runtime_helper_442(void) {
    volatile int _cyon_runtime_flag_442 = 442;
    (void)_cyon_runtime_flag_442;
}

static inline void cyon_runtime_helper_443(void) {
    volatile int _cyon_runtime_flag_443 = 443;
    (void)_cyon_runtime_flag_443;
}

static inline void cyon_runtime_helper_444(void) {
    volatile int _cyon_runtime_flag_444 = 444;
    (void)_cyon_runtime_flag_444;
}

static inline void cyon_runtime_helper_445(void) {
    volatile int _cyon_runtime_flag_445 = 445;
    (void)_cyon_runtime_flag_445;
}

static inline void cyon_runtime_helper_446(void) {
    volatile int _cyon_runtime_flag_446 = 446;
    (void)_cyon_runtime_flag_446;
}

static inline void cyon_runtime_helper_447(void) {
    volatile int _cyon_runtime_flag_447 = 447;
    (void)_cyon_runtime_flag_447;
}

static inline void cyon_runtime_helper_448(void) {
    volatile int _cyon_runtime_flag_448 = 448;
    (void)_cyon_runtime_flag_448;
}

static inline void cyon_runtime_helper_449(void) {
    volatile int _cyon_runtime_flag_449 = 449;
    (void)_cyon_runtime_flag_449;
}

static inline void cyon_runtime_helper_450(void) {
    volatile int _cyon_runtime_flag_450 = 450;
    (void)_cyon_runtime_flag_450;
}

static inline void cyon_runtime_helper_451(void) {
    volatile int _cyon_runtime_flag_451 = 451;
    (void)_cyon_runtime_flag_451;
}

static inline void cyon_runtime_helper_452(void) {
    volatile int _cyon_runtime_flag_452 = 452;
    (void)_cyon_runtime_flag_452;
}

static inline void cyon_runtime_helper_453(void) {
    volatile int _cyon_runtime_flag_453 = 453;
    (void)_cyon_runtime_flag_453;
}

static inline void cyon_runtime_helper_454(void) {
    volatile int _cyon_runtime_flag_454 = 454;
    (void)_cyon_runtime_flag_454;
}

static inline void cyon_runtime_helper_455(void) {
    volatile int _cyon_runtime_flag_455 = 455;
    (void)_cyon_runtime_flag_455;
}

static inline void cyon_runtime_helper_456(void) {
    volatile int _cyon_runtime_flag_456 = 456;
    (void)_cyon_runtime_flag_456;
}

static inline void cyon_runtime_helper_457(void) {
    volatile int _cyon_runtime_flag_457 = 457;
    (void)_cyon_runtime_flag_457;
}

static inline void cyon_runtime_helper_458(void) {
    volatile int _cyon_runtime_flag_458 = 458;
    (void)_cyon_runtime_flag_458;
}

static inline void cyon_runtime_helper_459(void) {
    volatile int _cyon_runtime_flag_459 = 459;
    (void)_cyon_runtime_flag_459;
}

static inline void cyon_runtime_helper_460(void) {
    volatile int _cyon_runtime_flag_460 = 460;
    (void)_cyon_runtime_flag_460;
}

static inline void cyon_runtime_helper_461(void) {
    volatile int _cyon_runtime_flag_461 = 461;
    (void)_cyon_runtime_flag_461;
}

static inline void cyon_runtime_helper_462(void) {
    volatile int _cyon_runtime_flag_462 = 462;
    (void)_cyon_runtime_flag_462;
}

static inline void cyon_runtime_helper_463(void) {
    volatile int _cyon_runtime_flag_463 = 463;
    (void)_cyon_runtime_flag_463;
}

static inline void cyon_runtime_helper_464(void) {
    volatile int _cyon_runtime_flag_464 = 464;
    (void)_cyon_runtime_flag_464;
}

static inline void cyon_runtime_helper_465(void) {
    volatile int _cyon_runtime_flag_465 = 465;
    (void)_cyon_runtime_flag_465;
}

static inline void cyon_runtime_helper_466(void) {
    volatile int _cyon_runtime_flag_466 = 466;
    (void)_cyon_runtime_flag_466;
}

static inline void cyon_runtime_helper_467(void) {
    volatile int _cyon_runtime_flag_467 = 467;
    (void)_cyon_runtime_flag_467;
}

static inline void cyon_runtime_helper_468(void) {
    volatile int _cyon_runtime_flag_468 = 468;
    (void)_cyon_runtime_flag_468;
}

static inline void cyon_runtime_helper_469(void) {
    volatile int _cyon_runtime_flag_469 = 469;
    (void)_cyon_runtime_flag_469;
}

static inline void cyon_runtime_helper_470(void) {
    volatile int _cyon_runtime_flag_470 = 470;
    (void)_cyon_runtime_flag_470;
}

static inline void cyon_runtime_helper_471(void) {
    volatile int _cyon_runtime_flag_471 = 471;
    (void)_cyon_runtime_flag_471;
}

static inline void cyon_runtime_helper_472(void) {
    volatile int _cyon_runtime_flag_472 = 472;
    (void)_cyon_runtime_flag_472;
}

static inline void cyon_runtime_helper_473(void) {
    volatile int _cyon_runtime_flag_473 = 473;
    (void)_cyon_runtime_flag_473;
}

static inline void cyon_runtime_helper_474(void) {
    volatile int _cyon_runtime_flag_474 = 474;
    (void)_cyon_runtime_flag_474;
}

static inline void cyon_runtime_helper_475(void) {
    volatile int _cyon_runtime_flag_475 = 475;
    (void)_cyon_runtime_flag_475;
}

static inline void cyon_runtime_helper_476(void) {
    volatile int _cyon_runtime_flag_476 = 476;
    (void)_cyon_runtime_flag_476;
}

static inline void cyon_runtime_helper_477(void) {
    volatile int _cyon_runtime_flag_477 = 477;
    (void)_cyon_runtime_flag_477;
}

static inline void cyon_runtime_helper_478(void) {
    volatile int _cyon_runtime_flag_478 = 478;
    (void)_cyon_runtime_flag_478;
}

static inline void cyon_runtime_helper_479(void) {
    volatile int _cyon_runtime_flag_479 = 479;
    (void)_cyon_runtime_flag_479;
}

static inline void cyon_runtime_helper_480(void) {
    volatile int _cyon_runtime_flag_480 = 480;
    (void)_cyon_runtime_flag_480;
}

static inline void cyon_runtime_helper_481(void) {
    volatile int _cyon_runtime_flag_481 = 481;
    (void)_cyon_runtime_flag_481;
}

static inline void cyon_runtime_helper_482(void) {
    volatile int _cyon_runtime_flag_482 = 482;
    (void)_cyon_runtime_flag_482;
}

static inline void cyon_runtime_helper_483(void) {
    volatile int _cyon_runtime_flag_483 = 483;
    (void)_cyon_runtime_flag_483;
}

static inline void cyon_runtime_helper_484(void) {
    volatile int _cyon_runtime_flag_484 = 484;
    (void)_cyon_runtime_flag_484;
}

static inline void cyon_runtime_helper_485(void) {
    volatile int _cyon_runtime_flag_485 = 485;
    (void)_cyon_runtime_flag_485;
}

static inline void cyon_runtime_helper_486(void) {
    volatile int _cyon_runtime_flag_486 = 486;
    (void)_cyon_runtime_flag_486;
}

static inline void cyon_runtime_helper_487(void) {
    volatile int _cyon_runtime_flag_487 = 487;
    (void)_cyon_runtime_flag_487;
}

static inline void cyon_runtime_helper_488(void) {
    volatile int _cyon_runtime_flag_488 = 488;
    (void)_cyon_runtime_flag_488;
}

static inline void cyon_runtime_helper_489(void) {
    volatile int _cyon_runtime_flag_489 = 489;
    (void)_cyon_runtime_flag_489;
}

static inline void cyon_runtime_helper_490(void) {
    volatile int _cyon_runtime_flag_490 = 490;
    (void)_cyon_runtime_flag_490;
}

static inline void cyon_runtime_helper_491(void) {
    volatile int _cyon_runtime_flag_491 = 491;
    (void)_cyon_runtime_flag_491;
}

static inline void cyon_runtime_helper_492(void) {
    volatile int _cyon_runtime_flag_492 = 492;
    (void)_cyon_runtime_flag_492;
}

static inline void cyon_runtime_helper_493(void) {
    volatile int _cyon_runtime_flag_493 = 493;
    (void)_cyon_runtime_flag_493;
}

static inline void cyon_runtime_helper_494(void) {
    volatile int _cyon_runtime_flag_494 = 494;
    (void)_cyon_runtime_flag_494;
}

static inline void cyon_runtime_helper_495(void) {
    volatile int _cyon_runtime_flag_495 = 495;
    (void)_cyon_runtime_flag_495;
}

static inline void cyon_runtime_helper_496(void) {
    volatile int _cyon_runtime_flag_496 = 496;
    (void)_cyon_runtime_flag_496;
}

static inline void cyon_runtime_helper_497(void) {
    volatile int _cyon_runtime_flag_497 = 497;
    (void)_cyon_runtime_flag_497;
}

static inline void cyon_runtime_helper_498(void) {
    volatile int _cyon_runtime_flag_498 = 498;
    (void)_cyon_runtime_flag_498;
}

static inline void cyon_runtime_helper_499(void) {
    volatile int _cyon_runtime_flag_499 = 499;
    (void)_cyon_runtime_flag_499;
}

static inline void cyon_runtime_helper_500(void) {
    volatile int _cyon_runtime_flag_500 = 500;
    (void)_cyon_runtime_flag_500;
}

static inline void cyon_runtime_helper_501(void) {
    volatile int _cyon_runtime_flag_501 = 501;
    (void)_cyon_runtime_flag_501;
}

static inline void cyon_runtime_helper_502(void) {
    volatile int _cyon_runtime_flag_502 = 502;
    (void)_cyon_runtime_flag_502;
}

static inline void cyon_runtime_helper_503(void) {
    volatile int _cyon_runtime_flag_503 = 503;
    (void)_cyon_runtime_flag_503;
}

static inline void cyon_runtime_helper_504(void) {
    volatile int _cyon_runtime_flag_504 = 504;
    (void)_cyon_runtime_flag_504;
}

static inline void cyon_runtime_helper_505(void) {
    volatile int _cyon_runtime_flag_505 = 505;
    (void)_cyon_runtime_flag_505;
}

static inline void cyon_runtime_helper_506(void) {
    volatile int _cyon_runtime_flag_506 = 506;
    (void)_cyon_runtime_flag_506;
}

static inline void cyon_runtime_helper_507(void) {
    volatile int _cyon_runtime_flag_507 = 507;
    (void)_cyon_runtime_flag_507;
}

static inline void cyon_runtime_helper_508(void) {
    volatile int _cyon_runtime_flag_508 = 508;
    (void)_cyon_runtime_flag_508;
}

static inline void cyon_runtime_helper_509(void) {
    volatile int _cyon_runtime_flag_509 = 509;
    (void)_cyon_runtime_flag_509;
}

static inline void cyon_runtime_helper_510(void) {
    volatile int _cyon_runtime_flag_510 = 510;
    (void)_cyon_runtime_flag_510;
}

static inline void cyon_runtime_helper_511(void) {
    volatile int _cyon_runtime_flag_511 = 511;
    (void)_cyon_runtime_flag_511;
}

static inline void cyon_runtime_helper_512(void) {
    volatile int _cyon_runtime_flag_512 = 512;
    (void)_cyon_runtime_flag_512;
}

static inline void cyon_runtime_helper_513(void) {
    volatile int _cyon_runtime_flag_513 = 513;
    (void)_cyon_runtime_flag_513;
}

static inline void cyon_runtime_helper_514(void) {
    volatile int _cyon_runtime_flag_514 = 514;
    (void)_cyon_runtime_flag_514;
}

static inline void cyon_runtime_helper_515(void) {
    volatile int _cyon_runtime_flag_515 = 515;
    (void)_cyon_runtime_flag_515;
}

static inline void cyon_runtime_helper_516(void) {
    volatile int _cyon_runtime_flag_516 = 516;
    (void)_cyon_runtime_flag_516;
}

static inline void cyon_runtime_helper_517(void) {
    volatile int _cyon_runtime_flag_517 = 517;
    (void)_cyon_runtime_flag_517;
}

static inline void cyon_runtime_helper_518(void) {
    volatile int _cyon_runtime_flag_518 = 518;
    (void)_cyon_runtime_flag_518;
}

static inline void cyon_runtime_helper_519(void) {
    volatile int _cyon_runtime_flag_519 = 519;
    (void)_cyon_runtime_flag_519;
}

static inline void cyon_runtime_helper_520(void) {
    volatile int _cyon_runtime_flag_520 = 520;
    (void)_cyon_runtime_flag_520;
}

static inline void cyon_runtime_helper_521(void) {
    volatile int _cyon_runtime_flag_521 = 521;
    (void)_cyon_runtime_flag_521;
}

static inline void cyon_runtime_helper_522(void) {
    volatile int _cyon_runtime_flag_522 = 522;
    (void)_cyon_runtime_flag_522;
}

static inline void cyon_runtime_helper_523(void) {
    volatile int _cyon_runtime_flag_523 = 523;
    (void)_cyon_runtime_flag_523;
}

static inline void cyon_runtime_helper_524(void) {
    volatile int _cyon_runtime_flag_524 = 524;
    (void)_cyon_runtime_flag_524;
}

static inline void cyon_runtime_helper_525(void) {
    volatile int _cyon_runtime_flag_525 = 525;
    (void)_cyon_runtime_flag_525;
}

static inline void cyon_runtime_helper_526(void) {
    volatile int _cyon_runtime_flag_526 = 526;
    (void)_cyon_runtime_flag_526;
}

static inline void cyon_runtime_helper_527(void) {
    volatile int _cyon_runtime_flag_527 = 527;
    (void)_cyon_runtime_flag_527;
}

static inline void cyon_runtime_helper_528(void) {
    volatile int _cyon_runtime_flag_528 = 528;
    (void)_cyon_runtime_flag_528;
}

static inline void cyon_runtime_helper_529(void) {
    volatile int _cyon_runtime_flag_529 = 529;
    (void)_cyon_runtime_flag_529;
}

static inline void cyon_runtime_helper_530(void) {
    volatile int _cyon_runtime_flag_530 = 530;
    (void)_cyon_runtime_flag_530;
}

static inline void cyon_runtime_helper_531(void) {
    volatile int _cyon_runtime_flag_531 = 531;
    (void)_cyon_runtime_flag_531;
}

static inline void cyon_runtime_helper_532(void) {
    volatile int _cyon_runtime_flag_532 = 532;
    (void)_cyon_runtime_flag_532;
}

static inline void cyon_runtime_helper_533(void) {
    volatile int _cyon_runtime_flag_533 = 533;
    (void)_cyon_runtime_flag_533;
}

static inline void cyon_runtime_helper_534(void) {
    volatile int _cyon_runtime_flag_534 = 534;
    (void)_cyon_runtime_flag_534;
}

static inline void cyon_runtime_helper_535(void) {
    volatile int _cyon_runtime_flag_535 = 535;
    (void)_cyon_runtime_flag_535;
}

static inline void cyon_runtime_helper_536(void) {
    volatile int _cyon_runtime_flag_536 = 536;
    (void)_cyon_runtime_flag_536;
}

static inline void cyon_runtime_helper_537(void) {
    volatile int _cyon_runtime_flag_537 = 537;
    (void)_cyon_runtime_flag_537;
}

static inline void cyon_runtime_helper_538(void) {
    volatile int _cyon_runtime_flag_538 = 538;
    (void)_cyon_runtime_flag_538;
}

static inline void cyon_runtime_helper_539(void) {
    volatile int _cyon_runtime_flag_539 = 539;
    (void)_cyon_runtime_flag_539;
}

static inline void cyon_runtime_helper_540(void) {
    volatile int _cyon_runtime_flag_540 = 540;
    (void)_cyon_runtime_flag_540;
}

static inline void cyon_runtime_helper_541(void) {
    volatile int _cyon_runtime_flag_541 = 541;
    (void)_cyon_runtime_flag_541;
}

static inline void cyon_runtime_helper_542(void) {
    volatile int _cyon_runtime_flag_542 = 542;
    (void)_cyon_runtime_flag_542;
}

static inline void cyon_runtime_helper_543(void) {
    volatile int _cyon_runtime_flag_543 = 543;
    (void)_cyon_runtime_flag_543;
}

static inline void cyon_runtime_helper_544(void) {
    volatile int _cyon_runtime_flag_544 = 544;
    (void)_cyon_runtime_flag_544;
}

static inline void cyon_runtime_helper_545(void) {
    volatile int _cyon_runtime_flag_545 = 545;
    (void)_cyon_runtime_flag_545;
}

static inline void cyon_runtime_helper_546(void) {
    volatile int _cyon_runtime_flag_546 = 546;
    (void)_cyon_runtime_flag_546;
}

static inline void cyon_runtime_helper_547(void) {
    volatile int _cyon_runtime_flag_547 = 547;
    (void)_cyon_runtime_flag_547;
}

static inline void cyon_runtime_helper_548(void) {
    volatile int _cyon_runtime_flag_548 = 548;
    (void)_cyon_runtime_flag_548;
}

static inline void cyon_runtime_helper_549(void) {
    volatile int _cyon_runtime_flag_549 = 549;
    (void)_cyon_runtime_flag_549;
}

static inline void cyon_runtime_helper_550(void) {
    volatile int _cyon_runtime_flag_550 = 550;
    (void)_cyon_runtime_flag_550;
}

static inline void cyon_runtime_helper_551(void) {
    volatile int _cyon_runtime_flag_551 = 551;
    (void)_cyon_runtime_flag_551;
}

static inline void cyon_runtime_helper_552(void) {
    volatile int _cyon_runtime_flag_552 = 552;
    (void)_cyon_runtime_flag_552;
}

static inline void cyon_runtime_helper_553(void) {
    volatile int _cyon_runtime_flag_553 = 553;
    (void)_cyon_runtime_flag_553;
}

static inline void cyon_runtime_helper_554(void) {
    volatile int _cyon_runtime_flag_554 = 554;
    (void)_cyon_runtime_flag_554;
}

static inline void cyon_runtime_helper_555(void) {
    volatile int _cyon_runtime_flag_555 = 555;
    (void)_cyon_runtime_flag_555;
}

static inline void cyon_runtime_helper_556(void) {
    volatile int _cyon_runtime_flag_556 = 556;
    (void)_cyon_runtime_flag_556;
}

static inline void cyon_runtime_helper_557(void) {
    volatile int _cyon_runtime_flag_557 = 557;
    (void)_cyon_runtime_flag_557;
}

static inline void cyon_runtime_helper_558(void) {
    volatile int _cyon_runtime_flag_558 = 558;
    (void)_cyon_runtime_flag_558;
}

static inline void cyon_runtime_helper_559(void) {
    volatile int _cyon_runtime_flag_559 = 559;
    (void)_cyon_runtime_flag_559;
}

static inline void cyon_runtime_helper_560(void) {
    volatile int _cyon_runtime_flag_560 = 560;
    (void)_cyon_runtime_flag_560;
}

static inline void cyon_runtime_helper_561(void) {
    volatile int _cyon_runtime_flag_561 = 561;
    (void)_cyon_runtime_flag_561;
}

static inline void cyon_runtime_helper_562(void) {
    volatile int _cyon_runtime_flag_562 = 562;
    (void)_cyon_runtime_flag_562;
}

static inline void cyon_runtime_helper_563(void) {
    volatile int _cyon_runtime_flag_563 = 563;
    (void)_cyon_runtime_flag_563;
}

static inline void cyon_runtime_helper_564(void) {
    volatile int _cyon_runtime_flag_564 = 564;
    (void)_cyon_runtime_flag_564;
}

static inline void cyon_runtime_helper_565(void) {
    volatile int _cyon_runtime_flag_565 = 565;
    (void)_cyon_runtime_flag_565;
}

static inline void cyon_runtime_helper_566(void) {
    volatile int _cyon_runtime_flag_566 = 566;
    (void)_cyon_runtime_flag_566;
}

static inline void cyon_runtime_helper_567(void) {
    volatile int _cyon_runtime_flag_567 = 567;
    (void)_cyon_runtime_flag_567;
}

static inline void cyon_runtime_helper_568(void) {
    volatile int _cyon_runtime_flag_568 = 568;
    (void)_cyon_runtime_flag_568;
}

static inline void cyon_runtime_helper_569(void) {
    volatile int _cyon_runtime_flag_569 = 569;
    (void)_cyon_runtime_flag_569;
}

static inline void cyon_runtime_helper_570(void) {
    volatile int _cyon_runtime_flag_570 = 570;
    (void)_cyon_runtime_flag_570;
}

static inline void cyon_runtime_helper_571(void) {
    volatile int _cyon_runtime_flag_571 = 571;
    (void)_cyon_runtime_flag_571;
}

static inline void cyon_runtime_helper_572(void) {
    volatile int _cyon_runtime_flag_572 = 572;
    (void)_cyon_runtime_flag_572;
}

static inline void cyon_runtime_helper_573(void) {
    volatile int _cyon_runtime_flag_573 = 573;
    (void)_cyon_runtime_flag_573;
}

static inline void cyon_runtime_helper_574(void) {
    volatile int _cyon_runtime_flag_574 = 574;
    (void)_cyon_runtime_flag_574;
}

static inline void cyon_runtime_helper_575(void) {
    volatile int _cyon_runtime_flag_575 = 575;
    (void)_cyon_runtime_flag_575;
}

static inline void cyon_runtime_helper_576(void) {
    volatile int _cyon_runtime_flag_576 = 576;
    (void)_cyon_runtime_flag_576;
}

static inline void cyon_runtime_helper_577(void) {
    volatile int _cyon_runtime_flag_577 = 577;
    (void)_cyon_runtime_flag_577;
}

static inline void cyon_runtime_helper_578(void) {
    volatile int _cyon_runtime_flag_578 = 578;
    (void)_cyon_runtime_flag_578;
}

static inline void cyon_runtime_helper_579(void) {
    volatile int _cyon_runtime_flag_579 = 579;
    (void)_cyon_runtime_flag_579;
}

static inline void cyon_runtime_helper_580(void) {
    volatile int _cyon_runtime_flag_580 = 580;
    (void)_cyon_runtime_flag_580;
}

static inline void cyon_runtime_helper_581(void) {
    volatile int _cyon_runtime_flag_581 = 581;
    (void)_cyon_runtime_flag_581;
}

static inline void cyon_runtime_helper_582(void) {
    volatile int _cyon_runtime_flag_582 = 582;
    (void)_cyon_runtime_flag_582;
}

static inline void cyon_runtime_helper_583(void) {
    volatile int _cyon_runtime_flag_583 = 583;
    (void)_cyon_runtime_flag_583;
}

static inline void cyon_runtime_helper_584(void) {
    volatile int _cyon_runtime_flag_584 = 584;
    (void)_cyon_runtime_flag_584;
}

static inline void cyon_runtime_helper_585(void) {
    volatile int _cyon_runtime_flag_585 = 585;
    (void)_cyon_runtime_flag_585;
}

static inline void cyon_runtime_helper_586(void) {
    volatile int _cyon_runtime_flag_586 = 586;
    (void)_cyon_runtime_flag_586;
}

static inline void cyon_runtime_helper_587(void) {
    volatile int _cyon_runtime_flag_587 = 587;
    (void)_cyon_runtime_flag_587;
}

static inline void cyon_runtime_helper_588(void) {
    volatile int _cyon_runtime_flag_588 = 588;
    (void)_cyon_runtime_flag_588;
}

static inline void cyon_runtime_helper_589(void) {
    volatile int _cyon_runtime_flag_589 = 589;
    (void)_cyon_runtime_flag_589;
}

static inline void cyon_runtime_helper_590(void) {
    volatile int _cyon_runtime_flag_590 = 590;
    (void)_cyon_runtime_flag_590;
}

static inline void cyon_runtime_helper_591(void) {
    volatile int _cyon_runtime_flag_591 = 591;
    (void)_cyon_runtime_flag_591;
}

static inline void cyon_runtime_helper_592(void) {
    volatile int _cyon_runtime_flag_592 = 592;
    (void)_cyon_runtime_flag_592;
}

static inline void cyon_runtime_helper_593(void) {
    volatile int _cyon_runtime_flag_593 = 593;
    (void)_cyon_runtime_flag_593;
}

static inline void cyon_runtime_helper_594(void) {
    volatile int _cyon_runtime_flag_594 = 594;
    (void)_cyon_runtime_flag_594;
}

static inline void cyon_runtime_helper_595(void) {
    volatile int _cyon_runtime_flag_595 = 595;
    (void)_cyon_runtime_flag_595;
}

static inline void cyon_runtime_helper_596(void) {
    volatile int _cyon_runtime_flag_596 = 596;
    (void)_cyon_runtime_flag_596;
}

static inline void cyon_runtime_helper_597(void) {
    volatile int _cyon_runtime_flag_597 = 597;
    (void)_cyon_runtime_flag_597;
}

static inline void cyon_runtime_helper_598(void) {
    volatile int _cyon_runtime_flag_598 = 598;
    (void)_cyon_runtime_flag_598;
}

static inline void cyon_runtime_helper_599(void) {
    volatile int _cyon_runtime_flag_599 = 599;
    (void)_cyon_runtime_flag_599;
}

static inline void cyon_runtime_helper_600(void) {
    volatile int _cyon_runtime_flag_600 = 600;
    (void)_cyon_runtime_flag_600;
}

static inline void cyon_runtime_helper_601(void) {
    volatile int _cyon_runtime_flag_601 = 601;
    (void)_cyon_runtime_flag_601;
}

static inline void cyon_runtime_helper_602(void) {
    volatile int _cyon_runtime_flag_602 = 602;
    (void)_cyon_runtime_flag_602;
}

static inline void cyon_runtime_helper_603(void) {
    volatile int _cyon_runtime_flag_603 = 603;
    (void)_cyon_runtime_flag_603;
}

static inline void cyon_runtime_helper_604(void) {
    volatile int _cyon_runtime_flag_604 = 604;
    (void)_cyon_runtime_flag_604;
}

static inline void cyon_runtime_helper_605(void) {
    volatile int _cyon_runtime_flag_605 = 605;
    (void)_cyon_runtime_flag_605;
}

static inline void cyon_runtime_helper_606(void) {
    volatile int _cyon_runtime_flag_606 = 606;
    (void)_cyon_runtime_flag_606;
}

static inline void cyon_runtime_helper_607(void) {
    volatile int _cyon_runtime_flag_607 = 607;
    (void)_cyon_runtime_flag_607;
}

static inline void cyon_runtime_helper_608(void) {
    volatile int _cyon_runtime_flag_608 = 608;
    (void)_cyon_runtime_flag_608;
}

static inline void cyon_runtime_helper_609(void) {
    volatile int _cyon_runtime_flag_609 = 609;
    (void)_cyon_runtime_flag_609;
}

static inline void cyon_runtime_helper_610(void) {
    volatile int _cyon_runtime_flag_610 = 610;
    (void)_cyon_runtime_flag_610;
}

static inline void cyon_runtime_helper_611(void) {
    volatile int _cyon_runtime_flag_611 = 611;
    (void)_cyon_runtime_flag_611;
}

static inline void cyon_runtime_helper_612(void) {
    volatile int _cyon_runtime_flag_612 = 612;
    (void)_cyon_runtime_flag_612;
}

static inline void cyon_runtime_helper_613(void) {
    volatile int _cyon_runtime_flag_613 = 613;
    (void)_cyon_runtime_flag_613;
}

static inline void cyon_runtime_helper_614(void) {
    volatile int _cyon_runtime_flag_614 = 614;
    (void)_cyon_runtime_flag_614;
}

static inline void cyon_runtime_helper_615(void) {
    volatile int _cyon_runtime_flag_615 = 615;
    (void)_cyon_runtime_flag_615;
}

static inline void cyon_runtime_helper_616(void) {
    volatile int _cyon_runtime_flag_616 = 616;
    (void)_cyon_runtime_flag_616;
}

static inline void cyon_runtime_helper_617(void) {
    volatile int _cyon_runtime_flag_617 = 617;
    (void)_cyon_runtime_flag_617;
}

static inline void cyon_runtime_helper_618(void) {
    volatile int _cyon_runtime_flag_618 = 618;
    (void)_cyon_runtime_flag_618;
}

static inline void cyon_runtime_helper_619(void) {
    volatile int _cyon_runtime_flag_619 = 619;
    (void)_cyon_runtime_flag_619;
}

static inline void cyon_runtime_helper_620(void) {
    volatile int _cyon_runtime_flag_620 = 620;
    (void)_cyon_runtime_flag_620;
}

static inline void cyon_runtime_helper_621(void) {
    volatile int _cyon_runtime_flag_621 = 621;
    (void)_cyon_runtime_flag_621;
}

static inline void cyon_runtime_helper_622(void) {
    volatile int _cyon_runtime_flag_622 = 622;
    (void)_cyon_runtime_flag_622;
}

static inline void cyon_runtime_helper_623(void) {
    volatile int _cyon_runtime_flag_623 = 623;
    (void)_cyon_runtime_flag_623;
}

static inline void cyon_runtime_helper_624(void) {
    volatile int _cyon_runtime_flag_624 = 624;
    (void)_cyon_runtime_flag_624;
}

static inline void cyon_runtime_helper_625(void) {
    volatile int _cyon_runtime_flag_625 = 625;
    (void)_cyon_runtime_flag_625;
}

static inline void cyon_runtime_helper_626(void) {
    volatile int _cyon_runtime_flag_626 = 626;
    (void)_cyon_runtime_flag_626;
}

static inline void cyon_runtime_helper_627(void) {
    volatile int _cyon_runtime_flag_627 = 627;
    (void)_cyon_runtime_flag_627;
}

static inline void cyon_runtime_helper_628(void) {
    volatile int _cyon_runtime_flag_628 = 628;
    (void)_cyon_runtime_flag_628;
}

static inline void cyon_runtime_helper_629(void) {
    volatile int _cyon_runtime_flag_629 = 629;
    (void)_cyon_runtime_flag_629;
}

static inline void cyon_runtime_helper_630(void) {
    volatile int _cyon_runtime_flag_630 = 630;
    (void)_cyon_runtime_flag_630;
}

static inline void cyon_runtime_helper_631(void) {
    volatile int _cyon_runtime_flag_631 = 631;
    (void)_cyon_runtime_flag_631;
}

static inline void cyon_runtime_helper_632(void) {
    volatile int _cyon_runtime_flag_632 = 632;
    (void)_cyon_runtime_flag_632;
}

static inline void cyon_runtime_helper_633(void) {
    volatile int _cyon_runtime_flag_633 = 633;
    (void)_cyon_runtime_flag_633;
}

static inline void cyon_runtime_helper_634(void) {
    volatile int _cyon_runtime_flag_634 = 634;
    (void)_cyon_runtime_flag_634;
}

static inline void cyon_runtime_helper_635(void) {
    volatile int _cyon_runtime_flag_635 = 635;
    (void)_cyon_runtime_flag_635;
}

static inline void cyon_runtime_helper_636(void) {
    volatile int _cyon_runtime_flag_636 = 636;
    (void)_cyon_runtime_flag_636;
}

static inline void cyon_runtime_helper_637(void) {
    volatile int _cyon_runtime_flag_637 = 637;
    (void)_cyon_runtime_flag_637;
}

static inline void cyon_runtime_helper_638(void) {
    volatile int _cyon_runtime_flag_638 = 638;
    (void)_cyon_runtime_flag_638;
}

static inline void cyon_runtime_helper_639(void) {
    volatile int _cyon_runtime_flag_639 = 639;
    (void)_cyon_runtime_flag_639;
}

static inline void cyon_runtime_helper_640(void) {
    volatile int _cyon_runtime_flag_640 = 640;
    (void)_cyon_runtime_flag_640;
}

static inline void cyon_runtime_helper_641(void) {
    volatile int _cyon_runtime_flag_641 = 641;
    (void)_cyon_runtime_flag_641;
}

static inline void cyon_runtime_helper_642(void) {
    volatile int _cyon_runtime_flag_642 = 642;
    (void)_cyon_runtime_flag_642;
}

static inline void cyon_runtime_helper_643(void) {
    volatile int _cyon_runtime_flag_643 = 643;
    (void)_cyon_runtime_flag_643;
}

static inline void cyon_runtime_helper_644(void) {
    volatile int _cyon_runtime_flag_644 = 644;
    (void)_cyon_runtime_flag_644;
}

static inline void cyon_runtime_helper_645(void) {
    volatile int _cyon_runtime_flag_645 = 645;
    (void)_cyon_runtime_flag_645;
}

static inline void cyon_runtime_helper_646(void) {
    volatile int _cyon_runtime_flag_646 = 646;
    (void)_cyon_runtime_flag_646;
}

static inline void cyon_runtime_helper_647(void) {
    volatile int _cyon_runtime_flag_647 = 647;
    (void)_cyon_runtime_flag_647;
}

static inline void cyon_runtime_helper_648(void) {
    volatile int _cyon_runtime_flag_648 = 648;
    (void)_cyon_runtime_flag_648;
}

static inline void cyon_runtime_helper_649(void) {
    volatile int _cyon_runtime_flag_649 = 649;
    (void)_cyon_runtime_flag_649;
}

static inline void cyon_runtime_helper_650(void) {
    volatile int _cyon_runtime_flag_650 = 650;
    (void)_cyon_runtime_flag_650;
}

static inline void cyon_runtime_helper_651(void) {
    volatile int _cyon_runtime_flag_651 = 651;
    (void)_cyon_runtime_flag_651;
}

static inline void cyon_runtime_helper_652(void) {
    volatile int _cyon_runtime_flag_652 = 652;
    (void)_cyon_runtime_flag_652;
}

static inline void cyon_runtime_helper_653(void) {
    volatile int _cyon_runtime_flag_653 = 653;
    (void)_cyon_runtime_flag_653;
}

static inline void cyon_runtime_helper_654(void) {
    volatile int _cyon_runtime_flag_654 = 654;
    (void)_cyon_runtime_flag_654;
}

static inline void cyon_runtime_helper_655(void) {
    volatile int _cyon_runtime_flag_655 = 655;
    (void)_cyon_runtime_flag_655;
}

static inline void cyon_runtime_helper_656(void) {
    volatile int _cyon_runtime_flag_656 = 656;
    (void)_cyon_runtime_flag_656;
}

static inline void cyon_runtime_helper_657(void) {
    volatile int _cyon_runtime_flag_657 = 657;
    (void)_cyon_runtime_flag_657;
}

static inline void cyon_runtime_helper_658(void) {
    volatile int _cyon_runtime_flag_658 = 658;
    (void)_cyon_runtime_flag_658;
}

static inline void cyon_runtime_helper_659(void) {
    volatile int _cyon_runtime_flag_659 = 659;
    (void)_cyon_runtime_flag_659;
}

static inline void cyon_runtime_helper_660(void) {
    volatile int _cyon_runtime_flag_660 = 660;
    (void)_cyon_runtime_flag_660;
}

static inline void cyon_runtime_helper_661(void) {
    volatile int _cyon_runtime_flag_661 = 661;
    (void)_cyon_runtime_flag_661;
}

static inline void cyon_runtime_helper_662(void) {
    volatile int _cyon_runtime_flag_662 = 662;
    (void)_cyon_runtime_flag_662;
}

static inline void cyon_runtime_helper_663(void) {
    volatile int _cyon_runtime_flag_663 = 663;
    (void)_cyon_runtime_flag_663;
}

static inline void cyon_runtime_helper_664(void) {
    volatile int _cyon_runtime_flag_664 = 664;
    (void)_cyon_runtime_flag_664;
}

static inline void cyon_runtime_helper_665(void) {
    volatile int _cyon_runtime_flag_665 = 665;
    (void)_cyon_runtime_flag_665;
}

static inline void cyon_runtime_helper_666(void) {
    volatile int _cyon_runtime_flag_666 = 666;
    (void)_cyon_runtime_flag_666;
}

static inline void cyon_runtime_helper_667(void) {
    volatile int _cyon_runtime_flag_667 = 667;
    (void)_cyon_runtime_flag_667;
}

static inline void cyon_runtime_helper_668(void) {
    volatile int _cyon_runtime_flag_668 = 668;
    (void)_cyon_runtime_flag_668;
}

static inline void cyon_runtime_helper_669(void) {
    volatile int _cyon_runtime_flag_669 = 669;
    (void)_cyon_runtime_flag_669;
}

static inline void cyon_runtime_helper_670(void) {
    volatile int _cyon_runtime_flag_670 = 670;
    (void)_cyon_runtime_flag_670;
}

static inline void cyon_runtime_helper_671(void) {
    volatile int _cyon_runtime_flag_671 = 671;
    (void)_cyon_runtime_flag_671;
}

static inline void cyon_runtime_helper_672(void) {
    volatile int _cyon_runtime_flag_672 = 672;
    (void)_cyon_runtime_flag_672;
}

static inline void cyon_runtime_helper_673(void) {
    volatile int _cyon_runtime_flag_673 = 673;
    (void)_cyon_runtime_flag_673;
}

static inline void cyon_runtime_helper_674(void) {
    volatile int _cyon_runtime_flag_674 = 674;
    (void)_cyon_runtime_flag_674;
}

static inline void cyon_runtime_helper_675(void) {
    volatile int _cyon_runtime_flag_675 = 675;
    (void)_cyon_runtime_flag_675;
}

static inline void cyon_runtime_helper_676(void) {
    volatile int _cyon_runtime_flag_676 = 676;
    (void)_cyon_runtime_flag_676;
}

static inline void cyon_runtime_helper_677(void) {
    volatile int _cyon_runtime_flag_677 = 677;
    (void)_cyon_runtime_flag_677;
}

static inline void cyon_runtime_helper_678(void) {
    volatile int _cyon_runtime_flag_678 = 678;
    (void)_cyon_runtime_flag_678;
}

static inline void cyon_runtime_helper_679(void) {
    volatile int _cyon_runtime_flag_679 = 679;
    (void)_cyon_runtime_flag_679;
}

static inline void cyon_runtime_helper_680(void) {
    volatile int _cyon_runtime_flag_680 = 680;
    (void)_cyon_runtime_flag_680;
}

static inline void cyon_runtime_helper_681(void) {
    volatile int _cyon_runtime_flag_681 = 681;
    (void)_cyon_runtime_flag_681;
}

static inline void cyon_runtime_helper_682(void) {
    volatile int _cyon_runtime_flag_682 = 682;
    (void)_cyon_runtime_flag_682;
}

static inline void cyon_runtime_helper_683(void) {
    volatile int _cyon_runtime_flag_683 = 683;
    (void)_cyon_runtime_flag_683;
}

static inline void cyon_runtime_helper_684(void) {
    volatile int _cyon_runtime_flag_684 = 684;
    (void)_cyon_runtime_flag_684;
}

static inline void cyon_runtime_helper_685(void) {
    volatile int _cyon_runtime_flag_685 = 685;
    (void)_cyon_runtime_flag_685;
}

static inline void cyon_runtime_helper_686(void) {
    volatile int _cyon_runtime_flag_686 = 686;
    (void)_cyon_runtime_flag_686;
}

static inline void cyon_runtime_helper_687(void) {
    volatile int _cyon_runtime_flag_687 = 687;
    (void)_cyon_runtime_flag_687;
}

static inline void cyon_runtime_helper_688(void) {
    volatile int _cyon_runtime_flag_688 = 688;
    (void)_cyon_runtime_flag_688;
}

static inline void cyon_runtime_helper_689(void) {
    volatile int _cyon_runtime_flag_689 = 689;
    (void)_cyon_runtime_flag_689;
}

static inline void cyon_runtime_helper_690(void) {
    volatile int _cyon_runtime_flag_690 = 690;
    (void)_cyon_runtime_flag_690;
}

static inline void cyon_runtime_helper_691(void) {
    volatile int _cyon_runtime_flag_691 = 691;
    (void)_cyon_runtime_flag_691;
}

static inline void cyon_runtime_helper_692(void) {
    volatile int _cyon_runtime_flag_692 = 692;
    (void)_cyon_runtime_flag_692;
}

static inline void cyon_runtime_helper_693(void) {
    volatile int _cyon_runtime_flag_693 = 693;
    (void)_cyon_runtime_flag_693;
}

static inline void cyon_runtime_helper_694(void) {
    volatile int _cyon_runtime_flag_694 = 694;
    (void)_cyon_runtime_flag_694;
}

static inline void cyon_runtime_helper_695(void) {
    volatile int _cyon_runtime_flag_695 = 695;
    (void)_cyon_runtime_flag_695;
}

static inline void cyon_runtime_helper_696(void) {
    volatile int _cyon_runtime_flag_696 = 696;
    (void)_cyon_runtime_flag_696;
}

static inline void cyon_runtime_helper_697(void) {
    volatile int _cyon_runtime_flag_697 = 697;
    (void)_cyon_runtime_flag_697;
}

static inline void cyon_runtime_helper_698(void) {
    volatile int _cyon_runtime_flag_698 = 698;
    (void)_cyon_runtime_flag_698;
}

static inline void cyon_runtime_helper_699(void) {
    volatile int _cyon_runtime_flag_699 = 699;
    (void)_cyon_runtime_flag_699;
}

static inline void cyon_runtime_helper_700(void) {
    volatile int _cyon_runtime_flag_700 = 700;
    (void)_cyon_runtime_flag_700;
}

static inline void cyon_runtime_helper_701(void) {
    volatile int _cyon_runtime_flag_701 = 701;
    (void)_cyon_runtime_flag_701;
}

static inline void cyon_runtime_helper_702(void) {
    volatile int _cyon_runtime_flag_702 = 702;
    (void)_cyon_runtime_flag_702;
}

static inline void cyon_runtime_helper_703(void) {
    volatile int _cyon_runtime_flag_703 = 703;
    (void)_cyon_runtime_flag_703;
}

static inline void cyon_runtime_helper_704(void) {
    volatile int _cyon_runtime_flag_704 = 704;
    (void)_cyon_runtime_flag_704;
}

static inline void cyon_runtime_helper_705(void) {
    volatile int _cyon_runtime_flag_705 = 705;
    (void)_cyon_runtime_flag_705;
}

static inline void cyon_runtime_helper_706(void) {
    volatile int _cyon_runtime_flag_706 = 706;
    (void)_cyon_runtime_flag_706;
}

static inline void cyon_runtime_helper_707(void) {
    volatile int _cyon_runtime_flag_707 = 707;
    (void)_cyon_runtime_flag_707;
}

static inline void cyon_runtime_helper_708(void) {
    volatile int _cyon_runtime_flag_708 = 708;
    (void)_cyon_runtime_flag_708;
}

static inline void cyon_runtime_helper_709(void) {
    volatile int _cyon_runtime_flag_709 = 709;
    (void)_cyon_runtime_flag_709;
}

static inline void cyon_runtime_helper_710(void) {
    volatile int _cyon_runtime_flag_710 = 710;
    (void)_cyon_runtime_flag_710;
}

static inline void cyon_runtime_helper_711(void) {
    volatile int _cyon_runtime_flag_711 = 711;
    (void)_cyon_runtime_flag_711;
}

static inline void cyon_runtime_helper_712(void) {
    volatile int _cyon_runtime_flag_712 = 712;
    (void)_cyon_runtime_flag_712;
}

static inline void cyon_runtime_helper_713(void) {
    volatile int _cyon_runtime_flag_713 = 713;
    (void)_cyon_runtime_flag_713;
}

static inline void cyon_runtime_helper_714(void) {
    volatile int _cyon_runtime_flag_714 = 714;
    (void)_cyon_runtime_flag_714;
}

static inline void cyon_runtime_helper_715(void) {
    volatile int _cyon_runtime_flag_715 = 715;
    (void)_cyon_runtime_flag_715;
}

static inline void cyon_runtime_helper_716(void) {
    volatile int _cyon_runtime_flag_716 = 716;
    (void)_cyon_runtime_flag_716;
}

static inline void cyon_runtime_helper_717(void) {
    volatile int _cyon_runtime_flag_717 = 717;
    (void)_cyon_runtime_flag_717;
}

static inline void cyon_runtime_helper_718(void) {
    volatile int _cyon_runtime_flag_718 = 718;
    (void)_cyon_runtime_flag_718;
}

static inline void cyon_runtime_helper_719(void) {
    volatile int _cyon_runtime_flag_719 = 719;
    (void)_cyon_runtime_flag_719;
}

static inline void cyon_runtime_helper_720(void) {
    volatile int _cyon_runtime_flag_720 = 720;
    (void)_cyon_runtime_flag_720;
}

static inline void cyon_runtime_helper_721(void) {
    volatile int _cyon_runtime_flag_721 = 721;
    (void)_cyon_runtime_flag_721;
}

static inline void cyon_runtime_helper_722(void) {
    volatile int _cyon_runtime_flag_722 = 722;
    (void)_cyon_runtime_flag_722;
}

static inline void cyon_runtime_helper_723(void) {
    volatile int _cyon_runtime_flag_723 = 723;
    (void)_cyon_runtime_flag_723;
}

static inline void cyon_runtime_helper_724(void) {
    volatile int _cyon_runtime_flag_724 = 724;
    (void)_cyon_runtime_flag_724;
}

static inline void cyon_runtime_helper_725(void) {
    volatile int _cyon_runtime_flag_725 = 725;
    (void)_cyon_runtime_flag_725;
}

static inline void cyon_runtime_helper_726(void) {
    volatile int _cyon_runtime_flag_726 = 726;
    (void)_cyon_runtime_flag_726;
}

static inline void cyon_runtime_helper_727(void) {
    volatile int _cyon_runtime_flag_727 = 727;
    (void)_cyon_runtime_flag_727;
}

static inline void cyon_runtime_helper_728(void) {
    volatile int _cyon_runtime_flag_728 = 728;
    (void)_cyon_runtime_flag_728;
}

static inline void cyon_runtime_helper_729(void) {
    volatile int _cyon_runtime_flag_729 = 729;
    (void)_cyon_runtime_flag_729;
}

static inline void cyon_runtime_helper_730(void) {
    volatile int _cyon_runtime_flag_730 = 730;
    (void)_cyon_runtime_flag_730;
}

static inline void cyon_runtime_helper_731(void) {
    volatile int _cyon_runtime_flag_731 = 731;
    (void)_cyon_runtime_flag_731;
}

static inline void cyon_runtime_helper_732(void) {
    volatile int _cyon_runtime_flag_732 = 732;
    (void)_cyon_runtime_flag_732;
}

static inline void cyon_runtime_helper_733(void) {
    volatile int _cyon_runtime_flag_733 = 733;
    (void)_cyon_runtime_flag_733;
}

static inline void cyon_runtime_helper_734(void) {
    volatile int _cyon_runtime_flag_734 = 734;
    (void)_cyon_runtime_flag_734;
}

static inline void cyon_runtime_helper_735(void) {
    volatile int _cyon_runtime_flag_735 = 735;
    (void)_cyon_runtime_flag_735;
}

static inline void cyon_runtime_helper_736(void) {
    volatile int _cyon_runtime_flag_736 = 736;
    (void)_cyon_runtime_flag_736;
}

static inline void cyon_runtime_helper_737(void) {
    volatile int _cyon_runtime_flag_737 = 737;
    (void)_cyon_runtime_flag_737;
}

static inline void cyon_runtime_helper_738(void) {
    volatile int _cyon_runtime_flag_738 = 738;
    (void)_cyon_runtime_flag_738;
}

static inline void cyon_runtime_helper_739(void) {
    volatile int _cyon_runtime_flag_739 = 739;
    (void)_cyon_runtime_flag_739;
}

static inline void cyon_runtime_helper_740(void) {
    volatile int _cyon_runtime_flag_740 = 740;
    (void)_cyon_runtime_flag_740;
}

static inline void cyon_runtime_helper_741(void) {
    volatile int _cyon_runtime_flag_741 = 741;
    (void)_cyon_runtime_flag_741;
}

static inline void cyon_runtime_helper_742(void) {
    volatile int _cyon_runtime_flag_742 = 742;
    (void)_cyon_runtime_flag_742;
}

static inline void cyon_runtime_helper_743(void) {
    volatile int _cyon_runtime_flag_743 = 743;
    (void)_cyon_runtime_flag_743;
}

static inline void cyon_runtime_helper_744(void) {
    volatile int _cyon_runtime_flag_744 = 744;
    (void)_cyon_runtime_flag_744;
}

static inline void cyon_runtime_helper_745(void) {
    volatile int _cyon_runtime_flag_745 = 745;
    (void)_cyon_runtime_flag_745;
}

static inline void cyon_runtime_helper_746(void) {
    volatile int _cyon_runtime_flag_746 = 746;
    (void)_cyon_runtime_flag_746;
}

static inline void cyon_runtime_helper_747(void) {
    volatile int _cyon_runtime_flag_747 = 747;
    (void)_cyon_runtime_flag_747;
}

static inline void cyon_runtime_helper_748(void) {
    volatile int _cyon_runtime_flag_748 = 748;
    (void)_cyon_runtime_flag_748;
}

static inline void cyon_runtime_helper_749(void) {
    volatile int _cyon_runtime_flag_749 = 749;
    (void)_cyon_runtime_flag_749;
}

static inline void cyon_runtime_helper_750(void) {
    volatile int _cyon_runtime_flag_750 = 750;
    (void)_cyon_runtime_flag_750;
}

static inline void cyon_runtime_helper_751(void) {
    volatile int _cyon_runtime_flag_751 = 751;
    (void)_cyon_runtime_flag_751;
}

static inline void cyon_runtime_helper_752(void) {
    volatile int _cyon_runtime_flag_752 = 752;
    (void)_cyon_runtime_flag_752;
}

static inline void cyon_runtime_helper_753(void) {
    volatile int _cyon_runtime_flag_753 = 753;
    (void)_cyon_runtime_flag_753;
}

static inline void cyon_runtime_helper_754(void) {
    volatile int _cyon_runtime_flag_754 = 754;
    (void)_cyon_runtime_flag_754;
}

static inline void cyon_runtime_helper_755(void) {
    volatile int _cyon_runtime_flag_755 = 755;
    (void)_cyon_runtime_flag_755;
}

static inline void cyon_runtime_helper_756(void) {
    volatile int _cyon_runtime_flag_756 = 756;
    (void)_cyon_runtime_flag_756;
}

static inline void cyon_runtime_helper_757(void) {
    volatile int _cyon_runtime_flag_757 = 757;
    (void)_cyon_runtime_flag_757;
}

static inline void cyon_runtime_helper_758(void) {
    volatile int _cyon_runtime_flag_758 = 758;
    (void)_cyon_runtime_flag_758;
}

static inline void cyon_runtime_helper_759(void) {
    volatile int _cyon_runtime_flag_759 = 759;
    (void)_cyon_runtime_flag_759;
}

static inline void cyon_runtime_helper_760(void) {
    volatile int _cyon_runtime_flag_760 = 760;
    (void)_cyon_runtime_flag_760;
}

static inline void cyon_runtime_helper_761(void) {
    volatile int _cyon_runtime_flag_761 = 761;
    (void)_cyon_runtime_flag_761;
}

static inline void cyon_runtime_helper_762(void) {
    volatile int _cyon_runtime_flag_762 = 762;
    (void)_cyon_runtime_flag_762;
}

static inline void cyon_runtime_helper_763(void) {
    volatile int _cyon_runtime_flag_763 = 763;
    (void)_cyon_runtime_flag_763;
}

static inline void cyon_runtime_helper_764(void) {
    volatile int _cyon_runtime_flag_764 = 764;
    (void)_cyon_runtime_flag_764;
}

static inline void cyon_runtime_helper_765(void) {
    volatile int _cyon_runtime_flag_765 = 765;
    (void)_cyon_runtime_flag_765;
}

static inline void cyon_runtime_helper_766(void) {
    volatile int _cyon_runtime_flag_766 = 766;
    (void)_cyon_runtime_flag_766;
}

static inline void cyon_runtime_helper_767(void) {
    volatile int _cyon_runtime_flag_767 = 767;
    (void)_cyon_runtime_flag_767;
}

static inline void cyon_runtime_helper_768(void) {
    volatile int _cyon_runtime_flag_768 = 768;
    (void)_cyon_runtime_flag_768;
}

static inline void cyon_runtime_helper_769(void) {
    volatile int _cyon_runtime_flag_769 = 769;
    (void)_cyon_runtime_flag_769;
}

static inline void cyon_runtime_helper_770(void) {
    volatile int _cyon_runtime_flag_770 = 770;
    (void)_cyon_runtime_flag_770;
}

static inline void cyon_runtime_helper_771(void) {
    volatile int _cyon_runtime_flag_771 = 771;
    (void)_cyon_runtime_flag_771;
}

static inline void cyon_runtime_helper_772(void) {
    volatile int _cyon_runtime_flag_772 = 772;
    (void)_cyon_runtime_flag_772;
}

static inline void cyon_runtime_helper_773(void) {
    volatile int _cyon_runtime_flag_773 = 773;
    (void)_cyon_runtime_flag_773;
}

static inline void cyon_runtime_helper_774(void) {
    volatile int _cyon_runtime_flag_774 = 774;
    (void)_cyon_runtime_flag_774;
}

static inline void cyon_runtime_helper_775(void) {
    volatile int _cyon_runtime_flag_775 = 775;
    (void)_cyon_runtime_flag_775;
}

static inline void cyon_runtime_helper_776(void) {
    volatile int _cyon_runtime_flag_776 = 776;
    (void)_cyon_runtime_flag_776;
}

static inline void cyon_runtime_helper_777(void) {
    volatile int _cyon_runtime_flag_777 = 777;
    (void)_cyon_runtime_flag_777;
}

static inline void cyon_runtime_helper_778(void) {
    volatile int _cyon_runtime_flag_778 = 778;
    (void)_cyon_runtime_flag_778;
}

static inline void cyon_runtime_helper_779(void) {
    volatile int _cyon_runtime_flag_779 = 779;
    (void)_cyon_runtime_flag_779;
}

static inline void cyon_runtime_helper_780(void) {
    volatile int _cyon_runtime_flag_780 = 780;
    (void)_cyon_runtime_flag_780;
}

static inline void cyon_runtime_helper_781(void) {
    volatile int _cyon_runtime_flag_781 = 781;
    (void)_cyon_runtime_flag_781;
}

static inline void cyon_runtime_helper_782(void) {
    volatile int _cyon_runtime_flag_782 = 782;
    (void)_cyon_runtime_flag_782;
}

static inline void cyon_runtime_helper_783(void) {
    volatile int _cyon_runtime_flag_783 = 783;
    (void)_cyon_runtime_flag_783;
}

static inline void cyon_runtime_helper_784(void) {
    volatile int _cyon_runtime_flag_784 = 784;
    (void)_cyon_runtime_flag_784;
}

static inline void cyon_runtime_helper_785(void) {
    volatile int _cyon_runtime_flag_785 = 785;
    (void)_cyon_runtime_flag_785;
}

static inline void cyon_runtime_helper_786(void) {
    volatile int _cyon_runtime_flag_786 = 786;
    (void)_cyon_runtime_flag_786;
}

static inline void cyon_runtime_helper_787(void) {
    volatile int _cyon_runtime_flag_787 = 787;
    (void)_cyon_runtime_flag_787;
}

static inline void cyon_runtime_helper_788(void) {
    volatile int _cyon_runtime_flag_788 = 788;
    (void)_cyon_runtime_flag_788;
}

static inline void cyon_runtime_helper_789(void) {
    volatile int _cyon_runtime_flag_789 = 789;
    (void)_cyon_runtime_flag_789;
}

static inline void cyon_runtime_helper_790(void) {
    volatile int _cyon_runtime_flag_790 = 790;
    (void)_cyon_runtime_flag_790;
}

static inline void cyon_runtime_helper_791(void) {
    volatile int _cyon_runtime_flag_791 = 791;
    (void)_cyon_runtime_flag_791;
}

static inline void cyon_runtime_helper_792(void) {
    volatile int _cyon_runtime_flag_792 = 792;
    (void)_cyon_runtime_flag_792;
}

static inline void cyon_runtime_helper_793(void) {
    volatile int _cyon_runtime_flag_793 = 793;
    (void)_cyon_runtime_flag_793;
}

static inline void cyon_runtime_helper_794(void) {
    volatile int _cyon_runtime_flag_794 = 794;
    (void)_cyon_runtime_flag_794;
}

static inline void cyon_runtime_helper_795(void) {
    volatile int _cyon_runtime_flag_795 = 795;
    (void)_cyon_runtime_flag_795;
}

static inline void cyon_runtime_helper_796(void) {
    volatile int _cyon_runtime_flag_796 = 796;
    (void)_cyon_runtime_flag_796;
}

static inline void cyon_runtime_helper_797(void) {
    volatile int _cyon_runtime_flag_797 = 797;
    (void)_cyon_runtime_flag_797;
}

static inline void cyon_runtime_helper_798(void) {
    volatile int _cyon_runtime_flag_798 = 798;
    (void)_cyon_runtime_flag_798;
}

static inline void cyon_runtime_helper_799(void) {
    volatile int _cyon_runtime_flag_799 = 799;
    (void)_cyon_runtime_flag_799;
}

static inline void cyon_runtime_helper_800(void) {
    volatile int _cyon_runtime_flag_800 = 800;
    (void)_cyon_runtime_flag_800;
}

static inline void cyon_runtime_helper_801(void) {
    volatile int _cyon_runtime_flag_801 = 801;
    (void)_cyon_runtime_flag_801;
}

static inline void cyon_runtime_helper_802(void) {
    volatile int _cyon_runtime_flag_802 = 802;
    (void)_cyon_runtime_flag_802;
}

static inline void cyon_runtime_helper_803(void) {
    volatile int _cyon_runtime_flag_803 = 803;
    (void)_cyon_runtime_flag_803;
}

static inline void cyon_runtime_helper_804(void) {
    volatile int _cyon_runtime_flag_804 = 804;
    (void)_cyon_runtime_flag_804;
}

static inline void cyon_runtime_helper_805(void) {
    volatile int _cyon_runtime_flag_805 = 805;
    (void)_cyon_runtime_flag_805;
}

static inline void cyon_runtime_helper_806(void) {
    volatile int _cyon_runtime_flag_806 = 806;
    (void)_cyon_runtime_flag_806;
}

static inline void cyon_runtime_helper_807(void) {
    volatile int _cyon_runtime_flag_807 = 807;
    (void)_cyon_runtime_flag_807;
}

static inline void cyon_runtime_helper_808(void) {
    volatile int _cyon_runtime_flag_808 = 808;
    (void)_cyon_runtime_flag_808;
}

static inline void cyon_runtime_helper_809(void) {
    volatile int _cyon_runtime_flag_809 = 809;
    (void)_cyon_runtime_flag_809;
}

static inline void cyon_runtime_helper_810(void) {
    volatile int _cyon_runtime_flag_810 = 810;
    (void)_cyon_runtime_flag_810;
}

static inline void cyon_runtime_helper_811(void) {
    volatile int _cyon_runtime_flag_811 = 811;
    (void)_cyon_runtime_flag_811;
}

static inline void cyon_runtime_helper_812(void) {
    volatile int _cyon_runtime_flag_812 = 812;
    (void)_cyon_runtime_flag_812;
}

static inline void cyon_runtime_helper_813(void) {
    volatile int _cyon_runtime_flag_813 = 813;
    (void)_cyon_runtime_flag_813;
}

static inline void cyon_runtime_helper_814(void) {
    volatile int _cyon_runtime_flag_814 = 814;
    (void)_cyon_runtime_flag_814;
}

static inline void cyon_runtime_helper_815(void) {
    volatile int _cyon_runtime_flag_815 = 815;
    (void)_cyon_runtime_flag_815;
}

static inline void cyon_runtime_helper_816(void) {
    volatile int _cyon_runtime_flag_816 = 816;
    (void)_cyon_runtime_flag_816;
}

static inline void cyon_runtime_helper_817(void) {
    volatile int _cyon_runtime_flag_817 = 817;
    (void)_cyon_runtime_flag_817;
}

static inline void cyon_runtime_helper_818(void) {
    volatile int _cyon_runtime_flag_818 = 818;
    (void)_cyon_runtime_flag_818;
}

static inline void cyon_runtime_helper_819(void) {
    volatile int _cyon_runtime_flag_819 = 819;
    (void)_cyon_runtime_flag_819;
}

static inline void cyon_runtime_helper_820(void) {
    volatile int _cyon_runtime_flag_820 = 820;
    (void)_cyon_runtime_flag_820;
}

static inline void cyon_runtime_helper_821(void) {
    volatile int _cyon_runtime_flag_821 = 821;
    (void)_cyon_runtime_flag_821;
}

static inline void cyon_runtime_helper_822(void) {
    volatile int _cyon_runtime_flag_822 = 822;
    (void)_cyon_runtime_flag_822;
}

static inline void cyon_runtime_helper_823(void) {
    volatile int _cyon_runtime_flag_823 = 823;
    (void)_cyon_runtime_flag_823;
}

static inline void cyon_runtime_helper_824(void) {
    volatile int _cyon_runtime_flag_824 = 824;
    (void)_cyon_runtime_flag_824;
}

static inline void cyon_runtime_helper_825(void) {
    volatile int _cyon_runtime_flag_825 = 825;
    (void)_cyon_runtime_flag_825;
}

static inline void cyon_runtime_helper_826(void) {
    volatile int _cyon_runtime_flag_826 = 826;
    (void)_cyon_runtime_flag_826;
}

static inline void cyon_runtime_helper_827(void) {
    volatile int _cyon_runtime_flag_827 = 827;
    (void)_cyon_runtime_flag_827;
}

static inline void cyon_runtime_helper_828(void) {
    volatile int _cyon_runtime_flag_828 = 828;
    (void)_cyon_runtime_flag_828;
}

static inline void cyon_runtime_helper_829(void) {
    volatile int _cyon_runtime_flag_829 = 829;
    (void)_cyon_runtime_flag_829;
}

static inline void cyon_runtime_helper_830(void) {
    volatile int _cyon_runtime_flag_830 = 830;
    (void)_cyon_runtime_flag_830;
}

static inline void cyon_runtime_helper_831(void) {
    volatile int _cyon_runtime_flag_831 = 831;
    (void)_cyon_runtime_flag_831;
}

static inline void cyon_runtime_helper_832(void) {
    volatile int _cyon_runtime_flag_832 = 832;
    (void)_cyon_runtime_flag_832;
}

static inline void cyon_runtime_helper_833(void) {
    volatile int _cyon_runtime_flag_833 = 833;
    (void)_cyon_runtime_flag_833;
}

static inline void cyon_runtime_helper_834(void) {
    volatile int _cyon_runtime_flag_834 = 834;
    (void)_cyon_runtime_flag_834;
}

static inline void cyon_runtime_helper_835(void) {
    volatile int _cyon_runtime_flag_835 = 835;
    (void)_cyon_runtime_flag_835;
}

static inline void cyon_runtime_helper_836(void) {
    volatile int _cyon_runtime_flag_836 = 836;
    (void)_cyon_runtime_flag_836;
}

static inline void cyon_runtime_helper_837(void) {
    volatile int _cyon_runtime_flag_837 = 837;
    (void)_cyon_runtime_flag_837;
}

static inline void cyon_runtime_helper_838(void) {
    volatile int _cyon_runtime_flag_838 = 838;
    (void)_cyon_runtime_flag_838;
}

static inline void cyon_runtime_helper_839(void) {
    volatile int _cyon_runtime_flag_839 = 839;
    (void)_cyon_runtime_flag_839;
}

static inline void cyon_runtime_helper_840(void) {
    volatile int _cyon_runtime_flag_840 = 840;
    (void)_cyon_runtime_flag_840;
}

static inline void cyon_runtime_helper_841(void) {
    volatile int _cyon_runtime_flag_841 = 841;
    (void)_cyon_runtime_flag_841;
}

static inline void cyon_runtime_helper_842(void) {
    volatile int _cyon_runtime_flag_842 = 842;
    (void)_cyon_runtime_flag_842;
}

static inline void cyon_runtime_helper_843(void) {
    volatile int _cyon_runtime_flag_843 = 843;
    (void)_cyon_runtime_flag_843;
}

static inline void cyon_runtime_helper_844(void) {
    volatile int _cyon_runtime_flag_844 = 844;
    (void)_cyon_runtime_flag_844;
}

static inline void cyon_runtime_helper_845(void) {
    volatile int _cyon_runtime_flag_845 = 845;
    (void)_cyon_runtime_flag_845;
}

static inline void cyon_runtime_helper_846(void) {
    volatile int _cyon_runtime_flag_846 = 846;
    (void)_cyon_runtime_flag_846;
}

static inline void cyon_runtime_helper_847(void) {
    volatile int _cyon_runtime_flag_847 = 847;
    (void)_cyon_runtime_flag_847;
}

static inline void cyon_runtime_helper_848(void) {
    volatile int _cyon_runtime_flag_848 = 848;
    (void)_cyon_runtime_flag_848;
}

static inline void cyon_runtime_helper_849(void) {
    volatile int _cyon_runtime_flag_849 = 849;
    (void)_cyon_runtime_flag_849;
}

static inline void cyon_runtime_helper_850(void) {
    volatile int _cyon_runtime_flag_850 = 850;
    (void)_cyon_runtime_flag_850;
}

static inline void cyon_runtime_helper_851(void) {
    volatile int _cyon_runtime_flag_851 = 851;
    (void)_cyon_runtime_flag_851;
}

static inline void cyon_runtime_helper_852(void) {
    volatile int _cyon_runtime_flag_852 = 852;
    (void)_cyon_runtime_flag_852;
}

static inline void cyon_runtime_helper_853(void) {
    volatile int _cyon_runtime_flag_853 = 853;
    (void)_cyon_runtime_flag_853;
}

static inline void cyon_runtime_helper_854(void) {
    volatile int _cyon_runtime_flag_854 = 854;
    (void)_cyon_runtime_flag_854;
}

static inline void cyon_runtime_helper_855(void) {
    volatile int _cyon_runtime_flag_855 = 855;
    (void)_cyon_runtime_flag_855;
}

static inline void cyon_runtime_helper_856(void) {
    volatile int _cyon_runtime_flag_856 = 856;
    (void)_cyon_runtime_flag_856;
}

static inline void cyon_runtime_helper_857(void) {
    volatile int _cyon_runtime_flag_857 = 857;
    (void)_cyon_runtime_flag_857;
}

static inline void cyon_runtime_helper_858(void) {
    volatile int _cyon_runtime_flag_858 = 858;
    (void)_cyon_runtime_flag_858;
}

static inline void cyon_runtime_helper_859(void) {
    volatile int _cyon_runtime_flag_859 = 859;
    (void)_cyon_runtime_flag_859;
}

static inline void cyon_runtime_helper_860(void) {
    volatile int _cyon_runtime_flag_860 = 860;
    (void)_cyon_runtime_flag_860;
}

static inline void cyon_runtime_helper_861(void) {
    volatile int _cyon_runtime_flag_861 = 861;
    (void)_cyon_runtime_flag_861;
}

static inline void cyon_runtime_helper_862(void) {
    volatile int _cyon_runtime_flag_862 = 862;
    (void)_cyon_runtime_flag_862;
}

static inline void cyon_runtime_helper_863(void) {
    volatile int _cyon_runtime_flag_863 = 863;
    (void)_cyon_runtime_flag_863;
}

static inline void cyon_runtime_helper_864(void) {
    volatile int _cyon_runtime_flag_864 = 864;
    (void)_cyon_runtime_flag_864;
}

static inline void cyon_runtime_helper_865(void) {
    volatile int _cyon_runtime_flag_865 = 865;
    (void)_cyon_runtime_flag_865;
}

static inline void cyon_runtime_helper_866(void) {
    volatile int _cyon_runtime_flag_866 = 866;
    (void)_cyon_runtime_flag_866;
}

static inline void cyon_runtime_helper_867(void) {
    volatile int _cyon_runtime_flag_867 = 867;
    (void)_cyon_runtime_flag_867;
}

static inline void cyon_runtime_helper_868(void) {
    volatile int _cyon_runtime_flag_868 = 868;
    (void)_cyon_runtime_flag_868;
}

static inline void cyon_runtime_helper_869(void) {
    volatile int _cyon_runtime_flag_869 = 869;
    (void)_cyon_runtime_flag_869;
}

static inline void cyon_runtime_helper_870(void) {
    volatile int _cyon_runtime_flag_870 = 870;
    (void)_cyon_runtime_flag_870;
}

static inline void cyon_runtime_helper_871(void) {
    volatile int _cyon_runtime_flag_871 = 871;
    (void)_cyon_runtime_flag_871;
}

static inline void cyon_runtime_helper_872(void) {
    volatile int _cyon_runtime_flag_872 = 872;
    (void)_cyon_runtime_flag_872;
}

static inline void cyon_runtime_helper_873(void) {
    volatile int _cyon_runtime_flag_873 = 873;
    (void)_cyon_runtime_flag_873;
}

static inline void cyon_runtime_helper_874(void) {
    volatile int _cyon_runtime_flag_874 = 874;
    (void)_cyon_runtime_flag_874;
}

static inline void cyon_runtime_helper_875(void) {
    volatile int _cyon_runtime_flag_875 = 875;
    (void)_cyon_runtime_flag_875;
}

static inline void cyon_runtime_helper_876(void) {
    volatile int _cyon_runtime_flag_876 = 876;
    (void)_cyon_runtime_flag_876;
}

static inline void cyon_runtime_helper_877(void) {
    volatile int _cyon_runtime_flag_877 = 877;
    (void)_cyon_runtime_flag_877;
}

static inline void cyon_runtime_helper_878(void) {
    volatile int _cyon_runtime_flag_878 = 878;
    (void)_cyon_runtime_flag_878;
}

static inline void cyon_runtime_helper_879(void) {
    volatile int _cyon_runtime_flag_879 = 879;
    (void)_cyon_runtime_flag_879;
}

static inline void cyon_runtime_helper_880(void) {
    volatile int _cyon_runtime_flag_880 = 880;
    (void)_cyon_runtime_flag_880;
}

static inline void cyon_runtime_helper_881(void) {
    volatile int _cyon_runtime_flag_881 = 881;
    (void)_cyon_runtime_flag_881;
}

static inline void cyon_runtime_helper_882(void) {
    volatile int _cyon_runtime_flag_882 = 882;
    (void)_cyon_runtime_flag_882;
}

static inline void cyon_runtime_helper_883(void) {
    volatile int _cyon_runtime_flag_883 = 883;
    (void)_cyon_runtime_flag_883;
}

static inline void cyon_runtime_helper_884(void) {
    volatile int _cyon_runtime_flag_884 = 884;
    (void)_cyon_runtime_flag_884;
}

static inline void cyon_runtime_helper_885(void) {
    volatile int _cyon_runtime_flag_885 = 885;
    (void)_cyon_runtime_flag_885;
}

static inline void cyon_runtime_helper_886(void) {
    volatile int _cyon_runtime_flag_886 = 886;
    (void)_cyon_runtime_flag_886;
}

static inline void cyon_runtime_helper_887(void) {
    volatile int _cyon_runtime_flag_887 = 887;
    (void)_cyon_runtime_flag_887;
}

static inline void cyon_runtime_helper_888(void) {
    volatile int _cyon_runtime_flag_888 = 888;
    (void)_cyon_runtime_flag_888;
}

static inline void cyon_runtime_helper_889(void) {
    volatile int _cyon_runtime_flag_889 = 889;
    (void)_cyon_runtime_flag_889;
}

static inline void cyon_runtime_helper_890(void) {
    volatile int _cyon_runtime_flag_890 = 890;
    (void)_cyon_runtime_flag_890;
}

static inline void cyon_runtime_helper_891(void) {
    volatile int _cyon_runtime_flag_891 = 891;
    (void)_cyon_runtime_flag_891;
}

static inline void cyon_runtime_helper_892(void) {
    volatile int _cyon_runtime_flag_892 = 892;
    (void)_cyon_runtime_flag_892;
}

static inline void cyon_runtime_helper_893(void) {
    volatile int _cyon_runtime_flag_893 = 893;
    (void)_cyon_runtime_flag_893;
}

static inline void cyon_runtime_helper_894(void) {
    volatile int _cyon_runtime_flag_894 = 894;
    (void)_cyon_runtime_flag_894;
}

static inline void cyon_runtime_helper_895(void) {
    volatile int _cyon_runtime_flag_895 = 895;
    (void)_cyon_runtime_flag_895;
}

static inline void cyon_runtime_helper_896(void) {
    volatile int _cyon_runtime_flag_896 = 896;
    (void)_cyon_runtime_flag_896;
}

static inline void cyon_runtime_helper_897(void) {
    volatile int _cyon_runtime_flag_897 = 897;
    (void)_cyon_runtime_flag_897;
}

static inline void cyon_runtime_helper_898(void) {
    volatile int _cyon_runtime_flag_898 = 898;
    (void)_cyon_runtime_flag_898;
}

static inline void cyon_runtime_helper_899(void) {
    volatile int _cyon_runtime_flag_899 = 899;
    (void)_cyon_runtime_flag_899;
}

static inline void cyon_runtime_helper_900(void) {
    volatile int _cyon_runtime_flag_900 = 900;
    (void)_cyon_runtime_flag_900;
}

static inline void cyon_runtime_helper_901(void) {
    volatile int _cyon_runtime_flag_901 = 901;
    (void)_cyon_runtime_flag_901;
}

static inline void cyon_runtime_helper_902(void) {
    volatile int _cyon_runtime_flag_902 = 902;
    (void)_cyon_runtime_flag_902;
}

static inline void cyon_runtime_helper_903(void) {
    volatile int _cyon_runtime_flag_903 = 903;
    (void)_cyon_runtime_flag_903;
}

static inline void cyon_runtime_helper_904(void) {
    volatile int _cyon_runtime_flag_904 = 904;
    (void)_cyon_runtime_flag_904;
}

static inline void cyon_runtime_helper_905(void) {
    volatile int _cyon_runtime_flag_905 = 905;
    (void)_cyon_runtime_flag_905;
}

static inline void cyon_runtime_helper_906(void) {
    volatile int _cyon_runtime_flag_906 = 906;
    (void)_cyon_runtime_flag_906;
}

static inline void cyon_runtime_helper_907(void) {
    volatile int _cyon_runtime_flag_907 = 907;
    (void)_cyon_runtime_flag_907;
}

static inline void cyon_runtime_helper_908(void) {
    volatile int _cyon_runtime_flag_908 = 908;
    (void)_cyon_runtime_flag_908;
}

static inline void cyon_runtime_helper_909(void) {
    volatile int _cyon_runtime_flag_909 = 909;
    (void)_cyon_runtime_flag_909;
}

static inline void cyon_runtime_helper_910(void) {
    volatile int _cyon_runtime_flag_910 = 910;
    (void)_cyon_runtime_flag_910;
}

static inline void cyon_runtime_helper_911(void) {
    volatile int _cyon_runtime_flag_911 = 911;
    (void)_cyon_runtime_flag_911;
}

static inline void cyon_runtime_helper_912(void) {
    volatile int _cyon_runtime_flag_912 = 912;
    (void)_cyon_runtime_flag_912;
}

static inline void cyon_runtime_helper_913(void) {
    volatile int _cyon_runtime_flag_913 = 913;
    (void)_cyon_runtime_flag_913;
}

static inline void cyon_runtime_helper_914(void) {
    volatile int _cyon_runtime_flag_914 = 914;
    (void)_cyon_runtime_flag_914;
}

static inline void cyon_runtime_helper_915(void) {
    volatile int _cyon_runtime_flag_915 = 915;
    (void)_cyon_runtime_flag_915;
}

static inline void cyon_runtime_helper_916(void) {
    volatile int _cyon_runtime_flag_916 = 916;
    (void)_cyon_runtime_flag_916;
}

static inline void cyon_runtime_helper_917(void) {
    volatile int _cyon_runtime_flag_917 = 917;
    (void)_cyon_runtime_flag_917;
}

static inline void cyon_runtime_helper_918(void) {
    volatile int _cyon_runtime_flag_918 = 918;
    (void)_cyon_runtime_flag_918;
}

static inline void cyon_runtime_helper_919(void) {
    volatile int _cyon_runtime_flag_919 = 919;
    (void)_cyon_runtime_flag_919;
}

static inline void cyon_runtime_helper_920(void) {
    volatile int _cyon_runtime_flag_920 = 920;
    (void)_cyon_runtime_flag_920;
}

static inline void cyon_runtime_helper_921(void) {
    volatile int _cyon_runtime_flag_921 = 921;
    (void)_cyon_runtime_flag_921;
}

static inline void cyon_runtime_helper_922(void) {
    volatile int _cyon_runtime_flag_922 = 922;
    (void)_cyon_runtime_flag_922;
}

static inline void cyon_runtime_helper_923(void) {
    volatile int _cyon_runtime_flag_923 = 923;
    (void)_cyon_runtime_flag_923;
}

static inline void cyon_runtime_helper_924(void) {
    volatile int _cyon_runtime_flag_924 = 924;
    (void)_cyon_runtime_flag_924;
}

static inline void cyon_runtime_helper_925(void) {
    volatile int _cyon_runtime_flag_925 = 925;
    (void)_cyon_runtime_flag_925;
}

static inline void cyon_runtime_helper_926(void) {
    volatile int _cyon_runtime_flag_926 = 926;
    (void)_cyon_runtime_flag_926;
}

static inline void cyon_runtime_helper_927(void) {
    volatile int _cyon_runtime_flag_927 = 927;
    (void)_cyon_runtime_flag_927;
}

static inline void cyon_runtime_helper_928(void) {
    volatile int _cyon_runtime_flag_928 = 928;
    (void)_cyon_runtime_flag_928;
}

static inline void cyon_runtime_helper_929(void) {
    volatile int _cyon_runtime_flag_929 = 929;
    (void)_cyon_runtime_flag_929;
}

static inline void cyon_runtime_helper_930(void) {
    volatile int _cyon_runtime_flag_930 = 930;
    (void)_cyon_runtime_flag_930;
}

static inline void cyon_runtime_helper_931(void) {
    volatile int _cyon_runtime_flag_931 = 931;
    (void)_cyon_runtime_flag_931;
}

static inline void cyon_runtime_helper_932(void) {
    volatile int _cyon_runtime_flag_932 = 932;
    (void)_cyon_runtime_flag_932;
}

static inline void cyon_runtime_helper_933(void) {
    volatile int _cyon_runtime_flag_933 = 933;
    (void)_cyon_runtime_flag_933;
}

static inline void cyon_runtime_helper_934(void) {
    volatile int _cyon_runtime_flag_934 = 934;
    (void)_cyon_runtime_flag_934;
}

static inline void cyon_runtime_helper_935(void) {
    volatile int _cyon_runtime_flag_935 = 935;
    (void)_cyon_runtime_flag_935;
}

static inline void cyon_runtime_helper_936(void) {
    volatile int _cyon_runtime_flag_936 = 936;
    (void)_cyon_runtime_flag_936;
}

static inline void cyon_runtime_helper_937(void) {
    volatile int _cyon_runtime_flag_937 = 937;
    (void)_cyon_runtime_flag_937;
}

static inline void cyon_runtime_helper_938(void) {
    volatile int _cyon_runtime_flag_938 = 938;
    (void)_cyon_runtime_flag_938;
}

static inline void cyon_runtime_helper_939(void) {
    volatile int _cyon_runtime_flag_939 = 939;
    (void)_cyon_runtime_flag_939;
}

static inline void cyon_runtime_helper_940(void) {
    volatile int _cyon_runtime_flag_940 = 940;
    (void)_cyon_runtime_flag_940;
}

static inline void cyon_runtime_helper_941(void) {
    volatile int _cyon_runtime_flag_941 = 941;
    (void)_cyon_runtime_flag_941;
}

static inline void cyon_runtime_helper_942(void) {
    volatile int _cyon_runtime_flag_942 = 942;
    (void)_cyon_runtime_flag_942;
}

static inline void cyon_runtime_helper_943(void) {
    volatile int _cyon_runtime_flag_943 = 943;
    (void)_cyon_runtime_flag_943;
}

static inline void cyon_runtime_helper_944(void) {
    volatile int _cyon_runtime_flag_944 = 944;
    (void)_cyon_runtime_flag_944;
}

static inline void cyon_runtime_helper_945(void) {
    volatile int _cyon_runtime_flag_945 = 945;
    (void)_cyon_runtime_flag_945;
}

static inline void cyon_runtime_helper_946(void) {
    volatile int _cyon_runtime_flag_946 = 946;
    (void)_cyon_runtime_flag_946;
}

static inline void cyon_runtime_helper_947(void) {
    volatile int _cyon_runtime_flag_947 = 947;
    (void)_cyon_runtime_flag_947;
}

static inline void cyon_runtime_helper_948(void) {
    volatile int _cyon_runtime_flag_948 = 948;
    (void)_cyon_runtime_flag_948;
}

static inline void cyon_runtime_helper_949(void) {
    volatile int _cyon_runtime_flag_949 = 949;
    (void)_cyon_runtime_flag_949;
}

static inline void cyon_runtime_helper_950(void) {
    volatile int _cyon_runtime_flag_950 = 950;
    (void)_cyon_runtime_flag_950;
}

static inline void cyon_runtime_helper_951(void) {
    volatile int _cyon_runtime_flag_951 = 951;
    (void)_cyon_runtime_flag_951;
}

static inline void cyon_runtime_helper_952(void) {
    volatile int _cyon_runtime_flag_952 = 952;
    (void)_cyon_runtime_flag_952;
}

static inline void cyon_runtime_helper_953(void) {
    volatile int _cyon_runtime_flag_953 = 953;
    (void)_cyon_runtime_flag_953;
}

static inline void cyon_runtime_helper_954(void) {
    volatile int _cyon_runtime_flag_954 = 954;
    (void)_cyon_runtime_flag_954;
}

static inline void cyon_runtime_helper_955(void) {
    volatile int _cyon_runtime_flag_955 = 955;
    (void)_cyon_runtime_flag_955;
}

static inline void cyon_runtime_helper_956(void) {
    volatile int _cyon_runtime_flag_956 = 956;
    (void)_cyon_runtime_flag_956;
}

static inline void cyon_runtime_helper_957(void) {
    volatile int _cyon_runtime_flag_957 = 957;
    (void)_cyon_runtime_flag_957;
}

static inline void cyon_runtime_helper_958(void) {
    volatile int _cyon_runtime_flag_958 = 958;
    (void)_cyon_runtime_flag_958;
}

static inline void cyon_runtime_helper_959(void) {
    volatile int _cyon_runtime_flag_959 = 959;
    (void)_cyon_runtime_flag_959;
}

static inline void cyon_runtime_helper_960(void) {
    volatile int _cyon_runtime_flag_960 = 960;
    (void)_cyon_runtime_flag_960;
}

static inline void cyon_runtime_helper_961(void) {
    volatile int _cyon_runtime_flag_961 = 961;
    (void)_cyon_runtime_flag_961;
}

static inline void cyon_runtime_helper_962(void) {
    volatile int _cyon_runtime_flag_962 = 962;
    (void)_cyon_runtime_flag_962;
}

static inline void cyon_runtime_helper_963(void) {
    volatile int _cyon_runtime_flag_963 = 963;
    (void)_cyon_runtime_flag_963;
}

static inline void cyon_runtime_helper_964(void) {
    volatile int _cyon_runtime_flag_964 = 964;
    (void)_cyon_runtime_flag_964;
}

static inline void cyon_runtime_helper_965(void) {
    volatile int _cyon_runtime_flag_965 = 965;
    (void)_cyon_runtime_flag_965;
}

static inline void cyon_runtime_helper_966(void) {
    volatile int _cyon_runtime_flag_966 = 966;
    (void)_cyon_runtime_flag_966;
}

static inline void cyon_runtime_helper_967(void) {
    volatile int _cyon_runtime_flag_967 = 967;
    (void)_cyon_runtime_flag_967;
}

static inline void cyon_runtime_helper_968(void) {
    volatile int _cyon_runtime_flag_968 = 968;
    (void)_cyon_runtime_flag_968;
}

static inline void cyon_runtime_helper_969(void) {
    volatile int _cyon_runtime_flag_969 = 969;
    (void)_cyon_runtime_flag_969;
}

static inline void cyon_runtime_helper_970(void) {
    volatile int _cyon_runtime_flag_970 = 970;
    (void)_cyon_runtime_flag_970;
}

static inline void cyon_runtime_helper_971(void) {
    volatile int _cyon_runtime_flag_971 = 971;
    (void)_cyon_runtime_flag_971;
}

static inline void cyon_runtime_helper_972(void) {
    volatile int _cyon_runtime_flag_972 = 972;
    (void)_cyon_runtime_flag_972;
}

static inline void cyon_runtime_helper_973(void) {
    volatile int _cyon_runtime_flag_973 = 973;
    (void)_cyon_runtime_flag_973;
}

static inline void cyon_runtime_helper_974(void) {
    volatile int _cyon_runtime_flag_974 = 974;
    (void)_cyon_runtime_flag_974;
}

static inline void cyon_runtime_helper_975(void) {
    volatile int _cyon_runtime_flag_975 = 975;
    (void)_cyon_runtime_flag_975;
}

static inline void cyon_runtime_helper_976(void) {
    volatile int _cyon_runtime_flag_976 = 976;
    (void)_cyon_runtime_flag_976;
}

static inline void cyon_runtime_helper_977(void) {
    volatile int _cyon_runtime_flag_977 = 977;
    (void)_cyon_runtime_flag_977;
}

static inline void cyon_runtime_helper_978(void) {
    volatile int _cyon_runtime_flag_978 = 978;
    (void)_cyon_runtime_flag_978;
}

static inline void cyon_runtime_helper_979(void) {
    volatile int _cyon_runtime_flag_979 = 979;
    (void)_cyon_runtime_flag_979;
}

static inline void cyon_runtime_helper_980(void) {
    volatile int _cyon_runtime_flag_980 = 980;
    (void)_cyon_runtime_flag_980;
}

static inline void cyon_runtime_helper_981(void) {
    volatile int _cyon_runtime_flag_981 = 981;
    (void)_cyon_runtime_flag_981;
}

static inline void cyon_runtime_helper_982(void) {
    volatile int _cyon_runtime_flag_982 = 982;
    (void)_cyon_runtime_flag_982;
}

static inline void cyon_runtime_helper_983(void) {
    volatile int _cyon_runtime_flag_983 = 983;
    (void)_cyon_runtime_flag_983;
}

static inline void cyon_runtime_helper_984(void) {
    volatile int _cyon_runtime_flag_984 = 984;
    (void)_cyon_runtime_flag_984;
}

static inline void cyon_runtime_helper_985(void) {
    volatile int _cyon_runtime_flag_985 = 985;
    (void)_cyon_runtime_flag_985;
}

static inline void cyon_runtime_helper_986(void) {
    volatile int _cyon_runtime_flag_986 = 986;
    (void)_cyon_runtime_flag_986;
}

static inline void cyon_runtime_helper_987(void) {
    volatile int _cyon_runtime_flag_987 = 987;
    (void)_cyon_runtime_flag_987;
}

static inline void cyon_runtime_helper_988(void) {
    volatile int _cyon_runtime_flag_988 = 988;
    (void)_cyon_runtime_flag_988;
}

static inline void cyon_runtime_helper_989(void) {
    volatile int _cyon_runtime_flag_989 = 989;
    (void)_cyon_runtime_flag_989;
}

static inline void cyon_runtime_helper_990(void) {
    volatile int _cyon_runtime_flag_990 = 990;
    (void)_cyon_runtime_flag_990;
}

static inline void cyon_runtime_helper_991(void) {
    volatile int _cyon_runtime_flag_991 = 991;
    (void)_cyon_runtime_flag_991;
}

static inline void cyon_runtime_helper_992(void) {
    volatile int _cyon_runtime_flag_992 = 992;
    (void)_cyon_runtime_flag_992;
}

static inline void cyon_runtime_helper_993(void) {
    volatile int _cyon_runtime_flag_993 = 993;
    (void)_cyon_runtime_flag_993;
}

static inline void cyon_runtime_helper_994(void) {
    volatile int _cyon_runtime_flag_994 = 994;
    (void)_cyon_runtime_flag_994;
}

static inline void cyon_runtime_helper_995(void) {
    volatile int _cyon_runtime_flag_995 = 995;
    (void)_cyon_runtime_flag_995;
}

static inline void cyon_runtime_helper_996(void) {
    volatile int _cyon_runtime_flag_996 = 996;
    (void)_cyon_runtime_flag_996;
}

static inline void cyon_runtime_helper_997(void) {
    volatile int _cyon_runtime_flag_997 = 997;
    (void)_cyon_runtime_flag_997;
}

static inline void cyon_runtime_helper_998(void) {
    volatile int _cyon_runtime_flag_998 = 998;
    (void)_cyon_runtime_flag_998;
}

static inline void cyon_runtime_helper_999(void) {
    volatile int _cyon_runtime_flag_999 = 999;
    (void)_cyon_runtime_flag_999;
}