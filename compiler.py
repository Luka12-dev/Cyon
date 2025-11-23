from __future__ import annotations

import argparse
import json
import os
import subprocess
import shutil
import sys
import textwrap
import re
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple, Callable

# Project layout (adjustable)

ROOT = Path(__file__).resolve().parent
CORE = ROOT / "core"
RUNTIME = CORE / "runtime"
INCLUDE = ROOT / "include"
LIB = RUNTIME / "libcyon.a"
BUILD = ROOT / "build"
EXAMPLES = ROOT / "examples"
LIBRARIES = ROOT / "libraries"

# Default config
DEFAULT_TARGET = "native"  # native | windows | linux | macos
CYON_NAME = "Cyon"
VERSION = "0.1.0"

# Utilities

def cyon_input():
    return input()

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def run_cmd(cmd: List[str], cwd: Optional[Path] = None, capture: bool = False, check: bool = True) -> subprocess.CompletedProcess:
    try:
        cp = subprocess.run([str(x) for x in cmd], cwd=str(cwd) if cwd else None, capture_output=capture, text=True, check=check)
        return cp
    except subprocess.CalledProcessError as ex:
        eprint(f"[cyon] Command failed: {' '.join(cmd)}")
        if ex.stdout:
            eprint("--- stdout ---")
            eprint(ex.stdout)
        if ex.stderr:
            eprint("--- stderr ---")
            eprint(ex.stderr)
        raise

def ensure_dir(p: Path) -> None:
    p.mkdir(parents=True, exist_ok=True)

def readable(path: Path) -> bool:
    return path.exists() and path.is_file()

def find_runtime_lib() -> Optional[Path]:
    # look for common runtime libs (static or shared). Prefer static.
    candidates = [
        RUNTIME / "libcyon.a",
        RUNTIME / "libcyon.so",
        RUNTIME / "libcyon.dylib",
        RUNTIME / "cyon.dll",
    ]
    for c in candidates:
        if c.exists():
            return c
    return None

def find_stdlib_archive() -> Optional[Path]:
    # libraries/libcyon_std.a is produced by libraries/Makefile
    candidate = LIBRARIES / "libcyon_std.a"
    if candidate.exists():
        return candidate
    # try to build it if Makefile present
    mf = LIBRARIES / "Makefile"
    if mf.exists():
        try:
            run_cmd(["make"], cwd=LIBRARIES)
        except Exception:
            return None
        if candidate.exists():
            return candidate
    return None

def is_root_user() -> bool:
    if os.name == "nt":
        try:
            import ctypes

            return ctypes.windll.shell32.IsUserAnAdmin() != 0
        except Exception:
            return False
    else:
        try:
            return os.geteuid() == 0
        except Exception:
            return False

# Import parsing and library resolution

_IMPORT_RE = re.compile(r'^\s*import\s+([A-Za-z_][A-Za-z0-9_\.]*)\s*;?\s*$')

def parse_imports_from_source(source_text: str) -> List[str]:
    """Scan source text for top-level import statements and return module names."""
    imports: List[str] = []
    for line in source_text.splitlines():
        m = _IMPORT_RE.match(line)
        if not m:
            continue
        raw = m.group(1)
        # accept forms: os, coreos, coreos.py, coreos.c -> normalize base name
        name = raw.split('.')[0]
        if name and name not in imports:
            imports.append(name)
    return imports

def resolve_imports_to_c_sources(names: List[str]) -> List[Path]:
    """For each import name, if libraries/<name>.c exists, return its Path."""
    out: List[Path] = []
    for n in names:
        cpath = LIBRARIES / f"{n}.c"
        if cpath.exists():
            out.append(cpath)
    return out

def resolve_imports_to_py_modules(names: List[str]) -> List[Path]:
    """For each import name, if libraries/<name>.py exists, return its Path."""
    out: List[Path] = []
    for n in names:
        pypath = LIBRARIES / f"{n}.py"
        if pypath.exists():
            out.append(pypath)
    return out

# Minimal lexer / parser demos

def lex(source: str) -> List[Tuple[str, str]]:
    tokens: List[Tuple[str, str]] = []
    i = 0
    n = len(source)
    while i < n:
        ch = source[i]
        if ch.isspace():
            i += 1
            continue
        if ch == '"' or ch == "'":
            # string literal, store raw content (we will json.dumps when generating C)
            quote = ch
            i += 1
            start = i
            while i < n and source[i] != quote:
                # naive: don't handle escapes
                i += 1
            val = source[start:i]
            tokens.append(("STRING", val))
            i += 1
            continue
        if ch.isalpha() or ch == "_":
            start = i
            i += 1
            while i < n and (source[i].isalnum() or source[i] == "_"):
                i += 1
            tokens.append(("IDENT", source[start:i]))
            continue
        if ch.isdigit():
            start = i
            i += 1
            dot_seen = False
            while i < n and (source[i].isdigit() or (source[i] == '.' and not dot_seen)):
                if source[i] == '.':
                    dot_seen = True
                i += 1
            tokens.append(("NUMBER", source[start:i]))
            continue
        # multi-char operators simple handling
        if i + 1 < n:
            two = source[i : i + 2]
            if two in ("<=", ">=", "==", "!=", ".."):
                tokens.append((two, two))
                i += 2
                continue
        tokens.append((ch, ch))
        i += 1
    return tokens


