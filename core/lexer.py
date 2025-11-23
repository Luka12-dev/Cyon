from __future__ import annotations

import re
import sys
import traceback
from dataclasses import dataclass
from typing import List, Optional, Tuple, Iterator, Dict, Any

class TokenType:
    # Structural
    EOF = "EOF"
    NEWLINE = "NEWLINE"
    INDENT = "INDENT"
    DEDENT = "DEDENT"

    # Literals
    IDENT = "IDENT"
    INT = "INT"
    FLOAT = "FLOAT"
    STRING = "STRING"
    CHAR = "CHAR"

    # Keywords (a subset for v0.1)
    FUNC = "FUNC"
    LET = "LET"
    IF = "IF"
    ELSE = "ELSE"
    FOR = "FOR"
    WHILE = "WHILE"
    RETURN = "RETURN"
    BREAK = "BREAK"
    CONTINUE = "CONTINUE"
    IMPORT = "IMPORT"
    TRUE = "TRUE"
    FALSE = "FALSE"
    NULL = "NULL"

    # Operators
    PLUS = "+"
    MINUS = "-"
    STAR = "*"
    SLASH = "/"
    MOD = "%"
    EQ = "="
    EQEQ = "=="
    NE = "!="
    LT = "<"
    GT = ">"
    LTE = "<="
    GTE = ">="
    BANG = "!"
    AND = "&&"
    OR = "||"
    ARROW = "->"

    # Punctuation
    LPAREN = "("
    RPAREN = ")"
    LBRACE = "{"
    RBRACE = "}"
    LBRACKET = "["
    RBRACKET = "]"
    COMMA = ","
    DOT = "."
    COLON = ":"
    SEMICOLON = ";"

    # Misc
    COMMENT = "COMMENT"
    UNKNOWN = "UNKNOWN"

    @classmethod
    def keywords(cls) -> Dict[str, str]:
        return {
            "func": cls.FUNC,
            "let": cls.LET,
            "if": cls.IF,
            "else": cls.ELSE,
            "for": cls.FOR,
            "while": cls.WHILE,
            "return": cls.RETURN,
            "break": cls.BREAK,
            "continue": cls.CONTINUE,
            "import": cls.IMPORT,
            "true": cls.TRUE,
            "false": cls.FALSE,
            "null": cls.NULL,
        }

# Token dataclass

@dataclass
class Position:
    index: int
    line: int
    column: int

    def copy(self) -> "Position":
        return Position(self.index, self.line, self.column)

@dataclass
class Token:
    type: str
    value: Optional[Any]
    start: Position
    end: Position

    def __repr__(self) -> str:
        v = repr(self.value) if self.value is not None else ""
        return f"Token({self.type}, {v}, {self.start.line}:{self.start.column})"

# Exceptions

class LexerError(Exception):
    def __init__(self, message: str, position: Optional[Position] = None):
        super().__init__(message)
        self.position = position

# Lexer implementation

