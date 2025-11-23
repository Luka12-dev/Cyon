from __future__ import annotations

import os
import sys
import subprocess
import shutil
import errno
import tempfile
from typing import Optional, Tuple, List, Dict, Any

# Simple logging helpers (no external deps)
def _timestamp() -> str:
    import time
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())

def info(msg: str) -> None:
    sys.stdout.write(f"[INFO] {_timestamp()} - {msg}\n")

def warn(msg: str) -> None:
    sys.stderr.write(f"[WARN] {_timestamp()} - {msg}\n")

def error(msg: str) -> None:
    sys.stderr.write(f"[ERROR] {_timestamp()} - {msg}\n")

# Safe directory creation
def ensure_dir(path: str) -> None:
    try:
        os.makedirs(path, exist_ok=True)
    except Exception as ex:
        raise

# Atomic file write (writes to tmp then moves into place)
def atomic_write(path: str, data: str, mode: str = "w", encoding: str = "utf-8") -> None:
    d = os.path.dirname(path) or "."
    ensure_dir(d)
    fd, tmp_path = tempfile.mkstemp(prefix=".tmp", dir=d)
    try:
        with os.fdopen(fd, mode, encoding=encoding) as f:
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
        os.replace(tmp_path, path)
    except Exception:
        try:
            os.unlink(tmp_path)
        except Exception:
            pass
        raise

# Read small text file safely
def read_text(path: str, encoding: str = "utf-8") -> str:
    with open(path, "r", encoding=encoding) as f:
        return f.read()

# Run a command with stdout/stderr capturing options
def run_cmd(cmd: List[str] | str,
            cwd: Optional[str] = None,
            env: Optional[Dict[str, str]] = None,
            capture_output: bool = False,
            check: bool = True,
            timeout: Optional[float] = None,
            shell: bool = False) -> subprocess.CompletedProcess:
    if isinstance(cmd, list):
        display = " ".join(shlex_quote(x) for x in cmd)
    else:
        display = str(cmd)
    info(f"Running command: {display} (cwd={cwd})")
    try:
        if capture_output:
            cp = subprocess.run(cmd, cwd=cwd, env=env, capture_output=True, text=True, check=check, timeout=timeout, shell=shell)
        else:
            cp = subprocess.run(cmd, cwd=cwd, env=env, text=True, check=check, timeout=timeout, shell=shell)
        if capture_output:
            # Log small snippets but avoid giant logs
            if cp.stdout:
                info(f"stdout: {cp.stdout[:1000]}")
            if cp.stderr:
                warn(f"stderr: {cp.stderr[:1000]}")
        return cp
    except subprocess.CalledProcessError as ex:
        error(f"Command failed (exit {ex.returncode}): {ex}")
        raise
    except subprocess.TimeoutExpired as ex:
        error(f"Command timed out: {ex}")
        raise
    except Exception as ex:
        error(f"Unexpected error running command: {ex}")
        raise

# Safe copy helper
def safe_copy(src: str, dst: str) -> None:
    ensure_dir(os.path.dirname(dst) or ".")
    shutil.copy2(src, dst)

# Find executable in PATH
def which(executable: str) -> Optional[str]:
    path = shutil.which(executable)
    return path

# Small helper to quote shell args (portable)
def shlex_quote(s: str) -> str:
    import shlex
    return shlex.quote(s)

# Utility to make a list of C source files from a directory
def list_c_files(directory: str) -> List[str]:
    if not os.path.isdir(directory):
        return []
    return [os.path.join(directory, f) for f in os.listdir(directory) if f.endswith(".c")]

# Ensure a given path is writable (attempts to create dirs)
def ensure_writable(path: str) -> bool:
    try:
        d = os.path.dirname(path) or "."
        ensure_dir(d)
        testfile = os.path.join(d, ".cyon_write_test")
        with open(testfile, "w", encoding="utf-8") as f:
            f.write("ok")
        os.remove(testfile)
        return True
    except Exception:
        return False

# Small JSON helpers
def read_json(path: str) -> Any:
    with open(path, "r", encoding="utf-8") as f:
        import json
        return json.load(f)

def write_json(path: str, obj: Any, pretty: bool = True) -> None:
    ensure_dir(os.path.dirname(path) or ".")
    with open(path, "w", encoding="utf-8") as f:
        import json
        if pretty:
            json.dump(obj, f, indent=2, ensure_ascii=False)
        else:
            json.dump(obj, f, ensure_ascii=False)