def parse(tokens: List[Tuple[str, str]]) -> Dict[str, Any]:
    ast: Dict[str, Any] = {"functions": []}
    i = 0
    n = len(tokens)
    while i < n:
        tok, val = tokens[i]
        if tok == "IDENT" and val == "func":
            # parse name
            i += 1
            if i < n and tokens[i][0] == "IDENT":
                name = tokens[i][1]
                # skip until '{'
                while i < n and tokens[i][1] != "{":
                    i += 1
                # collect until matching '}'
                i += 1
                body: List[Tuple[str, str]] = []
                depth = 1
                while i < n and depth > 0:
                    t, v = tokens[i]
                    if v == "{":
                        depth += 1
                    elif v == "}":
                        depth -= 1
                        if depth == 0:
                            i += 1
                            break
                    if depth > 0:
                        body.append((t, v))
                    i += 1
                ast["functions"].append({"name": name, "body": body})
                continue
        i += 1
    return ast

# Optimizer: small constant folding

def optimize(ast: Dict[str, Any]) -> Dict[str, Any]:
    for fn in ast.get("functions", []):
        body = fn.get("body", [])
        folded: List[Tuple[str, str]] = []
        i = 0
        while i < len(body):
            # pattern: (NUMBER,a) (op,+) (NUMBER,b)
            if (
                i + 2 < len(body)
                and body[i][0] == "NUMBER"
                and body[i + 1][1] in "+-*/"
                and body[i + 2][0] == "NUMBER"
            ):
                a = float(body[i][1]) if '.' in body[i][1] else int(body[i][1])
                op = body[i + 1][1]
                b = float(body[i + 2][1]) if '.' in body[i + 2][1] else int(body[i + 2][1])
                try:
                    if op == "+":
                        res = a + b
                    elif op == "-":
                        res = a - b
                    elif op == "*":
                        res = a * b
                    elif op == "/":
                        res = a / b if b != 0 else 0
                    else:
                        res = None
                    if res is not None:
                        if isinstance(res, float) and not res.is_integer():
                            folded.append(("NUMBER", str(res)))
                        else:
                            folded.append(("NUMBER", str(int(res))))
                        i += 3
                        continue
                except Exception:
                    pass
            folded.append(body[i])
            i += 1
        fn["body"] = folded
    return ast

# ===== Helpers to improve generated C (auto-declare, constant-fold if) =====

# mapping of known module.function -> inferred return type
_KNOWN_RETURNS = {
    "os.getcwd": "string",
    "fs.read": "string",
    "env.get": "string",
    "time.now": "string",
    "json.encode": "string",
    "json.decode": "string",
    "mathx.sqrt_approx": "double",
    "mathx.sqrt": "double",
    "ai.sigmoid": "double",
    "crypto.md5": "string",
    "crypto.sha256": "string",
    "net.connect": "int",
    "thread.spawn": "int",
    "util.random": "int",
    "file.read": "string",
    "file.write": "int",
    "fs.write": "int",
}

def _tokens_to_flat_str(tokens: List[Tuple[str, str]]) -> str:
    return " ".join(v for t, v in tokens)

def _infer_type_from_tokens(tokens: List[Tuple[str, str]]) -> str:
    """Infer simple type from RHS tokens: 'int', 'double', or 'string'."""
    # If any string literal present -> string
    for tk, val in tokens:
        if tk == "STRING":
            return "string"
    # detect known function calls like module.func(...) -> map via _KNOWN_RETURNS
    # look for pattern IDENT . IDENT
    for i in range(len(tokens) - 2):
        if tokens[i][0] == "IDENT" and tokens[i+1][1] == "." and tokens[i+2][0] == "IDENT":
            key = f"{tokens[i][1]}.{tokens[i+2][1]}"
            if key in _KNOWN_RETURNS:
                return _KNOWN_RETURNS[key]
    # numbers present -> int or double
    for tk, val in tokens:
        if tk == "NUMBER":
            if "." in val:
                return "double"
            return "int"
    # fallback string
    return "string"

def _collect_implicit_vars(body: List[Tuple[str, str]]) -> Dict[str, str]:
    """
    Scan function body tokens and return map of variable name -> inferred type
    for assignments that lack explicit declarations (heuristic).
    """
    implicit: Dict[str, str] = {}
    explicit: set = set()
    # collect explicit declared names first (patterns: int NAME ... or string NAME ...)
    i = 0
    while i < len(body):
        tk, val = body[i]
        if tk == "IDENT" and val in ("int", "double", "string", "str"):
            if i + 1 < len(body) and body[i+1][0] == "IDENT":
                explicit.add(body[i+1][1])
        i += 1

    i = 0
    while i < len(body):
        tk, val = body[i]
        # pattern: NAME = ...
        if tk == "IDENT" and i + 1 < len(body) and body[i+1][1] == "=":
            name = val
            if name in explicit or name in implicit:
                i += 1
                continue
            # collect rhs tokens until semicolon or end
            j = i + 2
            rhs: List[Tuple[str, str]] = []
            while j < len(body) and body[j][1] != ";":
                rhs.append(body[j])
                j += 1
            typ = _infer_type_from_tokens(rhs)
            implicit[name] = typ
            i = j
        else:
            i += 1
    return implicit

