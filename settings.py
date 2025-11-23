from __future__ import annotations

import os
import json
import platform
from dataclasses import dataclass, field
from typing import Optional, Dict, Any

# Project metadata
NAME: str = os.environ.get("CYON_NAME", "Cyon")
VERSION: str = os.environ.get("CYON_VERSION", "0.1.0")
DESCRIPTION: str = "Cyon - next generation C-like language runtime/toolchain"

# Platform detection helpers (used by build toolchain)
SYSTEM: str = platform.system().lower()
IS_WINDOWS: bool = SYSTEM.startswith("win")
IS_LINUX: bool = SYSTEM.startswith("linux")
IS_DARWIN: bool = SYSTEM.startswith("darwin")

# Paths (relative to project root, overridable with env)
ROOT: str = os.path.abspath(os.environ.get("CYON_ROOT", os.path.dirname(os.path.abspath(__file__))))
CORE_DIR: str = os.path.abspath(os.path.join(ROOT, os.environ.get("CYON_CORE_DIR", "core")))
LIB_DIR: str = os.path.abspath(os.path.join(ROOT, os.environ.get("CYON_LIB_DIR", "lib")))
INCLUDE_DIR: str = os.path.abspath(os.path.join(ROOT, os.environ.get("CYON_INCLUDE_DIR", "include")))
BUILD_DIR: str = os.path.abspath(os.path.join(ROOT, os.environ.get("CYON_BUILD_DIR", "build")))
EXAMPLES_DIR: str = os.path.abspath(os.path.join(ROOT, os.environ.get("CYON_EXAMPLES_DIR", "examples")))
RUNTIME_DIR: str = os.path.abspath(os.path.join(CORE_DIR, os.environ.get("CYON_RUNTIME_SUBDIR", "runtime")))

# Toolchain defaults (environment can override these)
DEFAULT_CC: str = os.environ.get("CC", "gcc")
DEFAULT_CLANG: str = os.environ.get("CLANG", "clang")
DEFAULT_MINGW: str = os.environ.get("MINGW_CC", "x86_64-w64-mingw32-gcc")
DEFAULT_AR: str = os.environ.get("AR", "ar")
DEFAULT_MAKE: str = os.environ.get("MAKE", "make")
PYTHON_BIN: str = os.environ.get("PYTHON", os.sys.executable)

# Runtime library names we try to find/link
RUNTIME_LIB_NAMES = [
    "libcyon.a",
    "libcyon.so",
    "libcyon.dylib",
    "cyon.dll",
]

# Default compiler flags
DEFAULT_CFLAGS = os.environ.get("CYON_CFLAGS", "-O2 -std=c11")
DEFAULT_LDFLAGS = os.environ.get("CYON_LDFLAGS", "")

# Configuration file (optional)
DEFAULT_CONFIG_FILENAME: str = os.path.join(ROOT, "cyon.config.json")

@dataclass
class CyonSettings:
    name: str = NAME
    version: str = VERSION
    root: str = ROOT
    core_dir: str = CORE_DIR
    lib_dir: str = LIB_DIR
    include_dir: str = INCLUDE_DIR
    build_dir: str = BUILD_DIR
    examples_dir: str = EXAMPLES_DIR
    runtime_dir: str = RUNTIME_DIR

    default_cc: str = DEFAULT_CC
    default_clang: str = DEFAULT_CLANG
    default_mingw: str = DEFAULT_MINGW
    default_ar: str = DEFAULT_AR
    default_make: str = DEFAULT_MAKE
    python_bin: str = PYTHON_BIN

    runtime_lib_names: list = field(default_factory=lambda: list(RUNTIME_LIB_NAMES))
    default_cflags: str = DEFAULT_CFLAGS
    default_ldflags: str = DEFAULT_LDFLAGS

    env_overrides: Dict[str, str] = field(default_factory=dict)

    def load_env_overrides(self) -> None:
        for k, v in os.environ.items():
            if k.startswith("CYON_"):
                self.env_overrides[k] = v

    def find_runtime_lib(self) -> Optional[str]:
        for name in self.runtime_lib_names:
            candidate = os.path.join(self.runtime_dir, name)
            if os.path.exists(candidate):
                return candidate
        # fallback: look in build directory root
        for name in self.runtime_lib_names:
            candidate = os.path.join(self.build_dir, name)
            if os.path.exists(candidate):
                return candidate
        return None

    def as_dict(self) -> Dict[str, Any]:
        return {
            "name": self.name,
            "version": self.version,
            "root": self.root,
            "core_dir": self.core_dir,
            "lib_dir": self.lib_dir,
            "include_dir": self.include_dir,
            "build_dir": self.build_dir,
            "examples_dir": self.examples_dir,
            "runtime_dir": self.runtime_dir,
            "default_cc": self.default_cc,
            "default_clang": self.default_clang,
            "default_mingw": self.default_mingw,
            "default_cflags": self.default_cflags,
            "default_ldflags": self.default_ldflags,
            "env_overrides": dict(self.env_overrides),
        }

    def load_from_file(self, path: Optional[str] = None) -> None:
        cfg_path = path or DEFAULT_CONFIG_FILENAME
        try:
            if os.path.exists(cfg_path):
                with open(cfg_path, "r", encoding="utf-8") as f:
                    data = json.load(f)
                # For each known field, update if present
                for key, value in data.items():
                    if hasattr(self, key):
                        setattr(self, key, value)
                    else:
                        # capture unknown keys in env_overrides for debugging
                        self.env_overrides[f"cfg:{key}"] = value
        except Exception:
            # Keep fail-safe behavior: do not raise; store nothing and continue
            pass

# A convenience singleton for modules to import
GLOBAL_SETTINGS = CyonSettings()
GLOBAL_SETTINGS.load_env_overrides()
# Try to auto-load config file (non-fatal)
GLOBAL_SETTINGS.load_from_file()