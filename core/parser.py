from __future__ import annotations

import sys
import traceback
from dataclasses import dataclass, field
from typing import List, Optional, Any, Dict, Iterator, Callable, Union, Tuple

# Local imports - these assume package layout where core/lexer.py
try:
    # relative import within package
    from .lexer import TokenType, Token, TokenStream, Lexer, tokenize_text
except Exception:
    # fallback for direct script execution in some environments
    from core.lexer import TokenType, Token, TokenStream, Lexer, tokenize_text

# Parse exceptions and utilities

class ParseError(Exception):
    def __init__(self, message: str, token: Optional[Token] = None):
        super().__init__(message)
        self.token = token


def pretty_token(t: Optional[Token]) -> str:
    if t is None:
        return "<no-token>"
    return f"{t.type}({t.value}) at {t.start.line}:{t.start.column}"

# AST Node definitions
# We'll use dataclasses for nodes. Nodes are intentionally verbose to aid
# introspection, debugging, and tooling.

@dataclass
class Node:
    """Base AST node."""
    def accept(self, visitor: "ASTVisitor"):
        method_name = "visit_" + self.__class__.__name__
        visitor_fn = getattr(visitor, method_name, visitor.generic_visit)
        return visitor_fn(self)

# Top level program node
@dataclass
class Program(Node):
    body: List[Node] = field(default_factory=list)

# Declarations and statements
@dataclass
class FunctionDecl(Node):
    name: str
    params: List[str]
    body: List[Node]

@dataclass
class ReturnStmt(Node):
    value: Optional[Node]

@dataclass
class VarDecl(Node):
    name: str
    value: Optional[Node]

@dataclass
class IfStmt(Node):
    condition: Node
    then_branch: List[Node]
    else_branch: Optional[List[Node]] = None

@dataclass
class WhileStmt(Node):
    condition: Node
    body: List[Node]

@dataclass
class ForStmt(Node):
    iterator: Optional[str]
    iterable: Optional[Node]
    body: List[Node]

@dataclass
class BreakStmt(Node):
    pass

@dataclass
class ContinueStmt(Node):
    pass

# Expressions
@dataclass
class Literal(Node):
    value: Any

@dataclass
class Identifier(Node):
    name: str

@dataclass
class BinaryOp(Node):
    left: Node
    op: str
    right: Node

@dataclass
class UnaryOp(Node):
    op: str
    operand: Node

@dataclass
class CallExpr(Node):
    callee: Node
    args: List[Node]

@dataclass
class ArrayExpr(Node):
    elements: List[Node]

@dataclass
class IndexExpr(Node):
    collection: Node
    index: Node

# AST Visitor / Transformer utilities

class ASTVisitor:
    """Base visitor with a generic_visit fallback."""
    def generic_visit(self, node: Node):
        # default behavior: walk dataclass fields
        for field_name, field_value in getattr(node, "__dict__", {}).items():
            if isinstance(field_value, list):
                for item in field_value:
                    if isinstance(item, Node):
                        item.accept(self)
            elif isinstance(field_value, Node):
                field_value.accept(self)