def _is_simple_constant_condition(tokens: List[Tuple[str, str]]) -> Optional[bool]:
    """
    If condition tokens are a simple numeric comparison (numbers and operators),
    evaluate and return True/False. Otherwise return None.
    Accept operators: >, <, >=, <=, ==, !=
    """
    allowed_ops = {" >", "<", ">=", "<=", "==", "!="}
    # build text but ensure only numbers and operators and parentheses are present
    expr_parts: List[str] = []
    for tk, val in tokens:
        if tk == "NUMBER":
            expr_parts.append(val)
        elif tk == "IDENT":
            # cannot evaluate if identifier present
            return None
        elif val in (">", "<", "==", "!=", ">=", "<=", "(", ")", "+", "-", "*", "/"):
            expr_parts.append(val)
        else:
            return None
    expr = " ".join(expr_parts)
    # additional safety: only digits, whitespace and operators allowed
    if re.fullmatch(r'[\d\.\s\(\)\+\-\*\/\<\>\=\!]+', expr) is None:
        return None
    try:
        val = eval(expr, {"__builtins__": {}})
        return bool(val)
    except Exception:
        return None

def _extract_block_tokens(body: List[Tuple[str, str]], start_idx: int) -> Tuple[List[Tuple[str, str]], int]:
    """
    Given body and index pointing at '{', extract inner tokens and return (inner_tokens, index_after_block).
    """
    assert body[start_idx][1] == "{"
    inner: List[Tuple[str, str]] = []
    depth = 0
    i = start_idx
    # move into block
    while i < len(body):
        tk, v = body[i]
        if v == "{":
            depth += 1
            if depth > 1:
                inner.append((tk, v))
            i += 1
            continue
        if v == "}":
            depth -= 1
            if depth == 0:
                i += 1
                break
            inner.append((tk, v))
            i += 1
            continue
        inner.append((tk, v))
        i += 1
    return inner, i

def _simplify_if_else(body: List[Tuple[str, str]]) -> List[Tuple[str, str]]:
    """
    Walk tokens and when encountering an if(...) { ... } [else { ... }] pattern with
    a constant-evaluable condition, replace the whole if/else with the chosen branch tokens.
    Otherwise leave tokens unchanged (we don't translate runtime if/else here).
    """
    out: List[Tuple[str, str]] = []
    i = 0
    n = len(body)
    while i < n:
        tk, v = body[i]
        if tk == "IDENT" and v == "if":
            # expect '('
            j = i + 1
            if j < n and body[j][1] == "(":
                # collect condition tokens between ( and )
                j += 1
                cond_tokens: List[Tuple[str, str]] = []
                paren_depth = 1
                while j < n and paren_depth > 0:
                    if body[j][1] == "(":
                        paren_depth += 1
                        cond_tokens.append(body[j])
                    elif body[j][1] == ")":
                        paren_depth -= 1
                        if paren_depth == 0:
                            j += 1
                            break
                        cond_tokens.append(body[j])
                    else:
                        cond_tokens.append(body[j])
                    j += 1
                # now j should be at token after ')'
                # find then block
                if j < n and body[j][1] == "{":
                    then_block, after_then = _extract_block_tokens(body, j)
                    k = after_then
                    # check for else
                    else_block: Optional[List[Tuple[str, str]]] = None
                    if k < n and body[k][0] == "IDENT" and body[k][1] == "else":
                        k += 1
                        if k < n and body[k][1] == "{":
                            else_block, after_else = _extract_block_tokens(body, k)
                            next_idx = after_else
                        else:
                            next_idx = k
                    else:
                        next_idx = k
                    # try to evaluate condition
                    cond_eval = _is_simple_constant_condition(cond_tokens)
                    if cond_eval is True:
                        out.extend(then_block)
                        i = next_idx
                        continue
                    elif cond_eval is False:
                        if else_block is not None:
                            out.extend(else_block)
                            i = next_idx
                            continue
                        else:
                            # no else -> drop
                            i = next_idx
                            continue
                    else:
                        # cannot evaluate statically -> keep original tokens (so other steps can see declarations)
                        # append original if (...) { ... } [else { ... }] raw tokens
                        # reconstruct original sequence from i to next_idx
                        for idx in range(i, next_idx):
                            out.append(body[idx])
                        i = next_idx
                        continue
                else:
                    # malformed if - just copy token
                    out.append(body[i])
                    i += 1
                    continue
            else:
                out.append(body[i])
                i += 1
                continue
        else:
            out.append(body[i])
            i += 1
    return out

# Code generator: AST -> C (improved: auto-declare implicit vars, constant-fold simple ifs)

