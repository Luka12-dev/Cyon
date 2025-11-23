import importlib
import inspect

import pytest


def get_tokenize():
    # Try to import the real lexer; fallback to compiler.lex
    try:
        mod = importlib.import_module("core.lexer")
        if hasattr(mod, "tokenize"):
            return getattr(mod, "tokenize")
    except Exception:
        pass
    try:
        mod = importlib.import_module("compiler")
        if hasattr(mod, "lex"):
            return getattr(mod, "lex")
    except Exception:
        pass
    raise RuntimeError("No tokenizer available (core.lexer.tokenize or compiler.lex)")


def test_basic_tokens_present():
    tokenize = get_tokenize()
    src = 'func main() { print("Hello", 123); }'
    tokens = tokenize(src)
    # tokens expected to be a sequence of tuples or similar
    assert tokens, "Tokenizer produced no tokens"
    # ensure common identifiers appear
    flat_values = []
    for t in tokens:
        # t can be (type, value) or a simple string
        if isinstance(t, (list, tuple)) and len(t) >= 2:
            flat_values.append(str(t[1]))
        else:
            flat_values.append(str(t))
    assert "func" in flat_values or "FUNC" in flat_values
    assert "main" in flat_values
    assert "print" in flat_values

def test_numbers_and_symbols():
    tokenize = get_tokenize()
    src = "let x = 42; if (x > 0) { x = x - 1; }"
    tokens = tokenize(src)
    # Ensure at least one numeric token exists
    has_num = any((isinstance(t, (list, tuple)) and str(t[1]).isdigit()) or (isinstance(t, str) and t.isdigit()) for t in tokens)
    assert has_num, "Tokenizer did not emit a numeric token for digits"

def test_tokenizer_return_shape():
    tokenize = get_tokenize()
    src = "\n"
    tokens = tokenize(src)
    assert hasattr(tokens, "__iter__"), "Tokenizer should return an iterable"