class ASTPrinter(ASTVisitor):
    def __init__(self):
        self.indent = 0

    def _w(self, s: str):
        print("  " * self.indent + s)

    def visit_Program(self, node: Program):
        self._w("Program:")
        self.indent += 1
        for stmt in node.body:
            stmt.accept(self)
        self.indent -= 1

    def visit_FunctionDecl(self, node: FunctionDecl):
        self._w(f"Function: {node.name}({', '.join(node.params)})")
        self.indent += 1
        for s in node.body:
            s.accept(self)
        self.indent -= 1

    def visit_VarDecl(self, node: VarDecl):
        self._w(f"VarDecl: {node.name}")
        if node.value:
            self.indent += 1
            node.value.accept(self)
            self.indent -= 1

    def visit_ReturnStmt(self, node: ReturnStmt):
        self._w("Return:")
        if node.value:
            self.indent += 1
            node.value.accept(self)
            self.indent -= 1

    def visit_IfStmt(self, node: IfStmt):
        self._w("If:")
        self.indent += 1
        self._w("Cond:")
        self.indent += 1
        node.condition.accept(self)
        self.indent -= 1
        self._w("Then:")
        self.indent += 1
        for s in node.then_branch:
            s.accept(self)
        self.indent -= 1
        if node.else_branch:
            self._w("Else:")
            self.indent += 1
            for s in node.else_branch:
                s.accept(self)
            self.indent -= 1
        self.indent -= 1

    def visit_WhileStmt(self, node: WhileStmt):
        self._w("While:")
        self.indent += 1
        self._w("Cond:")
        self.indent += 1
        node.condition.accept(self)
        self.indent -= 1
        self._w("Body:")
        self.indent += 1
        for s in node.body:
            s.accept(self)
        self.indent -= 2

    def visit_CallExpr(self, node: CallExpr):
        self._w("Call:")
        self.indent += 1
        self._w("Callee:")
        self.indent += 1
        node.callee.accept(self)
        self.indent -= 1
        if node.args:
            self._w("Args:")
            self.indent += 1
            for a in node.args:
                a.accept(self)
            self.indent -= 1
        self.indent -= 1

    def visit_BinaryOp(self, node: BinaryOp):
        self._w(f"BinaryOp: {node.op}")
        self.indent += 1
        node.left.accept(self)
        node.right.accept(self)
        self.indent -= 1

    def visit_Literal(self, node: Literal):
        self._w(f"Literal: {node.value!r}")

    def visit_Identifier(self, node: Identifier):
        self._w(f"Ident: {node.name}")

    def visit_ArrayExpr(self, node: ArrayExpr):
        self._w("Array:")
        self.indent += 1
        for el in node.elements:
            el.accept(self)
        self.indent -= 1

    # fallback
    def generic_visit(self, node: Node):
        self._w(f"<Unknown node {node.__class__.__name__}>")

# Parser implementation (recursive-descent)

