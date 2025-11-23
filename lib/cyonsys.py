from __future__ import annotations

import os
import sys
import subprocess
import shlex
import shutil
import platform
import time
import typing
from typing import Optional, List, Dict, Tuple, Any, Callable

# Module metadata
__version__ = "0.1.0"
__author__ = "Luka"
__license__ = "MIT"

# Default project layout assumptions (adjust in settings.py if needed)
ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
CORE_DIR = os.path.join(ROOT_DIR, "core")
RUNTIME_DIR = os.path.join(CORE_DIR, "runtime")
BUILD_DIR = os.path.join(ROOT_DIR, "build")
INCLUDE_DIR = os.path.join(ROOT_DIR, "include")

# Platform detection
IS_WINDOWS = platform.system().lower().startswith("win")
IS_LINUX = platform.system().lower().startswith("linux")
IS_DARWIN = platform.system().lower().startswith("darwin")

# Logging primitives (simple, no external deps)
def _timestamp() -> str:
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())

def log_info(msg: str) -> None:
    sys.stdout.write(f"[INFO] {_timestamp()} - {msg}\n")

def log_warn(msg: str) -> None:
    sys.stdout.write(f"[WARN] {_timestamp()} - {msg}\n")

def log_error(msg: str) -> None:
    sys.stderr.write(f"[ERROR] {_timestamp()} - {msg}\n")

# Helper: robust run subprocess wrapper
def run_cmd(cmd: str,
            cwd: Optional[str] = None,
            env: Optional[Dict[str, str]] = None,
            capture_output: bool = False,
            check: bool = False,
            shell: bool = False,
            timeout: Optional[float] = None) -> subprocess.CompletedProcess:
    if isinstance(cmd, str) and not shell:
        # Try to split for safety
        argv = shlex.split(cmd)
    else:
        argv = cmd

    log_info(f"Running command: {cmd} (cwd={cwd})")
    try:
        if capture_output:
            cp = subprocess.run(argv, cwd=cwd, env=env, capture_output=True, text=True, check=check, shell=shell, timeout=timeout)
        else:
            cp = subprocess.run(argv, cwd=cwd, env=env, text=True, check=check, shell=shell, timeout=timeout)
        if capture_output:
            log_info(f"Command stdout: {cp.stdout.strip()[:1000]}")
            if cp.stderr:
                log_warn(f"Command stderr: {cp.stderr.strip()[:1000]}")
        return cp
    except subprocess.CalledProcessError as ex:
        log_error(f"Command failed with exit {ex.returncode}: {ex}")
        raise
    except subprocess.TimeoutExpired as ex:
        log_error(f"Command timed out: {ex}")
        raise
    except Exception as ex:
        log_error(f"Unexpected error running command: {ex}")
        raise

# Filesystem utilities
def ensure_dir(path: str) -> None:
    if not os.path.exists(path):
        os.makedirs(path, exist_ok=True)
        log_info(f"Created directory: {path}")

def safe_remove(path: str) -> None:
    try:
        if os.path.isdir(path):
            shutil.rmtree(path)
            log_info(f"Removed directory tree: {path}")
        elif os.path.exists(path):
            os.remove(path)
            log_info(f"Removed file: {path}")
    except Exception as ex:
        log_warn(f"Could not remove {path}: {ex}")

def list_c_files(directory: str) -> List[str]:
    return [f for f in os.listdir(directory) if f.endswith(".c")]

def find_runtime_lib() -> Optional[str]:
    names = ["libcyon.a", "libcyon.so", "libcyon.dylib", "cyon.dll"]
    for n in names:
        candidate = os.path.join(RUNTIME_DIR, n)
        if os.path.exists(candidate):
            return candidate
    return None

# Build orchestration for runtime (calls Makefile)
def build_runtime(debug: bool = False) -> None:
    if not os.path.isdir(RUNTIME_DIR):
        raise FileNotFoundError(f"Runtime dir missing: {RUNTIME_DIR}")
    cmd = "make debug" if debug else "make"
    run_cmd(cmd, cwd=RUNTIME_DIR, shell=True)
    lib = find_runtime_lib()
    if lib:
        log_info(f"Runtime library created: {lib}")
    else:
        log_warn("Runtime library not found after build (check Makefile).")

# Top-level build for whole project: compile examples, link, etc.
def build_project() -> None:
    ensure_dir(BUILD_DIR)
    build_runtime(debug=False)
    # Potentially add additional steps: compile C wrappers, build extensions
    log_info("Project build finished.")

