import importlib
import os
from pathlib import Path

import pytest

PROJECT_ROOT = Path(__file__).resolve().parents[1]
EXAMPLES_DIR = PROJECT_ROOT / "examples"
BUILD_DIR = PROJECT_ROOT / "build"

def test_examples_directory_exists():
    assert EXAMPLES_DIR.exists() and EXAMPLES_DIR.is_dir(), "examples/ directory must exist"

def test_at_least_one_cyon_example():
    files = list(EXAMPLES_DIR.glob("*.cyon"))
    assert len(files) > 0, "No .cyon example files found in examples/"

def test_compile_example_if_compiler_available(tmp_path):
    try:
        compiler = importlib.import_module("compiler")
    except Exception:
        pytest.skip("compiler module not importable; skipping compile test")

    example = next(EXAMPLES_DIR.glob("*.cyon"), None)
    if example is None:
        pytest.skip("no example files to test")

    # Use compiler.cmd_build (if available) or compiler.main run through args
    if hasattr(compiler, "cmd_build"):
        out = compiler.cmd_build(str(example), target="native")
        assert out is not None
    elif hasattr(compiler, "main"):
        # call as subprocess alternative would be safer, but attempt direct call
        res = compiler.main(["build", str(example)])
        assert res == 0 or res is None
    else:
        pytest.skip("compiler does not expose build API; skipped")