def codegen_to_c(ast: Dict[str, Any], module_name: str = "module", imports: Optional[List[str]] = None) -> str:
    lines: List[str] = []
    lines.append("/* Generated by CYON compiler - improved codegen */")
    lines.append("#include <stdio.h>")
    lines.append("#include <stdlib.h>")
    lines.append("#include <string.h>")
    if (INCLUDE / "cyonstd.h").exists():
        lines.append('#include "cyonstd.h"')
    lines.append("")

    if imports:
        for im in imports:
            hdr = INCLUDE / f"{im}.h"
            if hdr.exists():
                lines.append(f'#include "{im}.h"')
        lines.append("")

    # Input helpers
    lines.append("int cyon_input_int(const char *prompt) {")
    lines.append('    if (prompt) { printf("%s", prompt); fflush(stdout); }')
    lines.append("    int value = 0;")
    lines.append('    scanf("%d", &value);')
    lines.append('    printf("\\n");')  # Add newline after input
    lines.append("    return value;")
    lines.append("}")
    lines.append("")
    lines.append("double cyon_input_double(const char *prompt) {")
    lines.append('    if (prompt) { printf("%s", prompt); fflush(stdout); }')
    lines.append("    double value = 0.0;")
    lines.append('    scanf("%lf", &value);')
    lines.append('    printf("\\n");')  # Add newline after input
    lines.append("    return value;")
    lines.append("}")
    lines.append("")
    lines.append("char* cyon_input_str(const char *prompt) {")
    lines.append('    if (prompt) { printf("%s", prompt); fflush(stdout); }')
    lines.append("    char *buf = malloc(1024);")
    lines.append("    if (!buf) return NULL;")
    lines.append('    if (fgets(buf, 1024, stdin)) {')
    lines.append("        size_t len = strlen(buf);")
    lines.append("        if (len > 0 && buf[len-1] == '\\n') buf[len-1] = '\\0';")
    lines.append("        return buf;")
    lines.append("    }")
    lines.append("    free(buf);")
    lines.append("    return NULL;")
    lines.append("}")
    lines.append("")

    for fn in ast.get("functions", []):
        name = fn.get("name", "anon")
        body = fn.get("body", [])
        # first, try to constant-fold simple if/else sequences
        body = _simplify_if_else(body)
        # collect implicit variables and explicit ones
        implicit = _collect_implicit_vars(body)
        # build var_types initial map (explicit declarations will be added during generation)
        var_types: Dict[str, str] = dict(implicit)

        lines.append(f"void cyon_func_{name}(void) {{")
        # predeclare implicit vars
        for varname, typ in implicit.items():
            if typ == "int":
                lines.append(f"    int {varname} = 0;")
            elif typ == "double":
                lines.append(f"    double {varname} = 0.0;")
            else:
                lines.append(f'    char *{varname} = NULL;')
        if implicit:
            lines.append("")

        i = 0
        while i < len(body):
            t, v = body[i]

            # For now, let's disable complex control structures and just handle simple if
            if t == "IDENT" and v == "if":
                i += 1
                # collect condition
                condition_tokens: List[str] = []
                while i < len(body) and body[i][1] != ":":
                    condition_tokens.append(body[i][1])
                    i += 1
                condition = " ".join(condition_tokens).strip()
                # Generate single-line if statement to avoid brace issues
                if i < len(body) and body[i][1] == ":":
                    i += 1  # skip the colon
                    # Look ahead for the next statement
                    next_stmt_tokens: List[str] = []
                    while i < len(body) and body[i][1] != ";":
                        if body[i][0] == "STRING":
                            next_stmt_tokens.append(json.dumps(body[i][1]))
                        else:
                            next_stmt_tokens.append(body[i][1])
                        i += 1
                    next_stmt = " ".join(next_stmt_tokens).strip()
                    
                    # Generate appropriate statement
                    if next_stmt.startswith("print"):
                        # Extract print arguments
                        print_args = next_stmt[5:].strip()
                        if print_args.startswith("(") and print_args.endswith(")"):
                            print_args = print_args[1:-1]
                        # Add newline to the print statement in if condition
                        # The print_args comes from json.dumps() so it's properly quoted
                        if print_args and print_args.startswith('"') and print_args.endswith('"'):
                            # Remove the closing quote, add \n, then add quote back
                            print_args_with_newline = print_args[:-1] + '\\n"'
                            lines.append(f"    if ({condition}) printf({print_args_with_newline});")
                        else:
                            # Fallback for complex expressions
                            lines.append(f"    if ({condition}) printf({print_args});")
                    else:
                        lines.append(f"    if ({condition}) {next_stmt};")
                    
                    if i < len(body) and body[i][1] == ";":
                        i += 1
                continue

            # explicit declarations: int x = ..., double x = ..., string x = ...
            if t == "IDENT" and v in ("int", "double", "string", "str"):
                var_type = v
                i += 1
                if i < len(body) and body[i][0] == "IDENT":
                    var_name = body[i][1]
                    i += 1
                    # optional assignment
                    if i < len(body) and body[i][1] == "=":
                        i += 1
                        value_tokens: List[str] = []
                        while i < len(body) and body[i][1] != ";":
                            # for string tokens, wrap as C string literal
                            if body[i][0] == "STRING":
                                value_tokens.append(json.dumps(body[i][1]))
                            else:
                                value_tokens.append(body[i][1])
                            i += 1
                        # Check for input functions in the value tokens
                        final_value_tokens: List[str] = []
                        j = 0
                        while j < len(value_tokens):
                            if value_tokens[j] in ("input_int", "input_double", "input_str", "input"):
                                input_func = value_tokens[j]
                                j += 1
                                if j < len(value_tokens) and value_tokens[j] == "(":
                                    j += 1
                                    prompt_tokens: List[str] = []
                                    paren_count = 1
                                    while j < len(value_tokens) and paren_count > 0:
                                        if value_tokens[j] == "(":
                                            paren_count += 1
                                        elif value_tokens[j] == ")":
                                            paren_count -= 1
                                        if paren_count > 0:
                                            prompt_tokens.append(value_tokens[j])
                                        j += 1
                                    
                                    prompt_expr = " ".join(prompt_tokens).strip() if prompt_tokens else "NULL"
                                    
                                    if input_func == "input_int":
                                        final_value_tokens.append(f"cyon_input_int({prompt_expr})")
                                    elif input_func == "input_double":
                                        final_value_tokens.append(f"cyon_input_double({prompt_expr})")
                                    else:  # input_str or input
                                        final_value_tokens.append(f"cyon_input_str({prompt_expr})")
                                    j -= 1  # Adjust for the loop increment
                                else:
                                    final_value_tokens.append(input_func)
                            else:
                                final_value_tokens.append(value_tokens[j])
                            j += 1
                        
                        value = " ".join(final_value_tokens).strip()
                        if var_type == "int":
                            lines.append(f"    int {var_name} = {value};")
                            var_types[var_name] = "int"
                        elif var_type == "double":
                            lines.append(f"    double {var_name} = {value};")
                            var_types[var_name] = "double"
                        else:
                            # string
                            if value.startswith('"') and value.endswith('"'):
                                lines.append(f'    char *{var_name} = {value};')
                            else:
                                # ensure quoted
                                val_q = json.dumps(value)
                                lines.append(f'    char *{var_name} = {val_q};')
                            var_types[var_name] = "string"
                        # skip semicolon
                        if i < len(body) and body[i][1] == ";":
                            i += 1
                        continue
                    else:
                        # declaration without assignment
                        if var_type == "int":
                            lines.append(f"    int {var_name} = 0;")
                            var_types[var_name] = "int"
                        elif var_type == "double":
                            lines.append(f"    double {var_name} = 0.0;")
                            var_types[var_name] = "double"
                        else:
                            lines.append(f"    char *{var_name} = NULL;")
                            var_types[var_name] = "string"
                        continue
                continue

            # assignment to already-declared or implicitly declared variable
            if t == "IDENT" and i + 1 < len(body) and body[i+1][1] == "=":
                var_name = v
                i += 2
                value_tokens: List[str] = []
                # collect RHS tokens until semicolon
                while i < len(body) and body[i][1] != ";":
                    if body[i][0] == "STRING":
                        value_tokens.append(json.dumps(body[i][1]))
                    elif body[i][0] == "IDENT" and body[i][1] in ("input_int", "input_double", "input_str", "input"):
                        # Handle input function calls in assignments
                        input_func = body[i][1]
                        i += 1
                        if i < len(body) and body[i][1] == "(":
                            i += 1
                            prompt_tokens: List[str] = []
                            paren_depth = 1
                            while i < len(body) and paren_depth > 0:
                                if body[i][1] == "(":
                                    paren_depth += 1
                                elif body[i][1] == ")":
                                    paren_depth -= 1
                                if paren_depth > 0:
                                    if body[i][0] == "STRING":
                                        prompt_tokens.append(json.dumps(body[i][1]))
                                    else:
                                        prompt_tokens.append(body[i][1])
                                i += 1
                            
                            prompt_expr = " ".join(prompt_tokens).strip() if prompt_tokens else "NULL"
                            
                            if input_func == "input_int":
                                value_tokens.append(f"cyon_input_int({prompt_expr})")
                            elif input_func == "input_double":
                                value_tokens.append(f"cyon_input_double({prompt_expr})")
                            else:  # input_str or input
                                value_tokens.append(f"cyon_input_str({prompt_expr})")
                            i -= 1  # Back up one to handle the semicolon check properly
                    else:
                        value_tokens.append(body[i][1])
                    i += 1
                value = " ".join(value_tokens).strip()
                # if var not known, fall back to char*
                vtype = var_types.get(var_name, "string")
                # generate assignment (no redeclaration)
                if vtype == "int":
                    lines.append(f"    {var_name} = {value};")
                elif vtype == "double":
                    lines.append(f"    {var_name} = {value};")
                else:
                    # string pointer assign, if RHS is function returning malloc'd string we assign directly
                    if value.startswith('"') and value.endswith('"'):
                        lines.append(f"    {var_name} = strdup({value});")
                    else:
                        lines.append(f"    {var_name} = {value};")
                if i < len(body) and body[i][1] == ";":
                    i += 1
                continue

            # simple print translation
            if t == "IDENT" and v == "print":
                i += 1
                args_tokens: List[List[Tuple[str, str]]] = []
                current: List[Tuple[str, str]] = []
                paren_depth = 0
                collecting = False

                while i < len(body):
                    tok_type, tok_val = body[i]
                    if tok_val == "(":
                        paren_depth += 1
                        collecting = True
                        i += 1
                        continue
                    if tok_val == ")":
                        paren_depth -= 1
                        if collecting and current:
                            args_tokens.append(current)
                            current = []
                        if paren_depth == 0:
                            i += 1
                            break
                        i += 1
                        continue
                    if collecting and paren_depth > 0:
                        if tok_val == ",":
                            if current:
                                args_tokens.append(current)
                                current = []
                        else:
                            current.append((tok_type, tok_val))
                    i += 1

                if current:
                    args_tokens.append(current)

                fmt_text = ""
                fmt_vals: List[str] = []

                for arg in args_tokens:
                    if len(arg) == 0:
                        continue
                    if len(arg) == 1:
                        atype, aval = arg[0]
                        if atype == "STRING":
                            piece = json.dumps(aval)
                            # drop surrounding quotes when embedding into format, but we need them as part of overall printf format string
                            # append directly as literal piece
                            literal = aval
                            if fmt_text and not fmt_text.endswith(" ") and not literal.startswith(" "):
                                fmt_text += " " + literal
                            else:
                                fmt_text += literal
                            continue
                        elif atype == "NUMBER":
                            if fmt_text and not fmt_text.endswith(" "):
                                fmt_text += " "
                            # choose %d or %f based on presence of dot
                            if "." in aval:
                                fmt_text += "%f"
                            else:
                                fmt_text += "%d"
                            fmt_vals.append(aval)
                            continue
                        elif atype == "IDENT":
                            vtype = var_types.get(aval)
                            if vtype == "int":
                                if fmt_text and not fmt_text.endswith(" "):
                                    fmt_text += " "
                                fmt_text += "%d"
                                fmt_vals.append(aval)
                                continue
                            elif vtype == "double":
                                if fmt_text and not fmt_text.endswith(" "):
                                    fmt_text += " "
                                fmt_text += "%f"
                                fmt_vals.append(aval)
                                continue
                            else:
                                if fmt_text and not fmt_text.endswith(" "):
                                    fmt_text += " "
                                fmt_text += "%s"
                                fmt_vals.append(aval)
                                continue
                    # composite expression
                    expr = " ".join(tok_val for tok_type, tok_val in arg)
                    numeric_hint = any(tok_type == "NUMBER" or (tok_type == "IDENT" and var_types.get(tok_val) == "int") for tok_type, tok_val in arg)
                    if fmt_text and not fmt_text.endswith(" "):
                        fmt_text += " "
                    if numeric_hint:
                        fmt_text += "%d"
                    else:
                        fmt_text += "%s"
                    fmt_vals.append(expr)

                # Automatically add newline to each print statement for proper line ending
                if not fmt_text.endswith('\n'):
                    fmt_text += '\n'
                fmt_text_c = json.dumps(fmt_text)  # safe quoting

                if fmt_vals:
                    vals_str = ", ".join(fmt_vals)
                    lines.append(f"    printf({fmt_text_c}, {vals_str});")
                else:
                    lines.append(f"    printf({fmt_text_c});")

                # skip semicolon
                if i < len(body) and body[i][1] == ";":
                    i += 1
                continue

            # Handle closing braces and other tokens
            if v == "}":
                lines.append("    }")
                i += 1
                continue
                
            # other tokens we don't specially translate: skip semicolons and stray tokens
            # This keeps generator forgiving and avoids crashing on unknown constructs.
            if v == ";" :
                i += 1
                continue

            # fallback: try to emit an expression if token sequence looks like a standalone function call e.g. foo();
            if t == "IDENT" and i + 1 < len(body) and body[i+1][1] == "(":
                # gather until semicolon
                call_parts: List[str] = []
                while i < len(body) and body[i][1] != ";":
                    if body[i][0] == "STRING":
                        call_parts.append(json.dumps(body[i][1]))
                    else:
                        call_parts.append(body[i][1])
                    i += 1
                call_code = " ".join(call_parts)
                lines.append(f"    {call_code};")
                if i < len(body) and body[i][1] == ";":
                    i += 1
                continue

            # unknown token - skip
            i += 1

        # Close any remaining open braces
        lines.append("}")
        lines.append("")

    # generate main
    has_main = any(fn.get("name") == "main" for fn in ast.get("functions", []))
    lines.append("int main(void) {")
    if has_main:
        lines.append("    cyon_func_main();")
    else:
        lines.append('    printf("CYON: no main found.\\n");')
    lines.append("    return 0;")
    lines.append("}")

    return "\n".join(lines)

