# Cyon Programming Language - Architecture & Structure

> **Comprehensive guide to the Cyon compiler internals, runtime system, and project organization**

---

## 📑 Table of Contents

1. [Overview](#overview)
2. [High-Level Architecture](#high-level-architecture)
3. [Directory Structure](#directory-structure)
4. [Compilation Pipeline](#compilation-pipeline)
5. [Core Components](#core-components)
6. [Runtime System](#runtime-system)
7. [Standard Library](#standard-library)
8. [Extended Libraries](#extended-libraries)
9. [Build System](#build-system)
10. [File Dependencies](#file-dependencies)
11. [Data Flow](#data-flow)
12. [Memory Management](#memory-management)

---

## 🎯 Overview

Cyon is a multi-stage compiler that transforms `.cyon` source code into native executables through an intermediate C representation. The project consists of approximately **50,000 lines** of code split between Python (toolchain) and C (runtime).

### Technology Stack

- **Frontend**: Python 3.8+ (Lexer, Parser, Optimizer, CodeGen)
- **Backend**: C99 (Runtime library, type system, memory management)
- **Build Tools**: GNU Make, GCC/Clang
- **Target Platforms**: Windows, Linux, macOS

### Code Distribution

```
Total: ~50,000 lines
├── Python (Compiler): ~20,000 lines (40%)
│   ├── cli.py: ~200 lines
│   ├── compiler.py: ~1,800 lines
│   ├── core/lexer.py: ~700 lines
│   ├── core/parser.py: ~1,000 lines
│   ├── core/optimizer.py: ~1,200 lines
│   ├── core/codegen.py: ~800 lines
│   ├── core/interpreter.py: ~700 lines
│   └── lib/*.py: ~3,000 lines
│
└── C (Runtime): ~30,000 lines (60%)
    ├── core/runtime/*.c: ~15,000 lines
    ├── core/runtime/*.h: ~5,000 lines
    ├── libraries/*.c: ~8,000 lines
    └── include/*.h: ~2,000 lines
```

---

## 🏛️ High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     CYON TOOLCHAIN                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐      ┌──────────────┐                    │
│  │   CLI Tool   │─────→│   Compiler   │                    │
│  │   (cli.py)   │      │(compiler.py) │                    │
│  └──────────────┘      └──────┬───────┘                    │
│                                │                            │
│         ┌──────────────────────┼──────────────────┐         │
│         ↓                      ↓                  ↓         │
│  ┌────────────┐        ┌────────────┐    ┌──────────────┐  │
│  │   Lexer    │───────→│   Parser   │───→│  Optimizer   │  │
│  │ (lexer.py) │ Tokens │ (parser.py)│AST │(optimizer.py)│  │
│  └────────────┘        └────────────┘    └──────┬───────┘  │
│                                                  │          │
│                                                  ↓          │
│                                         ┌──────────────┐    │
│                                         │  Code Gen    │    │
│                                         │(codegen.py)  │    │
│                                         └──────┬───────┘    │
│                                                │ C Code     │
└────────────────────────────────────────────────┼────────────┘
                                                 ↓
┌─────────────────────────────────────────────────────────────┐
│                   C COMPILER (GCC/Clang)                    │
└─────────────────────────────────────────────────────────────┘
                                                 ↓
┌─────────────────────────────────────────────────────────────┐
│                   NATIVE EXECUTABLE                         │
│                                                             │
│  ┌────────────────┐    ┌─────────────────────────────┐     │
│  │  User Code     │    │   Cyon Runtime System       │     │
│  │  (Generated C) │───→│   - Memory Management       │     │
│  │                │    │   - Type System             │     │
│  │                │    │   - I/O Operations          │     │
│  │                │    │   - Standard Library        │     │
│  └────────────────┘    └─────────────────────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

---

## 📂 Directory Structure

### Root Level

```
C:\Cyon\
│
├── cli.py              # Command-line interface wrapper
│                       # Routes commands to compiler.py
│                       # ~200 lines
│
├── compiler.py         # Main compiler orchestrator
│                       # Implements build/run/compile actions
│                       # Coordinates all compilation stages
│                       # ~1,800 lines
│
├── settings.py        # Configuration management
│                       # Compiler flags, paths, targets
│                       # ~200 lines
│
├── utils.py           # Shared utility functions
│                       # File I/O, path handling, helpers
│                       # ~180 lines
│
└── libcyon.a          # Compiled static runtime library
                        # Generated from core/runtime/
```

### Core Directory (`core/`)

```
core/
├── __init__.py        # Module initialization
│                      # Exports core components
│
├── lexer.py           # Lexical analyzer
│   │                  # Tokenizes source code
│   │                  # Handles keywords, operators, literals
│   │                  # ~700 lines
│   │
│   └─→ Token classes: KEYWORD, IDENTIFIER, NUMBER, STRING,
│                      OPERATOR, PUNCTUATION, COMMENT, EOF
│
├── parser.py          # Syntax parser
│   │                  # Builds Abstract Syntax Tree (AST)
│   │                  # Validates syntax rules
│   │                  # ~1,000 lines
│   │
│   └─→ AST Nodes: FunctionDef, VariableDecl, IfStatement,
│                  WhileLoop, ForLoop, BinaryOp, UnaryOp,
│                  CallExpr, ArrayLiteral, etc.
│
├── optimizer.py       # Code optimizer
│   │                  # Performs optimization passes on AST
│   │                  # Constant folding, dead code elimination
│   │                  # Loop unrolling, inline expansion
│   │                  # ~1,200 lines
│   │
│   └─→ Optimization passes:
│       • Constant propagation
│       • Dead code removal
│       • Common subexpression elimination
│       • Loop invariant code motion
│       • Tail call optimization
│
├── codegen.py         # C code generator
│   │                  # Translates optimized AST to C
│   │                  # Manages symbol tables
│   │                  # ~800 lines
│   │
│   └─→ Code generation for:
│       • Function definitions
│       • Variable declarations
│       • Control flow (if/while/for)
│       • Expressions and operators
│       • Array operations
│       • Function calls
│
├── interpreter.py     # Optional interpreter mode
│   │                  # Direct AST execution (no compilation)
│   │                  # Used for REPL and quick testing
│   │                  # ~700 lines
│   │
│   └─→ Runtime environment:
│       • Variable bindings
│       • Function call stack
│       • Expression evaluation
│       • Native function hooks
│
└── runtime/          # C runtime system (see below)
```

### Runtime Directory (`core/runtime/`)

```
runtime/
├── runtime.c          # Main runtime implementation
│   │                  # ~4,500 lines
│   │                  # Core runtime initialization
│   │                  # Value system implementation
│   │                  # Function registry and dispatch
│   │                  # Native function wrappers
│   │
│   └─→ Key functions:
│       • cyon_runtime_init()
│       • cyon_register_native()
│       • cyon_lookup_native()
│       • cyon_value_* (value constructors)
│       • cyon_array_* (array operations)
│
├── runtime.h          # Runtime public interface
│   │                  # ~5,800 lines
│   │                  # Function declarations
│   │                  # Type definitions
│   │                  # Macro definitions
│   │
│   └─→ Main sections:
│       • Value type system
│       • Memory management APIs
│       • Array/String APIs
│       • Native function types
│       • Error handling
│
├── coretypes.h        # Core type definitions
│   │                  # ~5,700 lines
│   │                  # Fundamental types (i8, i16, i32, i64, etc.)
│   │                  # Boolean, string, size types
│   │                  # Result and slice types
│   │                  # Dynamic array headers
│   │                  # String builder
│   │                  # Hash map implementation
│   │                  # Object header system
│   │
│   └─→ Type system:
│       • Exact-width integers (i8...i64, u8...u64)
│       • Floating point (f32, f64)
│       • cyon_bool, cyon_char, cyon_cstr
│       • cyon_result_t (error handling)
│       • cyon_slice_t (string slices)
│       • cyon_array_hdr_t (dynamic arrays)
│       • cyon_sb_t (string builder)
│       • cyon_map_t (hash map)
│       • cyon_obj_header_t (object metadata)
│
├── coreprint.c        # Printing and formatting
│   │                  # ~5,900 lines
│   │                  # All print functions
│   │                  # Format string handling
│   │                  # Debug output utilities
│   │
│   └─→ Functions:
│       • cyon_print_raw/println_raw
│       • cyon_print_int/int64/double/bool
│       • cyon_printf/printfln
│       • cyon_print_quoted/safe
│       • cyon_print_str_array
│       • cyon_hexdump
│       • cyon_print_hex/bin/oct
│       • cyon_log_info/warn/error
│
├── coremath.c         # Mathematical operations
│   │                  # ~4,300 lines
│   │                  # Basic arithmetic
│   │                  # Trigonometric functions
│   │                  # Statistical functions
│   │
│   └─→ Categories:
│       • Basic: abs, min, max, clamp
│       • Power: pow, sqrt, cbrt, exp, log
│       • Trig: sin, cos, tan, asin, acos, atan
│       • Hyperbolic: sinh, cosh, tanh
│       • Rounding: floor, ceil, round, trunc
│       • Random: rand, srand, rand_range
│
├── coremem.c          # Memory management
│   │                  # ~5,200 lines
│   │                  # Allocation/deallocation
│   │                  # Memory pools
│   │                  # Reference counting
│   │
│   └─→ Memory system:
│       • cyon_malloc/calloc/realloc/free
│       • cyon_mem_pool_* (pooled allocation)
│       • cyon_mem_copy/move/set
│       • cyon_mem_compare
│       • Alignment helpers
│       • Memory statistics tracking
│
├── coreloop.c         # Loop constructs
│   │                  # ~2,800 lines
│   │                  # For/while loop runtime support
│   │                  # Iterator protocols
│   │                  # Range generators
│   │
│   └─→ Loop support:
│       • cyon_range_* (range iterators)
│       • cyon_foreach_* (iteration helpers)
│       • Loop unrolling support
│       • Break/continue handling
│
├── coreinput.c        # Input handling
│   │                  # ~5,400 lines
│   │                  # Standard input reading
│   │                  # File input
│   │                  # Parsing utilities
│   │
│   └─→ Input functions:
│       • cyon_input_line (readline)
│       • cyon_input_int/float/bool
│       • cyon_parse_* (parsing helpers)
│       • File reading functions
│       • Buffer management
│
├── coreutils.c        # Utility functions
│   │                  # ~5,700 lines
│   │                  # String operations
│   │                  # Array utilities
│   │                  # System helpers
│   │
│   └─→ Utilities:
│       • String: concat, split, trim, compare
│       • Array: sort, search, filter, map
│       • Time: clock, timing functions
│       • System: environment, process control
│
├── MakeFile           # Build script for runtime
│                      # Compiles all .c files
│                      # Creates libcyon.a static library
│                      # ~30 lines
│
└── libcyon.a          # Compiled static library
                       # Linked with generated C code
```

### Library Directory (`lib/`)

Python-based standard library modules:

```
lib/
├── __init__.py        # Library module exports
│
├── cyonio.py          # I/O operations library
│   │                  # ~1,800 lines
│   │                  # File I/O wrappers
│   │                  # Stream handling
│   │                  # Formatting utilities
│   │
│   └─→ Modules:
│       • File: open, read, write, close
│       • Stream: stdin, stdout, stderr
│       • Format: sprintf, string formatting
│       • Path: join, split, exists, isdir
│
├── cyonmath.py        # Mathematical library
│   │                  # ~1,050 lines
│   │                  # Extended math functions
│   │                  # Matrix operations
│   │                  # Statistical functions
│   │
│   └─→ Functions:
│       • Advanced: factorial, gcd, lcm
│       • Matrix: create, multiply, transpose
│       • Stats: mean, median, stdev, variance
│       • Complex: complex numbers, operations
│
└── cyonsys.py         # System utilities
    │                  # ~530 lines
    │                  # OS interface
    │                  # Process management
    │                  # Environment access
    │
    └─→ System functions:
        • OS: platform, arch, version
        • Process: exec, spawn, kill
        • Env: getenv, setenv, unsetenv
        • Path: realpath, abspath, dirname
```

### Extended Libraries (`libraries/`)

C-based extension libraries:

```
libraries/
├── __init__.py        # Extension registry
│
├── coreai.c           # AI/ML utilities (~2,100 lines)
│   └─→ Neural networks, linear algebra, data processing
│
├── corecrypto.c       # Cryptography (~660 lines)
│   └─→ Hashing (MD5, SHA), encryption (AES, RSA)
│
├── corectypto.py      # Python crypto bridge (~220 lines)
│
├── coreenv.c          # Environment handling (~1,970 lines)
│   └─→ Environment variables, system configuration
│
├── corefile.c         # File operations (~1,710 lines)
│   └─→ Advanced file I/O, permissions, metadata
│
├── corefs.c           # Filesystem (~930 lines)
│   └─→ Directory operations, file search, glob
│
├── coregui.c          # GUI framework (~2,130 lines)
│   └─→ Window management, widgets, event loop
│
├── corejson.c         # JSON parsing (~630 lines)
│   └─→ JSON encode/decode, validation
│
├── corelog.c          # Logging system (~1,750 lines)
│   └─→ Log levels, file rotation, formatting
│
├── coremathx.c        # Extended math (~1,580 lines)
│   └─→ BigInt, arbitrary precision, special functions
│
├── corenet.c          # Networking (~2,470 lines)
│   └─→ Sockets, HTTP client/server, protocols
│
├── coreos.c           # OS interface (~990 lines)
│   └─→ Process control, signals, system calls
│
├── coreos.py          # Python OS bridge (~1,320 lines)
│
├── corethread.c       # Threading (~1,470 lines)
│   └─→ Thread creation, mutexes, condition variables
│
├── coretime.c         # Time utilities (~540 lines)
│   └─→ Date/time parsing, formatting, timers
│
└── MakeFile           # Library build script
```

### Include Directory (`include/`)

Public C header files for libraries:

```
include/
├── cyonstd.h          # Standard definitions (~110 lines)
│   └─→ Common macros, basic types
│
├── cyonlib.h          # Library interface (~440 lines)
│   └─→ Library initialization, module loading
│
├── cyonio.h           # I/O interface (~290 lines)
│   └─→ File/stream operations declarations
│
├── cyonmath.h         # Math interface (~320 lines)
│   └─→ Mathematical function declarations
│
├── cyonmem.h          # Memory interface (~160 lines)
│   └─→ Memory management function declarations
│
├── cyonfs.h           # Filesystem interface (~560 lines)
│   └─→ File system operation declarations
│
├── cyonnet.h          # Network interface (~310 lines)
│   └─→ Networking function declarations
│
└── cyoncrypto.h       # Crypto interface (~310 lines)
    └─→ Cryptographic function declarations
```

### Examples Directory (`examples/`)

Sample Cyon programs:

```
examples/
├── hello.cyon         # Basic Hello World (~19 lines)
├── calculator.cyon    # Simple calculator (~57 lines)
├── arrays.cyon        # Array operations (~75 lines)
├── loops.cyon         # Loop examples (~67 lines)
├── conditions.cyon    # Conditional logic (~57 lines)
├── recursion.cyon     # Recursive functions (~52 lines)
├── main.cyon          # Template main program (~18 lines)
└── test.cyon          # Comprehensive test (~965 lines)
```

### Tests Directory (`tests/`)

Unit and integration tests:

```
tests/
├── test_lexer.py      # Lexer unit tests (~58 lines)
├── test_parser.py     # Parser unit tests (~48 lines)
├── test_codegen.py    # Code generator tests (~58 lines)
├── test_examples.py   # Example compilation tests (~55 lines)
└── test_runtime.c     # Runtime C tests (~36 lines)
```

### Build Directory (`build/`)

Generated output directory (created during compilation):

```
build/
├── <source>.c         # Generated C code
├── <source>.o         # Object files
├── <source>.exe       # Executables (Windows)
├── <source>           # Executables (Unix)
└── *.tmp              # Temporary files
```

---

## ⚙️ Compilation Pipeline

### Stage 1: Lexical Analysis (Lexer)

**Input**: `.cyon` source code  
**Output**: Token stream  
**File**: `core/lexer.py`

```python
Source Code:
    func add(x: int, y: int) -> int { return x + y }

Tokens:
    [KEYWORD: func]
    [IDENTIFIER: add]
    [LPAREN: (]
    [IDENTIFIER: x]
    [COLON: :]
    [TYPE: int]
    ...
```

**Process**:
1. Read source file character by character
2. Identify keywords, identifiers, literals, operators
3. Handle whitespace and comments
4. Generate token objects with position info
5. Validate lexical structure

**Key Functions**:
- `tokenize()` - Main tokenization loop
- `scan_identifier()` - Recognize identifiers/keywords
- `scan_number()` - Parse numeric literals
- `scan_string()` - Parse string literals
- `scan_operator()` - Recognize operators

### Stage 2: Syntax Analysis (Parser)

**Input**: Token stream  
**Output**: Abstract Syntax Tree (AST)  
**File**: `core/parser.py`

```python
Tokens → AST:
    FunctionDef(
        name='add',
        params=[
            Param(name='x', type='int'),
            Param(name='y', type='int')
        ],
        return_type='int',
        body=Block([
            ReturnStmt(
                BinaryOp(op='+', left=Var('x'), right=Var('y'))
            )
        ])
    )
```

**Process**:
1. Consume tokens from lexer
2. Apply grammar rules (recursive descent)
3. Build AST nodes (top-down)
4. Validate syntax structure
5. Type checking (basic)
6. Symbol table construction

**Key Functions**:
- `parse_program()` - Entry point
- `parse_function()` - Function definitions
- `parse_statement()` - Statement parsing
- `parse_expression()` - Expression parsing
- `parse_type()` - Type annotations

### Stage 3: Optimization (Optimizer)

**Input**: AST  
**Output**: Optimized AST  
**File**: `core/optimizer.py`

```python
Before:
    x = 5
    y = 10
    z = x + y    # Can be computed at compile time

After:
    z = 15       # Constant folded
```

**Optimization Passes**:

1. **Constant Folding**
   - Evaluate constant expressions at compile time
   - `2 + 3` → `5`

2. **Dead Code Elimination**
   - Remove unreachable code
   - Remove unused variables

3. **Common Subexpression Elimination**
   - Reuse computed values
   - `a*b + a*b` → `temp = a*b; temp + temp`

4. **Loop Optimization**
   - Loop invariant code motion
   - Loop unrolling for small counts

5. **Tail Call Optimization**
   - Convert tail recursion to loops

6. **Inline Expansion**
   - Inline small functions

**Key Functions**:
- `optimize()` - Main optimization coordinator
- `fold_constants()` - Constant folding pass
- `eliminate_dead_code()` - DCE pass
- `optimize_loops()` - Loop optimization pass

### Stage 4: Code Generation (CodeGen)

**Input**: Optimized AST  
**Output**: C source code  
**File**: `core/codegen.py`

```python
AST → C Code:
    int64_t add(int64_t x, int64_t y) {
        return x + y;
    }
```

**Process**:
1. Traverse optimized AST
2. Generate equivalent C code
3. Manage variable names and scopes
4. Insert runtime calls where needed
5. Add header includes
6. Generate main() wrapper

**Code Generation Rules**:
- Cyon `int` → C `int64_t`
- Cyon `float` → C `double`
- Cyon `bool` → C `cyon_bool`
- Cyon `string` → C `cyon_string`
- Cyon `print()` → C `cyon_print()`
- Cyon arrays → C runtime array functions

**Key Functions**:
- `generate()` - Main generation entry
- `generate_function()` - Function code gen
- `generate_statement()` - Statement code gen
- `generate_expression()` - Expression code gen
- `mangle_name()` - Name mangling for symbols

### Stage 5: C Compilation (GCC/Clang)

**Input**: Generated C code + runtime library  
**Output**: Native executable  
**Tools**: GCC or Clang

```bash
# Compilation command (example)
gcc generated.c \
    -I./core/runtime \
    -I./include \
    -L./core/runtime \
    -lcyon \
    -lm \
    -o output.exe
```

**Process**:
1. Preprocess C code (expand macros, includes)
2. Compile to assembly
3. Assemble to object code
4. Link with runtime library
5. Generate executable

**Compiler Flags**:
- `-O2` - Optimization level 2 (default)
- `-Wall` - All warnings
- `-std=c99` - C99 standard
- `-I<path>` - Include directories
- `-L<path>` - Library directories
- `-l<lib>` - Link libraries

---

## 🔧 Core Components

### 1. CLI Tool (`cli.py`)

Command-line interface and dispatcher.

**Responsibilities**:
- Parse command-line arguments
- Route commands to compiler
- Handle user-friendly interface
- Forward additional flags

**Main Functions**:
```python
build_parser() -> ArgumentParser
    # Builds argument parser with subcommands

forward_to_compiler(argv) -> int
    # Delegates to compiler.main()

run(argv) -> int
    # Main entry point
```

**Commands**:
- `cyon run` - Compile and execute
- `cyon build` - Build executable
- `cyon compile` - Generate C only
- `cyon init` - Initialize project
- `cyon clean` - Clean artifacts
- `cyon info` - Show system info

### 2. Compiler (`compiler.py`)

Main compiler orchestrator.

**Responsibilities**:
- Coordinate compilation stages
- Manage temporary files
- Invoke C compiler
- Handle build targets
- Error reporting

**Main Functions**:
```python
compile_source(source_file, options) -> bool
    # Full compilation pipeline

build_executable(c_file, output, target) -> bool
    # Invoke C compiler and linker

run_executable(exe_path, args) -> int
    # Execute compiled program

clean_build_dir() -> None
    # Remove build artifacts

show_info() -> None
    # Display system information
```

**Compilation Flow**:
1. Read source file
2. Invoke lexer → tokens
3. Invoke parser → AST
4. Invoke optimizer → optimized AST
5. Invoke codegen → C code
6. Write C to build/ directory
7. Invoke GCC/Clang
8. Link with runtime library
9. Generate executable

### 3. Settings (`settings.py`)

Configuration management.

**Responsibilities**:
- Store compiler options
- Manage paths and directories
- Define build targets
- Platform-specific settings

**Configuration Options**:
```python
COMPILER = "gcc"           # C compiler to use
OPTIMIZATION = "-O2"       # Optimization level
STD = "-std=c99"           # C standard
INCLUDE_DIRS = [...]       # Include paths
LIBRARY_DIRS = [...]       # Library paths
RUNTIME_LIB = "cyon"       # Runtime library name
BUILD_DIR = "build/"       # Output directory
```

### 4. Utilities (`utils.py`)

Shared helper functions.

**Utility Categories**:
- File I/O operations
- Path manipulation
- String formatting
- Error reporting
- Platform detection

**Key Functions**:
```python
read_file(path) -> str
write_file(path, content) -> None
ensure_dir(path) -> None
get_platform() -> str
format_error(msg, line, col) -> str
```

---

## 🔄 Runtime System

### Value System

Cyon uses a tagged union for runtime values:

```c
typedef enum {
    CYON_V_NIL,
    CYON_V_INT,
    CYON_V_FLOAT,
    CYON_V_STRING,
    CYON_V_ARRAY,
    CYON_V_FUNC_NATIVE,
    CYON_V_FUNC_USER,
} cyon_val_t;

struct cyon_value {
    cyon_val_t type;
    union {
        int64_t i;
        double f;
        char *s;
        struct {
            cyon_value_ptr *items;
            size_t len;
        } arr;
        void *fn;
    } u;
};
```

### Type System (`coretypes.h`)

Provides fundamental types and data structures:

**Primitive Types**:
- `i8`, `i16`, `i32`, `i64` - Signed integers
- `u8`, `u16`, `u32`, `u64` - Unsigned integers
- `f32`, `f64` - Floating point
- `cyon_bool` - Boolean
- `cyon_char` - Character
- `cyon_cstr` - C string

**Compound Types**:
- `cyon_result_t` - Result/error type
- `cyon_slice_t` - String slice (ptr + len)
- `cyon_sb_t` - String builder
- `cyon_map_t` - Hash map
- `cyon_obj_header_t` - Object metadata

**Dynamic Arrays**:
```c
// Header stored before array data
typedef struct {
    size_t len;  // Current length
    size_t cap;  // Capacity
} cyon_array_hdr_t;

// Macro-based API
int *arr = cyon_array_new(int, 10);
cyon_array_push(arr, 42);
int val = arr[0];
cyon_array_free(arr);
```

### Memory Management (`coremem.c`)

**Allocation Functions**:
```c
void* cyon_malloc(size_t sz)
void* cyon_calloc(size_t nmemb, size_t size)
void* cyon_realloc(void *ptr, size_t new_size)
void cyon_free(void *ptr)
```

**Memory Pools**:
```c
cyon_mem_pool_t* cyon_mem_pool_create(size_t block_size)
void* cyon_mem_pool_alloc(cyon_mem_pool_t *pool)
void cyon_mem_pool_free(cyon_mem_pool_t *pool, void *ptr)
void cyon_mem_pool_destroy(cyon_mem_pool_t *pool)
```

**Reference Counting**:
```c
void cyon_obj_incref(void *data)
void cyon_obj_decref(void *data, void (*destroy_cb)(void*))
```

### Print System (`coreprint.c`)

Comprehensive output functions:

**Basic Printing**:
```c
void cyon_print_raw(const char *s)
void cyon_println_raw(const char *s)
void cyon_print_int(int v)
void cyon_print_int64(int64_t v)
void cyon_print_double(double v)
void cyon_print_bool(int b)
```

**Formatted Printing**:
```c
void cyon_printf(const char *fmt, ...)
void cyon_printfln(const char *fmt, ...)
void cyon_print_quoted(const char *s)
void cyon_print_safe(const char *s)
```

**Array/Debug Printing**:
```c
void cyon_print_str_array(const char **arr, size_t n, const char *sep)
void cyon_print_int_array(const int64_t *arr, size_t n)
void cyon_hexdump(const void *data, size_t len)
```

**Number Formatting**:
```c
void cyon_print_hex(int64_t v)   // Hexadecimal
void cyon_print_bin(int64_t v)   // Binary
void cyon_print_oct(int64_t v)   // Octal
```

**Logging**:
```c
void cyon_log_info(const char *fmt, ...)
void cyon_log_warn(const char *fmt, ...)
void cyon_log_error(const char *fmt, ...)
```

### Math System (`coremath.c`)

Mathematical operations and functions:

**Basic Operations**:
```c
int64_t cyon_abs(int64_t x)
int64_t cyon_min(int64_t a, int64_t b)
int64_t cyon_max(int64_t a, int64_t b)
int64_t cyon_clamp(int64_t x, int64_t lo, int64_t hi)
```

**Power Functions**:
```c
double cyon_pow(double base, double exp)
double cyon_sqrt(double x)
double cyon_cbrt(double x)
double cyon_exp(double x)
double cyon_log(double x)
double cyon_log10(double x)
```

**Trigonometric**:
```c
double cyon_sin(double x)
double cyon_cos(double x)
double cyon_tan(double x)
double cyon_asin(double x)
double cyon_acos(double x)
double cyon_atan(double x)
double cyon_atan2(double y, double x)
```

**Hyperbolic**:
```c
double cyon_sinh(double x)
double cyon_cosh(double x)
double cyon_tanh(double x)
```

**Rounding**:
```c
double cyon_floor(double x)
double cyon_ceil(double x)
double cyon_round(double x)
double cyon_trunc(double x)
```

**Random Numbers**:
```c
void cyon_srand(unsigned int seed)
int cyon_rand(void)
int cyon_rand_range(int min, int max)
double cyon_rand_float(void)
```

### Input System (`coreinput.c`)

User input and parsing:

**Input Functions**:
```c
cyon_string cyon_input_line(const char *prompt)
int cyon_input_int(const char *prompt, int *out)
double cyon_input_float(const char *prompt, double *out)
bool cyon_input_bool(const char *prompt, bool *out)
```

**Parsing Helpers**:
```c
long cyon_parse_int(const char *s, int *ok)
double cyon_parse_float(const char *s, int *ok)
bool cyon_parse_bool(const char *s, int *ok)
```

**File Input**:
```c
char* cyon_read_file(const char *path)
char** cyon_read_lines(const char *path, size_t *count)
```

### Loop System (`coreloop.c`)

Loop construct support:

**Range Iterators**:
```c
typedef struct {
    int64_t current;
    int64_t end;
    int64_t step;
} cyon_range_t;

cyon_range_t cyon_range(int64_t start, int64_t end, int64_t step)
bool cyon_range_next(cyon_range_t *r, int64_t *out)
```

**Iteration Helpers**:
```c
void cyon_foreach_array(void *arr, size_t len, size_t elem_size,
                        void (*fn)(void *elem, void *ctx), void *ctx)

void cyon_foreach_range(int64_t start, int64_t end,
                        void (*fn)(int64_t i, void *ctx), void *ctx)
```

### Utility System (`coreutils.c`)

General-purpose utilities:

**String Operations**:
```c
char* cyon_strdup_safe(const char *s)
char* cyon_strconcat(const char *a, const char *b)
char** cyon_strsplit(const char *s, const char *delim, size_t *count)
char* cyon_strtrim(const char *s)
int cyon_strcmp_safe(const char *a, const char *b)
bool cyon_str_startswith(const char *s, const char *prefix)
bool cyon_str_endswith(const char *s, const char *suffix)
```

**Array Utilities**:
```c
void cyon_array_sort(void *arr, size_t n, size_t size,
                     int (*cmp)(const void*, const void*))
void* cyon_array_search(void *arr, size_t n, size_t size,
                        void *key, int (*cmp)(const void*, const void*))
void cyon_array_reverse(void *arr, size_t n, size_t size)
```

**Time Functions**:
```c
double cyon_clock(void)
void cyon_sleep(double seconds)
int64_t cyon_time_now(void)
```

---

## 📚 Standard Library

### I/O Library (`lib/cyonio.py`)

Python-based I/O operations:

**File Operations**:
```python
class File:
    def __init__(self, path, mode='r')
    def read(self, size=-1) -> str
    def write(self, data: str) -> int
    def readline(self) -> str
    def readlines(self) -> list
    def close(self)
    def __enter__() / __exit__()  # Context manager

def open(path, mode='r') -> File
def read_file(path) -> str
def write_file(path, content) -> None
```

**Stream Operations**:
```python
class Stream:
    stdin: Stream
    stdout: Stream
    stderr: Stream
    
    def write(self, data: str)
    def read(self, size: int) -> str
    def flush(self)
```

**Path Operations**:
```python
def path_join(*parts) -> str
def path_split(path) -> tuple
def path_exists(path) -> bool
def path_isfile(path) -> bool
def path_isdir(path) -> bool
def path_dirname(path) -> str
def path_basename(path) -> str
def path_abspath(path) -> str
```

**Formatting**:
```python
def format_string(fmt: str, *args) -> str
def sprintf(fmt: str, *args) -> str
```

### Math Library (`lib/cyonmath.py`)

Extended mathematical functions:

**Constants**:
```python
PI = 3.141592653589793
E = 2.718281828459045
TAU = 6.283185307179586
INFINITY = float('inf')
NAN = float('nan')
```

**Advanced Functions**:
```python
def factorial(n: int) -> int
def gcd(a: int, b: int) -> int
def lcm(a: int, b: int) -> int
def is_prime(n: int) -> bool
def fibonacci(n: int) -> int
```

**Matrix Operations**:
```python
class Matrix:
    def __init__(self, rows, cols)
    def get(self, row, col) -> float
    def set(self, row, col, value)
    def multiply(self, other) -> Matrix
    def transpose(self) -> Matrix
    def determinant(self) -> float
    def inverse(self) -> Matrix
```

**Statistics**:
```python
def mean(values: list) -> float
def median(values: list) -> float
def mode(values: list) -> Any
def variance(values: list) -> float
def stdev(values: list) -> float
def correlation(x: list, y: list) -> float
```

**Complex Numbers**:
```python
class Complex:
    def __init__(self, real, imag)
    def abs(self) -> float
    def arg(self) -> float
    def conjugate(self) -> Complex
    def __add__/__sub__/__mul__/__truediv__()
```

### System Library (`lib/cyonsys.py`)

System interface utilities:

**OS Information**:
```python
def platform() -> str
def architecture() -> str
def version() -> str
def hostname() -> str
```

**Process Management**:
```python
def execute(command: str, args: list) -> int
def spawn(command: str, args: list) -> int
def kill(pid: int, signal: int)
def getpid() -> int
def getppid() -> int
```

**Environment**:
```python
def getenv(name: str, default=None) -> str
def setenv(name: str, value: str)
def unsetenv(name: str)
def environ() -> dict
```

**Path Utilities**:
```python
def realpath(path: str) -> str
def abspath(path: str) -> str
def dirname(path: str) -> str
def basename(path: str) -> str
def splitext(path: str) -> tuple
```

---

## 🔌 Extended Libraries

### AI Library (`libraries/coreai.c`)

Machine learning and data processing:

**Neural Networks**:
```c
typedef struct cyon_neural_net cyon_neural_net_t;

cyon_neural_net_t* cyon_nn_create(int *layers, int num_layers)
void cyon_nn_train(cyon_neural_net_t *nn, double **inputs, 
                   double **outputs, int num_samples, int epochs)
double* cyon_nn_predict(cyon_neural_net_t *nn, double *input)
void cyon_nn_destroy(cyon_neural_net_t *nn)
```

**Linear Algebra**:
```c
void cyon_vector_add(double *a, double *b, double *result, int n)
void cyon_vector_dot(double *a, double *b, double *result, int n)
void cyon_matrix_multiply(double **a, double **b, double **result, 
                          int m, int n, int p)
```

### Cryptography (`libraries/corecrypto.c`)

Security and hashing:

**Hashing**:
```c
void cyon_md5(const char *data, size_t len, char *out)
void cyon_sha1(const char *data, size_t len, char *out)
void cyon_sha256(const char *data, size_t len, char *out)
```

**Encryption**:
```c
void cyon_aes_encrypt(const char *data, const char *key, char *out)
void cyon_aes_decrypt(const char *data, const char *key, char *out)
void cyon_rsa_generate_keypair(char **public_key, char **private_key)
```

### Filesystem (`libraries/corefs.c`)

Advanced file operations:

```c
bool cyon_fs_exists(const char *path)
bool cyon_fs_isfile(const char *path)
bool cyon_fs_isdir(const char *path)
int cyon_fs_mkdir(const char *path)
int cyon_fs_rmdir(const char *path)
int cyon_fs_remove(const char *path)
char** cyon_fs_listdir(const char *path, size_t *count)
```

### Networking (`libraries/corenet.c`)

Network communications:

**Sockets**:
```c
typedef struct cyon_socket cyon_socket_t;

cyon_socket_t* cyon_socket_create(int domain, int type)
int cyon_socket_connect(cyon_socket_t *sock, const char *host, int port)
int cyon_socket_bind(cyon_socket_t *sock, const char *host, int port)
int cyon_socket_listen(cyon_socket_t *sock, int backlog)
cyon_socket_t* cyon_socket_accept(cyon_socket_t *sock)
int cyon_socket_send(cyon_socket_t *sock, const char *data, size_t len)
int cyon_socket_recv(cyon_socket_t *sock, char *buf, size_t len)
void cyon_socket_close(cyon_socket_t *sock)
```

**HTTP Client**:
```c
typedef struct cyon_http_response cyon_http_response_t;

cyon_http_response_t* cyon_http_get(const char *url)
cyon_http_response_t* cyon_http_post(const char *url, const char *data)
int cyon_http_status(cyon_http_response_t *resp)
char* cyon_http_body(cyon_http_response_t *resp)
void cyon_http_free(cyon_http_response_t *resp)
```

### Threading (`libraries/corethread.c`)

Concurrent programming:

**Thread Management**:
```c
typedef struct cyon_thread cyon_thread_t;

cyon_thread_t* cyon_thread_create(void* (*fn)(void*), void *arg)
int cyon_thread_join(cyon_thread_t *thread, void **retval)
void cyon_thread_detach(cyon_thread_t *thread)
void cyon_thread_exit(void *retval)
```

**Synchronization**:
```c
typedef struct cyon_mutex cyon_mutex_t;
typedef struct cyon_cond cyon_cond_t;

cyon_mutex_t* cyon_mutex_create(void)
void cyon_mutex_lock(cyon_mutex_t *mutex)
void cyon_mutex_unlock(cyon_mutex_t *mutex)
void cyon_mutex_destroy(cyon_mutex_t *mutex)

cyon_cond_t* cyon_cond_create(void)
void cyon_cond_wait(cyon_cond_t *cond, cyon_mutex_t *mutex)
void cyon_cond_signal(cyon_cond_t *cond)
void cyon_cond_broadcast(cyon_cond_t *cond)
void cyon_cond_destroy(cyon_cond_t *cond)
```

### JSON (`libraries/corejson.c`)

JSON parsing and generation:

```c
typedef struct cyon_json_value cyon_json_value_t;

cyon_json_value_t* cyon_json_parse(const char *json_str)
char* cyon_json_stringify(cyon_json_value_t *value)
cyon_json_value_t* cyon_json_get(cyon_json_value_t *obj, const char *key)
void cyon_json_free(cyon_json_value_t *value)
```

### Logging (`libraries/corelog.c`)

Structured logging system:

```c
typedef enum {
    CYON_LOG_DEBUG,
    CYON_LOG_INFO,
    CYON_LOG_WARN,
    CYON_LOG_ERROR,
    CYON_LOG_FATAL
} cyon_log_level_t;

void cyon_log_init(const char *filename, cyon_log_level_t level)
void cyon_log_debug(const char *fmt, ...)
void cyon_log_info(const char *fmt, ...)
void cyon_log_warn(const char *fmt, ...)
void cyon_log_error(const char *fmt, ...)
void cyon_log_fatal(const char *fmt, ...)
void cyon_log_shutdown(void)
```

### GUI (`libraries/coregui.c`)

Simple GUI framework:

```c
typedef struct cyon_window cyon_window_t;
typedef struct cyon_widget cyon_widget_t;

cyon_window_t* cyon_window_create(const char *title, int w, int h)
void cyon_window_show(cyon_window_t *win)
void cyon_window_hide(cyon_window_t *win)
void cyon_window_close(cyon_window_t *win)

cyon_widget_t* cyon_button_create(const char *label, void (*callback)(void))
cyon_widget_t* cyon_label_create(const char *text)
cyon_widget_t* cyon_textbox_create(const char *placeholder)

void cyon_widget_add(cyon_window_t *win, cyon_widget_t *widget)
void cyon_event_loop_run(void)
```

---

## 🏗️ Build System

### Runtime Build (`core/runtime/MakeFile`)

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -fPIC
AR = ar
ARFLAGS = rcs

SOURCES = runtime.c coreprint.c coremath.c coremem.c \
          coreloop.c coreinput.c coreutils.c

OBJECTS = $(SOURCES:.c=.o)
TARGET = libcyon.a

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
```

### Library Build (`libraries/MakeFile`)

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -fPIC -I../include
AR = ar
ARFLAGS = rcs

SOURCES = coreai.c corecrypto.c coreenv.c corefile.c \
          corefs.c coregui.c corejson.c corelog.c \
          coremathx.c corenet.c coreos.c corethread.c coretime.c

OBJECTS = $(SOURCES:.c=.o)
TARGET = libcyonext.a

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	cp $(TARGET) ../

.PHONY: all clean install
```

### Compilation Commands

**Build Cyon Program**:
```bash
# Step 1: Compile Cyon to C
python compiler.py build source.cyon

# Step 2: Compile C to executable (done automatically)
gcc build/source.c \
    -I./core/runtime \
    -I./include \
    -L./core/runtime \
    -L./libraries \
    -lcyon \
    -lcyonext \
    -lm \
    -lpthread \
    -o build/source
```

**Cross-compilation**:
```bash
# Windows from Linux
x86_64-w64-mingw32-gcc build/source.c \
    -I./core/runtime -I./include \
    -L./core/runtime -L./libraries \
    -lcyon -lcyonext -lm \
    -o build/source.exe

# Linux from Windows (WSL)
gcc build/source.c \
    -I./core/runtime -I./include \
    -L./core/runtime -L./libraries \
    -lcyon -lcyonext -lm -lpthread \
    -o build/source.elf
```

---

## 🔗 File Dependencies

### Dependency Graph

```
cli.py
  └─→ compiler.py
       ├─→ settings.py
       ├─→ utils.py
       └─→ core/
            ├─→ lexer.py
            ├─→ parser.py
            │    └─→ lexer.py
            ├─→ optimizer.py
            │    └─→ parser.py
            ├─→ codegen.py
            │    └─→ optimizer.py
            └─→ interpreter.py
                 └─→ parser.py

Generated C code
  ├─→ core/runtime/runtime.h
  ├─→ core/runtime/coretypes.h
  ├─→ include/cyonstd.h
  └─→ [linked with libcyon.a]

libcyon.a (runtime library)
  ├─→ runtime.o
  ├─→ coreprint.o
  ├─→ coremath.o
  ├─→ coremem.o
  ├─→ coreloop.o
  ├─→ coreinput.o
  └─→ coreutils.o

libcyonext.a (extension library)
  ├─→ coreai.o
  ├─→ corecrypto.o
  ├─→ corenet.o
  ├─→ corethread.o
  └─→ [other extensions]
```

### Include Hierarchy

```
Generated C includes:
  #include <stdio.h>
  #include <stdlib.h>
  #include <stdint.h>
  #include "runtime/runtime.h"
    └─→ #include "runtime/coretypes.h"
         ├─→ <stddef.h>
         ├─→ <stdint.h>
         ├─→ <stdbool.h>
         └─→ <inttypes.h>
```

---

## 📊 Data Flow

### Compilation Data Flow

```
Source File (.cyon)
    │
    ├─→ [Lexer] Read & Tokenize
    │       │
    │       ├─→ Keywords: func, if, while, for, return, ...
    │       ├─→ Identifiers: variable names, function names
    │       ├─→ Literals: numbers, strings, booleans
    │       ├─→ Operators: +, -, *, /, ==, !=, ...
    │       └─→ Punctuation: (, ), {, }, [, ], ...
    │
    ↓ Token Stream
    │
    ├─→ [Parser] Build AST
    │       │
    │       ├─→ Program (root)
    │       ├─→ Functions
    │       ├─→ Statements
    │       ├─→ Expressions
    │       └─→ Type information
    │
    ↓ Abstract Syntax Tree
    │
    ├─→ [Optimizer] Transform AST
    │       │
    │       ├─→ Constant folding
    │       ├─→ Dead code elimination
    │       ├─→ Loop optimization
    │       └─→ Inlining
    │
    ↓ Optimized AST
    │
    ├─→ [CodeGen] Generate C
    │       │
    │       ├─→ Function definitions
    │       ├─→ Variable declarations
    │       ├─→ Statement translation
    │       ├─→ Expression conversion
    │       └─→ Runtime function calls
    │
    ↓ C Source Code
    │
    ├─→ [GCC/Clang] Compile
    │       │
    │       ├─→ Preprocessing
    │       ├─→ Compilation to assembly
    │       ├─→ Assembly to object code
    │       └─→ Linking with libcyon.a
    │
    ↓ Native Executable
```

### Runtime Data Flow

```
Program Start
    │
    ├─→ cyon_runtime_init()
    │       │
    │       ├─→ Initialize memory system
    │       ├─→ Register native functions
    │       └─→ Setup global state
    │
    ↓
    │
    ├─→ main() [Generated]
    │       │
    │       ├─→ Variable initialization
    │       ├─→ Function calls
    │       │       │
    │       │       ├─→ Native functions
    │       │       │       └─→ cyon_print(), cyon_input(), etc.
    │       │       │
    │       │       └─→ User functions
    │       │               └─→ Generated C functions
    │       │
    │       └─→ Memory operations
    │               │
    │               ├─→ cyon_malloc/free
    │               ├─→ cyon_array_* operations
    │               └─→ cyon_obj_* ref counting
    │
    ↓
    │
    └─→ Program Exit
            │
            └─→ Cleanup (automatic via OS)
```

---

## 💾 Memory Management

### Memory Layout

```
Cyon Object Layout:
┌────────────────────────────────┐
│   cyon_obj_header_t            │
│   ├─→ tag (32-bit)             │
│   ├─→ flags (32-bit)           │
│   └─→ refcount (64-bit)        │
├────────────────────────────────┤
│   Payload Data                 │
│   (user-defined structure)     │
└────────────────────────────────┘

Dynamic Array Layout:
┌────────────────────────────────┐
│   cyon_array_hdr_t             │
│   ├─→ len (size_t)             │
│   └─→ cap (size_t)             │
├────────────────────────────────┤
│   Element[0]                   │
│   Element[1]                   │
│   ...                          │
│   Element[len-1]               │
│   [unused capacity]            │
└────────────────────────────────┘

String Builder Layout:
┌────────────────────────────────┐
│   cyon_sb_t                    │
│   ├─→ buf (char*)              │──→ ┌──────────────┐
│   ├─→ len (size_t)             │    │ String data  │
│   └─→ cap (size_t)             │    │ ...          │
└────────────────────────────────┘    └──────────────┘
```

### Memory Operations

**Allocation Strategy**:
1. Small objects (< 256 bytes): Memory pool
2. Medium objects (256B - 64KB): Direct malloc
3. Large objects (> 64KB): Direct malloc with tracking

**Reference Counting**:
```c
// Object creation (refcount = 1)
void *obj = cyon_obj_alloc(sizeof(MyStruct), TAG, FLAGS, 1);

// Increment reference
cyon_obj_incref(obj);  // refcount++

// Decrement reference
cyon_obj_decref(obj, my_destroy_fn);  // refcount--
// If refcount reaches 0, my_destroy_fn(obj) is called
// and memory is freed
```

**Memory Pools**:
```c
// Create pool for 64-byte blocks
cyon_mem_pool_t *pool = cyon_mem_pool_create(64);

// Allocate from pool (O(1))
void *ptr = cyon_mem_pool_alloc(pool);

// Free to pool (O(1))
cyon_mem_pool_free(pool, ptr);

// Destroy pool
cyon_mem_pool_destroy(pool);
```

### Garbage Collection Strategy

Currently: **Manual + Reference Counting**

Future plans (v2.0):
- Optional tracing garbage collector
- Generational GC for long-lived objects
- Incremental collection to reduce pauses

---

## 🎯 Summary

### Project Statistics

| Component | Files | Lines | Language |
|-----------|-------|-------|----------|
| **Compiler** | 8 | ~20,000 | Python |
| **Runtime** | 11 | ~30,000 | C |
| **Standard Library** | 3 | ~3,000 | Python |
| **Extensions** | 17 | ~10,000 | C |
| **Headers** | 9 | ~7,000 | C |
| **Tests** | 5 | ~200 | Python/C |
| **Total** | **53** | **~50,000** | Mixed |

### Key Features Implemented

✅ Lexical analysis with comprehensive token support  
✅ Recursive descent parser with AST generation  
✅ Multi-pass optimizer with common optimizations  
✅ C code generator with runtime integration  
✅ Reference-counted memory management  
✅ Dynamic arrays and hash maps  
✅ Comprehensive I/O system  
✅ Mathematical operations library  
✅ Cross-platform build support  
✅ Extended libraries (networking, threading, crypto, etc.)  
✅ CLI tool with multiple commands  
✅ Interactive IDE

### Architecture Strengths

1. **Modularity**: Clean separation of concerns
2. **Extensibility**: Easy to add new libraries
3. **Performance**: Native code generation via C
4. **Portability**: Cross-platform support
5. **Safety**: Runtime checks and memory management
6. **Developer Experience**: Rich tooling and examples

---

**Document Version**: 1.0  
**Last Updated**: November 2025  
**Cyon Version**: 1.0  
**Total Project Size**: ~50,000 lines of code