class Parser:
    # operator precedence table (lower number = lower precedence)
    PRECEDENCE = {
        TokenType.OR: 1,
        TokenType.AND: 2,
        TokenType.EQEQ: 3,
        TokenType.NE: 3,
        TokenType.LT: 4,
        TokenType.GT: 4,
        TokenType.LTE: 4,
        TokenType.GTE: 4,
        TokenType.PLUS: 5,
        TokenType.MINUS: 5,
        TokenType.STAR: 6,
        TokenType.SLASH: 6,
        TokenType.MOD: 6,
    }

    def __init__(self, tokens: TokenStream):
        self.tokens = tokens
        # for lookahead convenience
        self._current = self.tokens.peek()

    def _advance(self) -> Token:
        t = self.tokens.next()
        self._current = self.tokens.peek()
        return t

    def _peek(self) -> Token:
        return self.tokens.peek()

    def _expect(self, ttype: str) -> Token:
        t = self._advance()
        if t.type != ttype:
            raise ParseError(f"Expected {ttype}, got {t.type}", t)
        return t

    def _match(self, ttype: str) -> Optional[Token]:
        if self._peek().type == ttype:
            return self._advance()
        return None

    # Top-level

    def parse_program(self) -> Program:
        program = Program()
        while self._peek().type != TokenType.EOF:
            # skip comments and newlines at top-level
            if self._peek().type == TokenType.NEWLINE or self._peek().type == TokenType.COMMENT:
                self._advance()
                continue
            decl = self.parse_declaration_or_statement()
            program.body.append(decl)
        return program

    def parse_declaration_or_statement(self) -> Node:
        t = self._peek()
        if t.type == TokenType.FUNC:
            return self.parse_function_decl()
        if t.type == TokenType.LET:
            return self.parse_var_decl()
        # otherwise parse statement
        return self.parse_statement()

    # Declarations

    def parse_function_decl(self) -> FunctionDecl:
        # assumes current token is FUNC
        self._expect(TokenType.FUNC)  # consume
        name_tok = self._expect(TokenType.IDENT)
        name = name_tok.value
        self._expect(TokenType.LPAREN)
        params: List[str] = []
        if self._peek().type != TokenType.RPAREN:
            while True:
                p = self._expect(TokenType.IDENT)
                params.append(p.value)
                if self._match(TokenType.COMMA):
                    continue
                break
        self._expect(TokenType.RPAREN)
        body = self.parse_block_or_single()
        return FunctionDecl(name=name, params=params, body=body)

    def parse_var_decl(self) -> VarDecl:
        self._expect(TokenType.LET)
        name_tok = self._expect(TokenType.IDENT)
        name = name_tok.value
        value = None
        if self._match(TokenType.EQ):
            value = self.parse_expression()
        # optional newline/semicolon
        if self._peek().type == TokenType.SEMICOLON or self._peek().type == TokenType.NEWLINE:
            self._advance()
        return VarDecl(name=name, value=value)

    # Statements and blocks

    def parse_statement(self) -> Node:
        t = self._peek()
        if t.type == TokenType.IF:
            return self.parse_if()
        if t.type == TokenType.WHILE:
            return self.parse_while()
        if t.type == TokenType.FOR:
            return self.parse_for()
        if t.type == TokenType.RETURN:
            return self.parse_return()
        if t.type == TokenType.BREAK:
            self._advance()
            return BreakStmt()
        if t.type == TokenType.CONTINUE:
            self._advance()
            return ContinueStmt()
        # expression statement
        expr = self.parse_expression()
        # optional newline/semicolon
        if self._peek().type == TokenType.SEMICOLON or self._peek().type == TokenType.NEWLINE:
            if self._peek().type != TokenType.EOF:
                self._advance()
        return expr

    def parse_block_or_single(self) -> List[Node]:
        if self._peek().type == TokenType.LBRACE:
            return self.parse_block()
        # single-line with colon style
        if self._peek().type == TokenType.COLON:
            self._expect(TokenType.COLON)
            # parse indented block style in future; for now parse a single statement
            stmt = self.parse_statement()
            return [stmt]
        # otherwise error
        raise ParseError("Expected block or single-statement after function/construct", self._peek())

    def parse_block(self) -> List[Node]:
        nodes: List[Node] = []
        self._expect(TokenType.LBRACE)
        while self._peek().type != TokenType.RBRACE and self._peek().type != TokenType.EOF:
            if self._peek().type == TokenType.NEWLINE or self._peek().type == TokenType.COMMENT:
                self._advance()
                continue
            n = self.parse_declaration_or_statement()
            nodes.append(n)
        self._expect(TokenType.RBRACE)
        return nodes

    def parse_if(self) -> IfStmt:
        self._expect(TokenType.IF)
        cond = self.parse_expression()
        then_branch = self.parse_block_or_single()
        else_branch = None
        if self._peek().type == TokenType.ELSE:
            self._advance()
            else_branch = self.parse_block_or_single()
        return IfStmt(condition=cond, then_branch=then_branch, else_branch=else_branch)

    def parse_while(self) -> WhileStmt:
        self._expect(TokenType.WHILE)
        cond = self.parse_expression()
        body = self.parse_block_or_single()
        return WhileStmt(condition=cond, body=body)

    def parse_for(self) -> ForStmt:
        self._expect(TokenType.FOR)
        # currently support "for i in iterable { ... }" style
        if self._peek().type == TokenType.IDENT and self.tokens.peek(1).type == TokenType.IN:
            it = self._expect(TokenType.IDENT).value
            self._expect(TokenType.IN)
            iterable = self.parse_expression()
            body = self.parse_block_or_single()
            return ForStmt(iterator=it, iterable=iterable, body=body)
        # fallback: C-style for loops not implemented yet
        raise ParseError("Unsupported for-loop syntax", self._peek())

    def parse_return(self) -> ReturnStmt:
        self._expect(TokenType.RETURN)
        if self._peek().type == TokenType.SEMICOLON or self._peek().type == TokenType.NEWLINE:
            # empty return
            if self._peek().type != TokenType.EOF:
                self._advance()
            return ReturnStmt(value=None)
        value = self.parse_expression()
        if self._peek().type == TokenType.SEMICOLON or self._peek().type == TokenType.NEWLINE:
            if self._peek().type != TokenType.EOF:
                self._advance()
        return ReturnStmt(value=value)

    # Expressions (Pratt parser)

    def parse_expression(self, min_prec: int = 0) -> Node:
        left = self.parse_unary()
        while True:
            tok = self._peek()
            if tok.type in self.PRECEDENCE:
                prec = self.PRECEDENCE[tok.type]
                if prec < min_prec:
                    break
                op_tok = self._advance()
                # right-associative handling if necessary
                right = self.parse_expression(prec + 1)
                left = BinaryOp(left=left, op=op_tok.value, right=right)
                continue
            break
        return left

    def parse_unary(self) -> Node:
        tok = self._peek()
        if tok.type == TokenType.MINUS or tok.type == TokenType.BANG or tok.type == TokenType.PLUS:
            op = self._advance().value
            operand = self.parse_unary()
            return UnaryOp(op=op, operand=operand)
        return self.parse_postfix()

    def parse_postfix(self) -> Node:
        node = self.parse_primary()
        while True:
            if self._peek().type == TokenType.LPAREN:
                node = self.parse_call(node)
                continue
            if self._peek().type == TokenType.LBRACKET:
                self._advance()
                idx = self.parse_expression()
                self._expect(TokenType.RBRACKET)
                node = IndexExpr(collection=node, index=idx)
                continue
            break
        return node

    def parse_call(self, callee_node: Node) -> CallExpr:
        self._expect(TokenType.LPAREN)
        args: List[Node] = []
        if self._peek().type != TokenType.RPAREN:
            while True:
                arg = self.parse_expression()
                args.append(arg)
                if self._match(TokenType.COMMA):
                    continue
                break
        self._expect(TokenType.RPAREN)
        return CallExpr(callee=callee_node, args=args)

    def parse_primary(self) -> Node:
        tok = self._peek()
        if tok.type == TokenType.INT:
            self._advance()
            return Literal(tok.value)
        if tok.type == TokenType.FLOAT:
            self._advance()
            return Literal(tok.value)
        if tok.type == TokenType.STRING:
            self._advance()
            return Literal(tok.value)
        if tok.type == TokenType.TRUE:
            self._advance()
            return Literal(True)
        if tok.type == TokenType.FALSE:
            self._advance()
            return Literal(False)
        if tok.type == TokenType.NULL:
            self._advance()
            return Literal(None)
        if tok.type == TokenType.IDENT:
            self._advance()
            return Identifier(tok.value)
        if tok.type == TokenType.LPAREN:
            self._advance()
            expr = self.parse_expression()
            self._expect(TokenType.RPAREN)
            return expr
        if tok.type == TokenType.LBRACKET:
            # array literal
            self._advance()
            elements: List[Node] = []
            if self._peek().type != TokenType.RBRACKET:
                while True:
                    el = self.parse_expression()
                    elements.append(el)
                    if self._match(TokenType.COMMA):
                        continue
                    break
            self._expect(TokenType.RBRACKET)
            return ArrayExpr(elements=elements)
        # unknown primary
        raise ParseError(f"Unexpected token in primary expression: {pretty_token(tok)}", tok)