def clean_runtime() -> None:
    if not os.path.isdir(RUNTIME_DIR):
        log_warn("Runtime directory not found; nothing to clean.")
        return
    run_cmd("make clean", cwd=RUNTIME_DIR, shell=True)
    # Optionally remove build outputs under build/
    safe_remove(os.path.join(RUNTIME_DIR, "libcyon.a"))
    log_info("Runtime cleaned.")

def rebuild_runtime() -> None:
    clean_runtime()
    build_runtime()

# Example runner: compile a generated C file and run
def compile_and_run_c(source_file: str, output_name: Optional[str] = None, extra_cflags: Optional[str] = None) -> int:
    if not os.path.exists(source_file):
        raise FileNotFoundError(source_file)
    if output_name is None:
        output_name = os.path.splitext(os.path.basename(source_file))[0]
    outpath = os.path.join(BUILD_DIR, output_name)
    ensure_dir(BUILD_DIR)

    cc = os.environ.get("CC", "gcc")
    cflags = "-O2 -std=c11"
    if extra_cflags:
        cflags += " " + extra_cflags
    lib = find_runtime_lib()
    link_lib = lib if lib else ""
    cmd = f"{cc} {cflags} {shlex.quote(source_file)} -I{shlex.quote(INCLUDE_DIR)} {shlex.quote(link_lib)} -o {shlex.quote(outpath)}"
    run_cmd(cmd, shell=True)
    log_info(f"Compiled {source_file} -> {outpath}")
    # Run
    cp = run_cmd(shlex.quote(outpath), shell=True, check=False)
    return cp.returncode if isinstance(cp, subprocess.CompletedProcess) else 0

# Utility: run example .cyon file through compiler and run result
def run_example(example_name: str) -> None:
    examples_dir = os.path.join(ROOT_DIR, "examples")
    src = os.path.join(examples_dir, example_name)
    if not os.path.exists(src):
        raise FileNotFoundError(src)
    # Assuming compiler.py can translate .cyon -> .c and place in build/
    compiler_py = os.path.join(ROOT_DIR, "compiler.py")
    if os.path.exists(compiler_py):
        run_cmd(f"python {shlex.quote(compiler_py)} {shlex.quote(src)}", shell=True)
        generated_c = os.path.join(BUILD_DIR, os.path.splitext(example_name)[0] + ".c")
        if os.path.exists(generated_c):
            compile_and_run_c(generated_c)
        else:
            log_warn("Generated C not found; check compiler output.")
    else:
        log_warn("compiler.py not found; cannot run example automatically.")

# Small helpers for paths and file idempotency
def canonical_path(path: str) -> str:
    return os.path.abspath(os.path.expanduser(path))

def write_atomic(path: str, data: str, mode: str = "w") -> None:
    tmp = path + ".tmp"
    ensure_dir(os.path.dirname(path) or ".")
    with open(tmp, mode, encoding="utf-8") as f:
        f.write(data)
        f.flush()
        os.fsync(f.fileno())
    os.replace(tmp, path)
    log_info(f"Wrote file atomically: {path}")

# Environment and configuration utilities
def get_env(key: str, default: Optional[str] = None) -> Optional[str]:
    return os.environ.get(key, default)

def set_env(key: str, value: str) -> None:
    os.environ[key] = value

# Simple process runner with streaming output
def run_and_stream(cmd: str, cwd: Optional[str] = None, env: Optional[Dict[str, str]] = None) -> int:
    """
    Run command and stream stdout/stderr live.
    Returns exit code.
    """
    log_info(f"Streaming command: {cmd}")
    if isinstance(cmd, str):
        argv = cmd if sys.platform == "win32" else shlex.split(cmd)
    else:
        argv = cmd
    proc = subprocess.Popen(argv, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, shell=False)
    assert proc.stdout is not None
    try:
        for line in proc.stdout:
            sys.stdout.write(line)
        proc.wait()
        return proc.returncode
    except KeyboardInterrupt:
        proc.terminate()
        raise

# Safety wrapper for invoking the top-level make (if present)
def make_top(clean: bool = False) -> None:
    top_make = os.path.join(ROOT_DIR, "Makefile")
    if not os.path.exists(top_make):
        log_warn("Top-level Makefile not found in project root.")
        return
    cmd = "make clean" if clean else "make"
    run_cmd(cmd, cwd=ROOT_DIR, shell=True)

