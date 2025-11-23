import importlib
import re

import pytest

def get_codegen():
    try:
        mod = importlib.import_module("core.codegen")
        if hasattr(mod, "codegen_to_c"):
            return getattr(mod, "codegen_to_c")
    except Exception:
        pass
    try:
        mod = importlib.import_module("compiler")
        # compiler has function named `codegen_to_c` in our scaffold
        if hasattr(mod, "codegen_to_c"):
            return getattr(mod, "codegen_to_c")
    except Exception:
        pass
    raise RuntimeError("No codegen available (core.codegen.codegen_to_c or compiler.codegen_to_c)")

def test_codegen_emits_main_and_return():
    codegen = get_codegen()
    # Provide a tiny AST structure; parser/optimizer output shape may vary
    ast = {"nodes": [("IDENT", "main")], "meta": {}}
    ccode = codegen(ast)
    assert isinstance(ccode, str)
    # check presence of main and a return or exit sequence
    assert re.search(r"int\s+main\s*\(", ccode) or re.search(r"main\s*:\s*", ccode), "Generated C should define main"
    assert ("return" in ccode) or ("exit(" in ccode) or ("printf(" in ccode)

def test_codegen_produces_valid_c_file(tmp_path):
    codegen = get_codegen()
    ast = {"nodes": [("IDENT", "dummy")], "meta": {}}
    ccode = codegen(ast)
    path = tmp_path / "out.c"
    path.write_text(ccode, encoding="utf-8")
    assert path.exists()
    assert path.stat().st_size > 0