class Lexer:
    DEFAULT_OPERATORS = {
        "==": TokenType.EQEQ,
        "!=": TokenType.NE,
        "<=": TokenType.LTE,
        ">=": TokenType.GTE,
        "&&": TokenType.AND,
        "||": TokenType.OR,
        "->": TokenType.ARROW,
        "+": TokenType.PLUS,
        "-": TokenType.MINUS,
        "*": TokenType.STAR,
        "/": TokenType.SLASH,
        "%": TokenType.MOD,
        "=": TokenType.EQ,
        "<": TokenType.LT,
        ">": TokenType.GT,
        "!": TokenType.BANG,
        "(": TokenType.LPAREN,
        ")": TokenType.RPAREN,
        "{": TokenType.LBRACE,
        "}": TokenType.RBRACE,
        "[": TokenType.LBRACKET,
        "]": TokenType.RBRACKET,
        ",": TokenType.COMMA,
        ".": TokenType.DOT,
        ":": TokenType.COLON,
        ";": TokenType.SEMICOLON,
    }

    token_specification = [
        ("WHITESPACE", r"[ \t]+"),
        ("NEWLINE", r"\n"),
        ("COMMENT_SL", r"//.*"),
        ("COMMENT_ML", r"/\*(?:.|\n)*?\*/"),
        ("FLOAT", r"\d+\.\d+([eE][+-]?\d+)?"),
        ("INT", r"\d+"),
        ("IDENT", r"[A-Za-z_][A-Za-z0-9_]*"),
        ("STRING", r'"(?:\\.|[^\\"])*"'),
        ("CHAR", r"'(?:\\.|[^\\'])'"),
        ("OP", r"==|!=|<=|>=|&&|\|\||->|[+\-*/%!=<>\(\)\{\}\[\],.:;\\]")
    ]

    master_regex = re.compile(
        "|".join(f"(?P<{name}>{pattern})" for name, pattern in token_specification),
        flags=re.MULTILINE,
    )

    def __init__(self, text: str, filename: str = "<string>", *, collect_errors: bool = True):
        self.text = text
        self.filename = filename
        self.collect_errors = collect_errors
        self.errors: List[LexerError] = []
        self.pos = Position(0, 1, 1)
        self._index = 0
        self._line_start = 0
        self._matches = list(self.master_regex.finditer(text))
        # pointer in matches
        self._mi = 0
        # indentation stack if needed in future
        self._indent_stack: List[int] = [0]

    def _advance_match(self, match: re.Match) -> None:
        start = match.start()
        end = match.end()
        # update index
        self.pos.index = end
        # calculate line and column
        # count newlines between _index and end to update line number
        segment = self.text[self._index:end]
        newlines = segment.count("\n")
        if newlines:
            # last newline position
            last_nl = segment.rfind("\n")
            self.pos.line += newlines
            self.pos.column = end - (self._index + last_nl)
        else:
            self.pos.column += end - self._index
        self._index = end

    def _current_slice(self, start: int, end: int) -> str:
        return self.text[start:end]

    def tokenize(self) -> Iterator[Token]:
        # iterate through regex matches and produce tokens
        last_pos = self.pos.copy()
        for match in self._matches:
            kind = match.lastgroup
            value = match.group()
            start_index = match.start()
            end_index = match.end()

            start_pos = Position(start_index, last_pos.line, last_pos.column)

            if kind == "WHITESPACE":
                # consume whitespace, update positions
                # do not emit any token for usual spaces
                self._advance_match(match)
                last_pos = self.pos.copy()
                continue

            if kind == "NEWLINE":
                # emit NEWLINE token
                end_pos = Position(end_index, last_pos.line + 1, 1)
                token = Token(TokenType.NEWLINE, "\\n", start_pos, end_pos)
                yield token
                self._advance_match(match)
                last_pos = self.pos.copy()
                continue

            if kind == "COMMENT_SL":
                # single-line comment: emit as COMMENT or ignore
                comment_text = value[2:]
                end_pos = Position(end_index, start_pos.line, start_pos.column + len(value))
                token = Token(TokenType.COMMENT, comment_text, start_pos, end_pos)
                # we may choose to yield comments for tooling; here we do
                yield token
                self._advance_match(match)
                last_pos = self.pos.copy()
                continue

            if kind == "COMMENT_ML":
                # multi-line comment - may contain newlines
                comment_text = value[2:-2]
                # calculate line/column after consuming
                nl_count = value.count("\n")
                if nl_count:
                    # line calculation
                    end_line = last_pos.line + nl_count
                    after_last_nl = value.rfind("\n")
                    end_col = len(value) - after_last_nl
                else:
                    end_line = last_pos.line
                    end_col = last_pos.column + len(value)
                end_pos = Position(end_index, end_line, end_col)
                token = Token(TokenType.COMMENT, comment_text, start_pos, end_pos)
                yield token
                self._advance_match(match)
                last_pos = self.pos.copy()
                continue

            if kind == "IDENT":
                # check for keywords
                lower = value
                kw_map = TokenType.keywords()
                if lower in kw_map:
                    ttype = kw_map[lower]
                    token = Token(ttype, value, start_pos, Position(end_index, last_pos.line, last_pos.column + len(value)))
                    yield token
                else:
                    token = Token(TokenType.IDENT, value, start_pos, Position(end_index, last_pos.line, last_pos.column + len(value)))
                    yield token
                self._advance_match(match)
                last_pos = self.pos.copy()
                continue

            if kind == "INT":
                token = Token(TokenType.INT, int(value), start_pos, Position(end_index, last_pos.line, last_pos.column + len(value)))
                yield token
                self._advance_match(match)
                last_pos = self.pos.copy()
                continue

            if kind == "FLOAT":
                token = Token(TokenType.FLOAT, float(value), start_pos, Position(end_index, last_pos.line, last_pos.column + len(value)))
                yield token
                self._advance_match(match)
                last_pos = self.pos.copy()
                continue

            if kind == "STRING":
                # unescape string literal
                raw = value[1:-1]
                unescaped = self._unescape_string(raw)
                token = Token(TokenType.STRING, unescaped, start_pos, Position(end_index, last_pos.line, last_pos.column + len(value)))
                yield token
                self._advance_match(match)
                last_pos = self.pos.copy()
                continue

            if kind == "CHAR":
                raw = value[1:-1]
                unescaped = self._unescape_string(raw)
                token = Token(TokenType.CHAR, unescaped, start_pos, Position(end_index, last_pos.line, last_pos.column + len(value)))
                yield token
                self._advance_match(match)
                last_pos = self.pos.copy()
                continue

            if kind == "OP":
                # match against operator table for multi-char ops
                op_value = value
                # direct mapping
                mapped = self.DEFAULT_OPERATORS.get(op_value, None)
                if mapped is not None:
                    token = Token(mapped, op_value, start_pos, Position(end_index, last_pos.line, last_pos.column + len(value)))
                    yield token
                else:
                    # unknown operator -> UNKNOWN
                    token = Token(TokenType.UNKNOWN, op_value, start_pos, Position(end_index, last_pos.line, last_pos.column + len(value)))
                    yield token
                self._advance_match(match)
                last_pos = self.pos.copy()
                continue

            # fallback (should not happen)
            self._advance_match(match)
            last_pos = self.pos.copy()

        # finally, emit EOF
        eof_pos = Position(len(self.text), last_pos.line, last_pos.column)
        yield Token(TokenType.EOF, None, eof_pos, eof_pos)

    # Utilities

    def _unescape_string(self, s: str) -> str:
        # Use a simple state machine for escapes for clarity and extensibility
        out_chars: List[str] = []
        i = 0
        L = len(s)
        while i < L:
            ch = s[i]
            if ch != "\\":
                out_chars.append(ch)
                i += 1
                continue
            # handle escape
            i += 1
            if i >= L:
                out_chars.append("\\")
                break
            esc = s[i]
            if esc == "n":
                out_chars.append("\n")
            elif esc == "t":
                out_chars.append("\t")
            elif esc == "r":
                out_chars.append("\r")
            elif esc == "'":
                out_chars.append("'")
            elif esc == '"':
                out_chars.append('"')
            elif esc == "\\":
                out_chars.append("\\")
            elif esc == "u":
                # unicode escape \uXXXX
                hex_digits = s[i+1:i+5]
                try:
                    code_point = int(hex_digits, 16)
                    out_chars.append(chr(code_point))
                    i += 4
                except Exception:
                    # invalid escape, keep raw
                    out_chars.append('\\u')
            else:
                # unknown escape, keep literally
                out_chars.append(esc)
            i += 1
        return "".join(out_chars)