# Compiler pipeline helpers

def compile_source_to_c(source_path: Path, out_c_path: Path) -> Tuple[Path, List[str], List[Path], List[Path]]:
    """
    Compile .cyon source to .c file.
    Returns tuple: (out_c_path, import_names, c_sources_for_imports, py_modules_for_imports)
    """
    if not source_path.exists():
        raise FileNotFoundError(f"Source not found: {source_path}")
    src_text = source_path.read_text(encoding="utf-8")
    imports = parse_imports_from_source(src_text)
    toks = lex(src_text)
    ast = parse(toks)
    ast = optimize(ast)
    ccode = codegen_to_c(ast, module_name=source_path.stem, imports=imports)
    ensure_dir(out_c_path.parent)
    out_c_path.write_text(ccode, encoding="utf-8")

    c_sources = resolve_imports_to_c_sources(imports)
    py_modules = resolve_imports_to_py_modules(imports)
    return out_c_path, imports, c_sources, py_modules

# Build helpers

def build_c_to_exe(c_file: Path, output_path: Path, target: str = "native", extra_flags: Optional[List[str]] = None, extra_c_sources: Optional[List[Path]] = None) -> Path:
    ensure_dir(output_path.parent)
    cc = os.environ.get("CC", "gcc")
    flags = ["-O2", "-std=c11", "-I", str(INCLUDE), "-I", str(LIBRARIES)]
    link_flags: List[str] = []
    if extra_flags:
        flags.extend(extra_flags)

    # Add pthread and math when likely needed
    link_flags.extend(["-pthread", "-lm"])

    # choose compiler for target
    if target == "native":
        clang = shutil.which("clang")
        if clang:
            cc = clang
        cmd = [cc, *flags, str(c_file)]
        if extra_c_sources:
            cmd.extend([str(p) for p in extra_c_sources])
        stdlib = find_stdlib_archive()
        if stdlib:
            cmd.append(str(stdlib))
        cmd.extend([*link_flags, "-o", str(output_path)])
    elif target == "linux":
        cmd = [cc, *flags, str(c_file)]
        if extra_c_sources:
            cmd.extend([str(p) for p in extra_c_sources])
        stdlib = find_stdlib_archive()
        if stdlib:
            cmd.append(str(stdlib))
        cmd.extend([*link_flags, "-o", str(output_path)])
    elif target == "macos":
        clang = shutil.which("clang")
        if clang:
            cc = clang
        cmd = [cc, *flags, str(c_file)]
        if extra_c_sources:
            cmd.extend([str(p) for p in extra_c_sources])
        stdlib = find_stdlib_archive()
        if stdlib:
            cmd.append(str(stdlib))
        cmd.extend([*link_flags, "-o", str(output_path)])
    elif target == "windows":
        mingw = shutil.which("x86_64-w64-mingw32-gcc") or shutil.which("i686-w64-mingw32-gcc")
        if mingw:
            cmd = [mingw, *flags, str(c_file)]
            if extra_c_sources:
                cmd.extend([str(p) for p in extra_c_sources])
            stdlib = find_stdlib_archive()
            if stdlib:
                cmd.append(str(stdlib))
            cmd.extend([*link_flags, "-o", str(output_path.with_suffix('.exe'))])
        else:
            cmd = [cc, *flags, str(c_file)]
            if extra_c_sources:
                cmd.extend([str(p) for p in extra_c_sources])
            stdlib = find_stdlib_archive()
            if stdlib:
                cmd.append(str(stdlib))
            cmd.extend([*link_flags, "-o", str(output_path.with_suffix('.exe'))])
    else:
        raise ValueError(f"Unsupported target: {target}")

    run_cmd(cmd)
    return output_path.with_suffix('.exe' if target == 'windows' else output_path.suffix)