# Small convenience for running tests
def run_pytests(pattern: Optional[str] = None) -> None:
    # Use pytest if available, fallback to unittest discovery
    try:
        import pytest  # type: ignore
        args = []
        if pattern:
            args.append(pattern)
        exit_code = pytest.main(args)
        if exit_code != 0:
            raise SystemExit(exit_code)
    except Exception:
        # fallback
        cmd = f"python -m unittest discover -v"
        run_cmd(cmd, cwd=ROOT_DIR, shell=True)

# Generator: create many small helper stubs if you need them
def generate_stub_helpers(filename: str,
                          prefix: str = "cyonsys_helper",
                          count: int = 1000,
                          inline: bool = False,
                          overwrite: bool = True) -> str:
    ensure_dir(os.path.dirname(filename) or ".")
    if os.path.exists(filename) and not overwrite:
        raise FileExistsError(filename + " already exists")

    lines: List[str] = []
    header = [
        "# Auto-generated helper stubs",
        "# Generated by lib/cyonsys.generate_stub_helpers",
        f"# prefix={prefix}, count={count}, inline={inline}",
        "",
        "from typing import Any",
        "",
    ]
    lines.extend(header)

    for i in range(count):
        idx = str(i).zfill(3)
        # keep each function short + docstring to be useful
        lines.append(f"def {prefix}_{idx}() -> None:")
        lines.append(f"    \"\"\"Helper stub #{i} - no-op placeholder.\"\"\"")
        lines.append(f"    # volatile-like placeholder to avoid optimizer removal in C examples")
        lines.append(f"    _tmp = {i}")
        lines.append(f"    _ = _tmp")
        lines.append("")  # blank line

    content = "\n".join(lines)
    write_atomic(filename, content)
    log_info(f"Wrote stub helpers to: {filename} (count={count})")
    return filename

# Example convenience wrapper to generate exactly 1000 helpers in project lib/
def generate_1000_helpers_in_lib():
    out = os.path.join(ROOT_DIR, "lib", "cyonsys_helpers_generated.py")
    return generate_stub_helpers(out, prefix="cyonsys_helper", count=1000, inline=False, overwrite=True)

# Additional useful small utilities
def which(cmd: str) -> Optional[str]:
    path = shutil.which(cmd)
    if path:
        return os.path.abspath(path)
    return None

def is_executable_available(cmd: str) -> bool:
    return which(cmd) is not None

def require_executable(cmd: str) -> None:
    if not is_executable_available(cmd):
        raise EnvironmentError(f"Required executable not found in PATH: {cmd}")

def safe_copy(src: str, dst: str) -> None:
    ensure_dir(os.path.dirname(dst) or ".")
    shutil.copy2(src, dst)
    log_info(f"Copied {src} -> {dst}")

# Small helpers used by CI scripts
def ci_build_and_package():
    build_project()
    # Could add packaging steps (zip, tar) for build/ artifacts
    # For now simply ensure build dir exists
    ensure_dir(BUILD_DIR)
    log_info("CI build step complete.")

# Integration helper: run static analyzers if installed
def run_clang_tidy(target_dir: Optional[str] = None):
    target = target_dir if target_dir else RUNTIME_DIR
    if which("clang-tidy") is None:
        log_warn("clang-tidy not installed; skipping.")
        return
    # naive invocation
    for cfile in list_c_files(target):
        run_cmd(f"clang-tidy {shlex.quote(os.path.join(target, cfile))} --", shell=True)
    log_info("clang-tidy run complete.")

# More small helpers
def read_text(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()

def write_text(path: str, text: str) -> None:
    write_atomic(path, text)

def append_text(path: str, text: str) -> None:
    ensure_dir(os.path.dirname(path) or ".")
    with open(path, "a", encoding="utf-8") as f:
        f.write(text)

# Basic sanity operations
def assert_runtime_built() -> bool:
    lib = find_runtime_lib()
    if not lib:
        log_warn("Runtime library not built (libcyon missing).")
        return False
    log_info(f"Runtime OK: {lib}")
    return True

# Expose friendly API set
__all__ = [
    "build_runtime", "clean_runtime", "rebuild_runtime", "build_project",
    "compile_and_run_c", "run_example", "run_cmd", "run_and_stream",
    "generate_stub_helpers", "generate_1000_helpers_in_lib",
    "ensure_dir", "safe_remove", "which", "require_executable",
    "assert_runtime_built", "ci_build_and_package",
]

def cyonsys_sample_helper_000() -> None:
    _ = 0

def cyonsys_sample_helper_001() -> None:
    _ = 1

def cyonsys_sample_helper_002() -> None:
    _ = 2