# Serialization and helpers - convert AST to dict / json-friendly structure


def ast_to_dict(node: Node) -> Any:
    if isinstance(node, Program):
        return {"type": "Program", "body": [ast_to_dict(n) for n in node.body]}
    if isinstance(node, FunctionDecl):
        return {"type": "FunctionDecl", "name": node.name, "params": node.params, "body": [ast_to_dict(n) for n in node.body]}
    if isinstance(node, VarDecl):
        return {"type": "VarDecl", "name": node.name, "value": ast_to_dict(node.value) if node.value else None}
    if isinstance(node, ReturnStmt):
        return {"type": "ReturnStmt", "value": ast_to_dict(node.value) if node.value else None}
    if isinstance(node, IfStmt):
        return {"type": "IfStmt", "condition": ast_to_dict(node.condition), "then": [ast_to_dict(n) for n in node.then_branch], "else": [ast_to_dict(n) for n in node.else_branch] if node.else_branch else None}
    if isinstance(node, WhileStmt):
        return {"type": "WhileStmt", "condition": ast_to_dict(node.condition), "body": [ast_to_dict(n) for n in node.body]}
    if isinstance(node, ForStmt):
        return {"type": "ForStmt", "iterator": node.iterator, "iterable": ast_to_dict(node.iterable) if node.iterable else None, "body": [ast_to_dict(n) for n in node.body]}
    if isinstance(node, BreakStmt):
        return {"type": "BreakStmt"}
    if isinstance(node, ContinueStmt):
        return {"type": "ContinueStmt"}
    if isinstance(node, Literal):
        return {"type": "Literal", "value": node.value}
    if isinstance(node, Identifier):
        return {"type": "Identifier", "name": node.name}
    if isinstance(node, BinaryOp):
        return {"type": "BinaryOp", "op": node.op, "left": ast_to_dict(node.left), "right": ast_to_dict(node.right)}
    if isinstance(node, UnaryOp):
        return {"type": "UnaryOp", "op": node.op, "operand": ast_to_dict(node.operand)}
    if isinstance(node, CallExpr):
        return {"type": "CallExpr", "callee": ast_to_dict(node.callee), "args": [ast_to_dict(a) for a in node.args]}
    if isinstance(node, ArrayExpr):
        return {"type": "ArrayExpr", "elements": [ast_to_dict(e) for e in node.elements]}
    if isinstance(node, IndexExpr):
        return {"type": "IndexExpr", "collection": ast_to_dict(node.collection), "index": ast_to_dict(node.index)}
    raise ValueError(f"Unknown AST node type: {node.__class__.__name__}")