# High-level CLI actions

def action_run(args: argparse.Namespace) -> int:
    source = Path(args.source)
    if not source.exists():
        eprint(f"[cyon] Source not found: {source}")
        return 2
    ensure_dir(BUILD)
    out_c = BUILD / (source.stem + ".c")
    out_bin = BUILD / (source.stem)
    if args.target == 'windows':
        out_bin = out_bin.with_suffix('.exe')
    out_c_path, imports, c_sources, py_modules = compile_source_to_c(source, out_c)
    # If we have python module imports, warn (they are for runtime python wrappers)
    if py_modules:
        eprint(f"[cyon] Note: python wrapper modules found for imports: {[p.name for p in py_modules]}")
    build_c_to_exe(out_c_path, out_bin, target=args.target, extra_c_sources=c_sources)
    # run produced binary
    exe_path = str(out_bin)
    if args.target == 'windows' and not exe_path.lower().endswith('.exe'):
        exe_path += '.exe'
    try:
        cp = run_cmd([exe_path], check=False)
        return cp.returncode if isinstance(cp, subprocess.CompletedProcess) else 0
    except FileNotFoundError:
        eprint(f"[cyon] Executable not found: {exe_path}")
        return 3

def action_build(args: argparse.Namespace) -> int:
    src = Path(args.source)
    ensure_dir(BUILD)

    # convenience: if directory given, build every .cyon inside it
    targets: List[Tuple[Path, Path]] = []
    if src.is_dir():
        sources = list(src.glob("*.cyon"))
        if not sources:
            eprint("[cyon] No .cyon files found to build.")
            return 1
        for s in sources:
            out_c = BUILD / (s.stem + ".c")
            out_bin = BUILD / (s.stem)
            targets.append((out_c, out_bin))
        # compile all and collect extra c sources
        for s, b in zip(sources, [t[1] for t in targets]):
            out_c_path, imports, c_sources, py_modules = compile_source_to_c(s, BUILD / (s.stem + ".c"))
            if py_modules:
                eprint(f"[cyon] Note: python wrapper modules for {s.name}: {[p.name for p in py_modules]}")
            build_c_to_exe(out_c_path, b, target=args.target, extra_c_sources=c_sources)
    else:
        out_c_path, imports, c_sources, py_modules = compile_source_to_c(src, BUILD / (src.stem + ".c"))
        out_bin = BUILD / (src.stem)
        if args.target == 'windows':
            out_bin = out_bin.with_suffix('.exe')
        if py_modules:
            eprint(f"[cyon] Note: python wrapper modules for {src.name}: {[p.name for p in py_modules]}")
        build_c_to_exe(out_c_path, out_bin, target=args.target, extra_c_sources=c_sources)

    # run post-build if requested
    if getattr(args, "run", False):
        # reuse action_run behavior
        return action_run(args)

    return 0