def tokenize_text(text: str, filename: str = "<string>") -> List[Token]:
    lexer = Lexer(text, filename)
    return list(lexer.tokenize())

EXAMPLES: Dict[str, str] = {
    "hello": r'''func main() {
    print("Hello, world!")
}
''',

    "condition_basic": r'''func main() {
    let x = 10
    if x > 5:
        print("Big number")
    else:
        print("Small number")
}
''',

    "loops": r'''func main() {
    let i = 0
    while i < 10 {
        print(i)
        i = i + 1
    }
}
''',

    "comments": r'''// single line comment
func main() {
    /*
       multi-line
       comment
    */
    print("after comments")
}
''',

    "strings_and_escapes": r'''func main() {
    print("Line1\nLine2\tTabbed\\Backslash\"Quote\'")
    let c = 'a'
}
''',

    "operators": r'''func main() {
    let a = 10
    let b = 20
    if a == b || a != b && (a < b) {
        print("ops")
    }
}
''',

    # a long example with many constructs concatenated to make tests long
    "big_example": """
// Big example combining constructs, loops, function calls, arrays and math
func fact(n) {
    if n <= 1:
        return 1
    else:
        return n * fact(n - 1)
}

func main() {
    print("Factorial of 5: ")
    let res = fact(5)
    print(res)

    let arr = [1, 2, 3, 4, 5]
    for i in arr {
        print(i)
    }

    let sum = 0
    let idx = 0
    while idx < 5 {
        sum = sum + arr[idx]
        idx = idx + 1
    }
    print(sum)
}
"""
}

