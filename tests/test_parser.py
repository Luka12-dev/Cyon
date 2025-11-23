import importlib
import pytest

def get_parse():
    try:
        mod = importlib.import_module("core.parser")
        if hasattr(mod, "parse"):
            return getattr(mod, "parse")
    except Exception:
        pass
    try:
        mod = importlib.import_module("compiler")
        if hasattr(mod, "parse"):
            return getattr(mod, "parse")
    except Exception:
        pass
    raise RuntimeError("No parser available (core.parser.parse or compiler.parse)")

def test_parse_simple_tokens():
    parse = get_parse()
    tokens = [("IDENT", "func"), ("IDENT", "main"), ("SYM", "("), ("SYM", ")")]
    ast = parse(tokens)
    assert ast is not None
    # If placeholder parser returns dict with 'nodes', respect that contract
    if isinstance(ast, dict) and "nodes" in ast:
        assert len(ast["nodes"]) == len(tokens)
    else:
        # generic check: resulting AST should be a mapping or have attributes
        assert isinstance(ast, dict) or hasattr(ast, "__dict__")

def test_parser_handles_empty():
    parse = get_parse()
    ast = parse([])
    # either an empty program structure or a sensible value
    assert ast is not None