def action_clean(args: argparse.Namespace) -> int:
    if BUILD.exists():
        shutil.rmtree(BUILD)
    # also call runtime clean optionally
    if getattr(args, "runtime", False):
        rp = RUNTIME
        if (rp / "Makefile").exists():
            try:
                run_cmd(["make", "clean"], cwd=rp)
            except Exception:
                eprint("[cyon] runtime make clean failed.")
    eprint("[cyon] Clean complete.")
    return 0


def action_init(args: argparse.Namespace) -> int:
    for p in [CORE, CORE / "runtime", INCLUDE, BUILD, EXAMPLES, ROOT / "lib", ROOT / "tests", LIBRARIES]:
        ensure_dir(p)
    settings = ROOT / "settings.py"
    if not settings.exists():
        settings.write_text(textwrap.dedent(f"""\
        # Generated settings for Cyon project
        NAME = "{CYON_NAME}"
        VERSION = "{VERSION}"
        ROOT = r"{ROOT}"
        """), encoding="utf-8")
    hello = EXAMPLES / "hello.cyon"
    if not hello.exists():
        hello.write_text('func main() { print("Hello, Cyon!\\n"); }', encoding="utf-8")
    eprint("[cyon] Project skeleton initialized.")
    return 0


def action_test(args: argparse.Namespace) -> int:
    try:
        run_cmd([sys.executable, "-m", "pytest", "tests"], check=True)
    except Exception:
        eprint("[cyon] Pytest failed or not available.")
    test_c = ROOT / "tests" / "test_runtime.c"
    if test_c.exists():
        out = BUILD / "test_runtime"
        try:
            build_c_to_exe(test_c, out, target="native")
            run_cmd([str(out)])
        except Exception:
            eprint("[cyon] C runtime test failed.")
    return 0