def _print_tokens(tokens: List[Token]):
    for t in tokens:
        print(t)


def run_quick_tests():
    print("Running quick lexer tests for Cyon (this is verbose)\n")
    for name, src in EXAMPLES.items():
        print(f"--- Example: {name} ---")
        try:
            toks = tokenize_text(src, filename=f"example:{name}")
            _print_tokens(toks)
        except Exception as e:
            print(f"Error while tokenizing example '{name}': {e}")
            traceback.print_exc()
        print("\n")

class TokenStream:
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.index = 0

    def peek(self, offset: int = 0) -> Token:
        idx = self.index + offset
        if idx >= len(self.tokens):
            return self.tokens[-1]
        return self.tokens[idx]

    def next(self) -> Token:
        if self.index >= len(self.tokens):
            return self.tokens[-1]
        t = self.tokens[self.index]
        self.index += 1
        return t

    def expect(self, ttype: str) -> Token:
        t = self.next()
        if t.type != ttype:
            raise LexerError(f"Expected token {ttype}, got {t.type} at {t.start}", t.start)
        return t

    def match(self, ttype: str) -> bool:
        if self.peek().type == ttype:
            self.next()
            return True
        return False

    def rewind(self, count: int = 1):
        self.index = max(0, self.index - count)

def is_keyword_token(t: Token) -> bool:
    return t.type in TokenType.keywords().values()

def is_literal_token(t: Token) -> bool:
    return t.type in {TokenType.INT, TokenType.FLOAT, TokenType.STRING, TokenType.CHAR}

def token_to_source_slice(src: str, token: Token) -> str:
    s = token.start.index
    e = token.end.index
    return src[s:e]

def pretty_position(pos: Position) -> str:
    return f"line {pos.line}, col {pos.column}"

def human_error_context(src: str, pos: Position, window: int = 40) -> str:
    # provide a slice of the source around pos.index for diagnostics
    s = max(0, pos.index - window)
    e = min(len(src), pos.index + window)
    snippet = src[s:e]
    # highlight the approximate position with a caret
    caret_pos = pos.index - s
    caret_line = " " * caret_pos + "^"
    return f"...{snippet}...\n{caret_line}"

def cli_main(argv: Optional[List[str]] = None):
    argv = argv if argv is not None else sys.argv[1:]
    if not argv:
        print("Usage: python core/lexer.py <file.cyon> [--dump]")
        print("Runs lexer and prints tokens. Use --dump to display full token values.")
        return 0

    filename = argv[0]
    dump = "--dump" in argv
    try:
        with open(filename, "r", encoding="utf-8") as f:
            src = f.read()
    except Exception as e:
        print(f"Failed to read file {filename}: {e}")
        return 2

    print(f"Tokenizing {filename}...\n")
    lex = Lexer(src, filename=filename)
    tokens = list(lex.tokenize())
    for t in tokens:
        if t.type == TokenType.COMMENT and not dump:
            # print a short comment marker
            print(f"{t.type:<10} at {pretty_position(t.start)}")
        else:
            if dump:
                print(repr(t))
            else:
                print(f"{t.type:<10} {t.value!s:<20} {pretty_position(t.start)}")
    if lex.errors:
        print("\nErrors:")
        for err in lex.errors:
            print(err)
    return 0