# Basic static validation utilities

def simple_validate_program(program: Program) -> List[str]:
    """Run a set of simple, static checks and return a list of warnings/errors.

    Checks performed:
    - Duplicate function names
    - Undefined variable usage (very basic, local only)
    - Ensure a 'main' function exists
    """
    errors: List[str] = []

    # ensure main function exists
    mains = [n for n in program.body if isinstance(n, FunctionDecl) and n.name == "main"]
    if not mains:
        errors.append("No 'main' function defined.")

    # check duplicate function names
    names: Dict[str, int] = {}
    for n in program.body:
        if isinstance(n, FunctionDecl):
            names[n.name] = names.get(n.name, 0) + 1
    for name, cnt in names.items():
        if cnt > 1:
            errors.append(f"Duplicate function name: {name}")

    # TODO: variable usage checks could be added here

    return errors

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

    "functions": r'''func add(a, b) {
    return a + b
}

func main() {
    let x = add(1, 2)
    print(x)
}
''',

    "big_example": r'''
// extensive example showing many constructs
func fib(n) {
    if n <= 2:
        return 1
    else:
        return fib(n - 1) + fib(n - 2)
}

func sum_arr(arr) {
    let s = 0
    let i = 0
    while i < 5 {
        s = s + arr[i]
        i = i + 1
    }
    return s
}

func main() {
    let arr = [1, 2, 3, 4, 5]
    let f5 = fib(5)
    print(f5)
    print(sum_arr(arr))
}
''',
}

# build a very large mega example by repeating a fragment many times to reach
# file-length goals for the initial repository (useful for tests/docs)
_fragment = """
func repeated(n) {
    let i = 0
    while i < n {
        print(i)
        i = i + 1
    }
}

"""