def action_docs(args: argparse.Namespace) -> int:
    r = ROOT / "README.md"
    if r.exists():
        eprint(f"[cyon] README: {r}")
    else:
        eprint("[cyon] No README.md found")
    return 0

def make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="cyon", description="CYON language toolchain CLI")
    subparsers = parser.add_subparsers(dest="command", metavar="<command>")

    # run
    sp_run = subparsers.add_parser("run", help="compile & run a .cyon file immediately")
    sp_run.add_argument("source", help="CYON source file or directory")
    sp_run.add_argument("--target", choices=["native", "windows", "linux", "macos"], default=DEFAULT_TARGET, help="target OS to build for")
    sp_run.add_argument("--root", action="store_true", help="run as root/admin (checked, not escalated automatically)")
    sp_run.set_defaults(func=action_run)

    # build
    sp_build = subparsers.add_parser("build", help="build a .cyon file or directory")
    sp_build.add_argument("source", help="CYON source file or directory")
    sp_build.add_argument("--target", choices=["native", "windows", "linux", "macos"], default=DEFAULT_TARGET, help="target OS to build for")
    sp_build.add_argument("--release", action="store_true", help="build release (strip symbols, optimize)")
    sp_build.add_argument("--root", action="store_true", help="build with root privileges (checked)")
    sp_build.add_argument("--run", action="store_true", help="run after a successful build (convenience)")
    sp_build.set_defaults(func=action_build)

    # clean
    sp_clean = subparsers.add_parser("clean", help="clean build artifacts")
    sp_clean.add_argument("--runtime", action="store_true", help="also run core/runtime/Makefile clean")
    sp_clean.set_defaults(func=action_clean)

    # init
    sp_init = subparsers.add_parser("init", help="initialize a new Cyon project skeleton")
    sp_init.set_defaults(func=action_init)

    # test
    sp_test = subparsers.add_parser("test", help="run tests for project")
    sp_test.set_defaults(func=action_test)

    # docs
    sp_docs = subparsers.add_parser("docs", help="open or build docs")
    sp_docs.set_defaults(func=action_docs)

    # version
    sp_version = subparsers.add_parser("version", help="print version")
    sp_version.set_defaults(func=lambda args: print(f"{CYON_NAME} {VERSION}"))

    # Additional convenience commands programmatically registered
    aliases: Dict[str, Callable[[argparse.Namespace], int]] = {}
    aliases["b"] = action_build
    aliases["build-win"] = lambda a: action_build(argparse.Namespace(source=a.source if hasattr(a, 'source') else str(EXAMPLES), target="windows", release=False, root=False, run=False))
    aliases["build-linux"] = lambda a: action_build(argparse.Namespace(source=a.source if hasattr(a, 'source') else str(EXAMPLES), target="linux", release=False, root=False, run=False))
    aliases["build-macos"] = lambda a: action_build(argparse.Namespace(source=a.source if hasattr(a, 'source') else str(EXAMPLES), target="macos", release=False, root=False, run=False))
    aliases["run-fast"] = action_run
    aliases["compile"] = action_build
    aliases["rebuild"] = lambda a: (action_clean(argparse.Namespace(runtime=False)), action_build(a))[1]
    aliases["rb"] = aliases["rebuild"]

    # create many small aliases
    for i in range(10):
        aliases[f"quick{i}"] = action_run
    for i in range(10):
        aliases[f"make{i}"] = action_build
    for i in range(10):
        aliases[f"do{i}"] = action_build
    for i in range(10):
        aliases[f"go{i}"] = action_run

    aliases["dep"] = lambda a: (eprint("[cyon] deps not implemented; use package manager"), 0)[1]
    aliases["deps"] = lambda a: (eprint("[cyon] deps not implemented; use package manager"), 0)[1]
    aliases["fmt"] = lambda a: (eprint("[cyon] format not implemented"), 0)[1]
    aliases["lint"] = lambda a: (eprint("[cyon] lint not implemented"), 0)[1]
    aliases["doc"] = action_docs
    aliases["pkg"] = lambda a: (eprint("[cyon] package not implemented"), 0)[1]
    aliases["install"] = lambda a: (eprint("[cyon] install not implemented"), 0)[1]

    for name in sorted(aliases.keys()):
        sp = subparsers.add_parser(name, help=f"alias command: {name}")
        sp.add_argument("source", nargs="?", default=str(EXAMPLES), help="source file or directory")
        sp.add_argument("--target", choices=["native", "windows", "linux", "macos"], default=DEFAULT_TARGET)
        sp.set_defaults(func=aliases[name])

    return parser

# Entrypoint

def main(argv: Optional[List[str]] = None) -> int:
    parser = make_parser()
    args = parser.parse_args(argv)
    if args is None or not hasattr(args, "func"):
        parser.print_help()
        return 0

    if getattr(args, "root", False):
        if not is_root_user():
            eprint("[cyon] Warning: --root requested but you are not running as root/admin.")
        else:
            eprint("[cyon] Running with root privileges.")

    try:
        res = args.func(args)
        if isinstance(res, int):
            return res
        return 0
    except Exception as ex:
        eprint(f"[cyon] Error: {ex}")
        return 1

if __name__ == "__main__":
    sys.exit(main())