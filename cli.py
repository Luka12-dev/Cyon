from __future__ import annotations

import argparse
import sys
import os
from typing import List, Optional

try:
    import compiler as compiler_module
except Exception:  # pragma: no cover - defensive
    compiler_module = None

ROOT = os.path.abspath(os.path.dirname(__file__))

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="cyon",
        description="Lightweight CLI wrapper for the CYON toolchain (delegates to compiler.py)",
    )

    # Most common direct actions are build/run; other options forwarded to compiler
    sub = p.add_subparsers(dest="action", help="Primary action")

    # Run action: compile + run
    s_run = sub.add_parser("run", help="Compile and run a .cyon source file")
    s_run.add_argument("source", help=".cyon source file to run")
    s_run.add_argument("--root", action="store_true", help="attempt to run with elevated privileges")

    # Build action: create an executable
    s_build = sub.add_parser("build", help="Build .cyon into native executable")
    s_build.add_argument("source", help=".cyon source file to build")
    s_build.add_argument("--target", choices=["native", "windows", "linux", "macos", "all"], default="native", help="target platform")
    s_build.add_argument("--output", help="output path (file or directory under build/)")
    s_build.add_argument("--run", action="store_true", help="run binary after successful build")
    s_build.add_argument("--root", action="store_true", help="attempt to build with elevated privileges")

    # Compile-only: generate C
    s_compile = sub.add_parser("compile", help="Generate C from .cyon without building binary")
    s_compile.add_argument("source", help=".cyon source file to compile to C")
    s_compile.add_argument("--emit-c", action="store_true", help="write C output to build/ and exit")

    # Utility commands
    sub.add_parser("init", help="Create project skeleton directories (core/, lib/, examples/, include/, build/)")
    sub.add_parser("clean", help="Remove build artifacts (build/) and run runtime clean if available")
    sub.add_parser("info", help="Show project info and environment diagnostics")

    # Pass-through: accept unknown extra args and forward them to compiler. This keeps
    # the wrapper flexible while allowing concise common commands here.
    return p

def forward_to_compiler(argv: Optional[List[str]] = None) -> int:
    argv = list(argv or sys.argv[1:])

    # If we have the module loaded and it exposes `main`, call it directly.
    if compiler_module is not None and hasattr(compiler_module, "main"):
        try:
            return int(compiler_module.main(argv))
        except SystemExit as sx:
            return int(getattr(sx, "code", 0) or 0)
        except Exception as ex:
            print(f"[cli] Error while invoking compiler.main(): {ex}")
            return 1

    # Fallback: run compiler.py as a subprocess
    compiler_script = os.path.join(ROOT, "compiler.py")
    if os.path.exists(compiler_script):
        cmd = [sys.executable, compiler_script] + argv
        rc = os.spawnvpe(os.P_WAIT, sys.executable, cmd, os.environ)
        return int(rc)

    print("[cli] Could not locate compiler module or compiler.py script. Ensure compiler.py is in project root.")
    return 2


def run(argv: Optional[List[str]] = None) -> int:
    parser = build_parser()
    parsed, extras = parser.parse_known_args(argv)

    # Map wrapper actions to compiler invocations
    if parsed.action in (None, "info"):
        # default: show info if nothing given
        return forward_to_compiler(["info"])

    if parsed.action == "init":
        return forward_to_compiler(["init"])  # delegates to compiler

    if parsed.action == "clean":
        return forward_to_compiler(["clean"])

    if parsed.action == "run":
        # Compose args to pass to compiler.run
        cmd = ["run", parsed.source]
        if getattr(parsed, "root", False):
            cmd.append("--root")
        cmd.extend(extras)
        return forward_to_compiler(cmd)

    if parsed.action == "build":
        cmd = ["build", parsed.source]
        if parsed.target:
            cmd.extend(["--target", parsed.target])
        if parsed.output:
            cmd.extend(["--output", parsed.output])
        if parsed.run:
            cmd.append("--run")
        if getattr(parsed, "root", False):
            cmd.append("--root")
        cmd.extend(extras)
        return forward_to_compiler(cmd)

    if parsed.action == "compile":
        cmd = ["build", parsed.source, "--emit", "c"]
        if getattr(parsed, "emit_c", False):
            cmd = ["build", parsed.source, "--emit", "c"]
        cmd.extend(extras)
        return forward_to_compiler(cmd)

    # Unknown action: hand off raw to compiler
    raw = [parsed.action] + (argv or [])
    return forward_to_compiler(raw)