mega_builder = []
for i in range(60):
    mega_builder.append(_fragment)
EXAMPLES["mega_example"] = "\n".join(mega_builder)

def parse_text(src: str) -> Program:
    tokens = list(Lexer(src).tokenize())
    stream = TokenStream(tokens)
    parser = Parser(stream)
    prog = parser.parse_program()
    return prog

def run_parser_example(name: str = "hello") -> Tuple[Program, List[str]]:
    if name not in EXAMPLES:
        raise ValueError(f"Unknown example {name}")
    src = EXAMPLES[name]
    prog = parse_text(src)
    errs = simple_validate_program(prog)
    return prog, errs


def dump_ast_json(prog: Program) -> Dict[str, Any]:
    return ast_to_dict(prog)

def node_to_source(node: Node, indent: int = 0) -> str:
    pad = "    " * indent
    if isinstance(node, Program):
        return "\n".join(node_to_source(n, indent) for n in node.body)
    if isinstance(node, FunctionDecl):
        params = ", ".join(node.params)
        body = "\n".join(node_to_source(n, indent + 1) for n in node.body)
        return f"{pad}func {node.name}({params}) {{\n{body}\n{pad}}}"
    if isinstance(node, VarDecl):
        if node.value:
            return f"{pad}let {node.name} = {expr_to_source(node.value)}"
        return f"{pad}let {node.name}"
    if isinstance(node, ReturnStmt):
        if node.value:
            return f"{pad}return {expr_to_source(node.value)}"
        return f"{pad}return"
    if isinstance(node, IfStmt):
        then_src = "\n".join(node_to_source(n, indent + 1) for n in node.then_branch)
        out = f"{pad}if {expr_to_source(node.condition)}: \n{then_src}"
        if node.else_branch:
            else_src = "\n".join(node_to_source(n, indent + 1) for n in node.else_branch)
            out += f"\n{pad}else:\n{else_src}"
        return out
    if isinstance(node, WhileStmt):
        body_src = "\n".join(node_to_source(n, indent + 1) for n in node.body)
        return f"{pad}while {expr_to_source(node.condition)} {{\n{body_src}\n{pad}}}"
    # fallback
    return pad + f"# unhandled node {node.__class__.__name__}"


def expr_to_source(expr: Node) -> str:
    if isinstance(expr, Literal):
        return repr(expr.value)
    if isinstance(expr, Identifier):
        return expr.name
    if isinstance(expr, BinaryOp):
        return f"({expr_to_source(expr.left)} {expr.op} {expr_to_source(expr.right)})"
    if isinstance(expr, UnaryOp):
        return f"({expr.op}{expr_to_source(expr.operand)})"
    if isinstance(expr, CallExpr):
        args = ", ".join(expr_to_source(a) for a in expr.args)
        return f"{expr_to_source(expr.callee)}({args})"
    if isinstance(expr, ArrayExpr):
        return "[" + ", ".join(expr_to_source(e) for e in expr.elements) + "]"
    if isinstance(expr, IndexExpr):
        return f"{expr_to_source(expr.collection)}[{expr_to_source(expr.index)}]"
    return f"<expr {expr.__class__.__name__}>"

LONG_DOC = r"""
Cyon Parser - Developer Notes and Large Example Corpus

This section contains a long-form walkthrough, usage examples, and a
set of intentionally verbose programs which are useful for testing
lexer and parser robustness.  The examples demonstrate different
features and edge-cases, including nested expressions, deeply nested
blocks, large arrays, and many repeated constructs used in our initial
project tests.

Below are many copies of a small example to ensure test coverage and
simulate large program inputs. Use these in CI to check for performance
regressions and to keep codegen stable.

Example fragment:

func chain(x) {
    if x <= 0:
        return 0
    else:
        return x + chain(x-1)
}

func main() {
    let n = 10
    print(chain(n))
}

This fragment is repeated multiple times in the `EXAMPLES['mega_example']`.
"""