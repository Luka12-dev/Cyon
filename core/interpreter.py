from __future__ import annotations

import sys
import time
import math
import traceback
import types
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional, Callable, Tuple

# Import AST classes
try:
    from .parser import (
        Program, FunctionDecl, VarDecl, IfStmt, WhileStmt, ForStmt, ReturnStmt,
        BreakStmt, ContinueStmt, Literal, Identifier, BinaryOp, UnaryOp,
        CallExpr, ArrayExpr, IndexExpr, Node
    )
except Exception:
    from core.parser import (
        Program, FunctionDecl, VarDecl, IfStmt, WhileStmt, ForStmt, ReturnStmt,
        BreakStmt, ContinueStmt, Literal, Identifier, BinaryOp, UnaryOp,
        CallExpr, ArrayExpr, IndexExpr, Node
    )

@dataclass
class RuntimeError(Exception):
    message: str
    node: Optional[Node] = None

    def __str__(self):
        loc = f" at node {self.node.__class__.__name__}" if self.node else ""
        return f"RuntimeError: {self.message}{loc}"

@dataclass
class FunctionValue:
    name: str
    params: List[str]
    body: List[Node]
    env: "Environment"  # closure

    def __repr__(self):
        return f"<Function {self.name}/{len(self.params)}>"

@dataclass
class ArrayValue:
    elements: List[Any]

    def __repr__(self):
        return f"[{', '.join(repr(e) for e in self.elements)}]"

@dataclass
class NativeFunction:
    fn: Callable
    name: str

    def __call__(self, *args, **kwargs):
        return self.fn(*args, **kwargs)

    def __repr__(self):
        return f"<NativeFunction {self.name}>"

@dataclass
class Environment:
    parent: Optional["Environment"] = None
    values: Dict[str, Any] = field(default_factory=dict)

    def define(self, name: str, value: Any) -> None:
        self.values[name] = value

    def set(self, name: str, value: Any) -> None:
        env = self.find_env_for(name)
        if env is not None:
            env.values[name] = value
        else:
            self.values[name] = value

    def get(self, name: str) -> Any:
        if name in self.values:
            return self.values[name]
        if self.parent:
            return self.parent.get(name)
        raise RuntimeError(f"Undefined variable '{name}'")

    def find_env_for(self, name: str) -> Optional["Environment"]:
        if name in self.values:
            return self
        if self.parent:
            return self.parent.find_env_for(name)
        return None

@dataclass
class ReturnSignal(Exception):
    value: Any

@dataclass
class BreakSignal(Exception):
    pass

@dataclass
class ContinueSignal(Exception):
    pass

class Interpreter:
    def __init__(self, *, debug: bool = False, trace: bool = False, step: bool = False):
        self.global_env = Environment(parent=None)
        self.debug = debug
        self.trace = trace
        self.step = step
        self._setup_builtins()
        self.call_stack: List[Tuple[str, Environment]] = []
        self.exec_count = 0
        self.profile: Dict[str, int] = {}

    def _setup_builtins(self):
        # register builtin functions (wrap native Python functions)
        self.global_env.define('print', NativeFunction(self._builtin_print, 'print'))
        self.global_env.define('input', NativeFunction(self._builtin_input, 'input'))
        self.global_env.define('len', NativeFunction(self._builtin_len, 'len'))
        self.global_env.define('range', NativeFunction(self._builtin_range, 'range'))
        self.global_env.define('time', NativeFunction(self._builtin_time, 'time'))
        # math helpers
        self.global_env.define('sin', NativeFunction(math.sin, 'sin'))
        self.global_env.define('cos', NativeFunction(math.cos, 'cos'))
        self.global_env.define('int', NativeFunction(int, 'int'))
        self.global_env.define('float', NativeFunction(float, 'float'))

    def _builtin_print(self, *args):
        out = " ".join(str(a) for a in args)
        print(out)
        return None

    def _builtin_input(self, prompt: str = ''):
        try:
            return input(prompt)
        except EOFError:
            return ''

    def _builtin_len(self, x):
        try:
            return len(x)
        except Exception:
            raise RuntimeError("len() applied to non-collection")

    def _builtin_range(self, *args):
        return list(range(*args))

    def _builtin_time(self):
        return time.time()

    def run_program(self, program: Program) -> Any:
        # load function declarations
        for node in program.body:
            if isinstance(node, FunctionDecl):
                fn = FunctionValue(name=node.name, params=node.params, body=node.body, env=self.global_env)
                self.global_env.define(node.name, fn)
            elif isinstance(node, VarDecl):
                val = self.eval_expression(node.value) if node.value else None
                self.global_env.define(node.name, val)
            else:
                # top-level expression - evaluate and ignore result
                self.eval_statement(node)

        # call main
        try:
            main = self.global_env.get('main')
        except RuntimeError:
            raise RuntimeError("No 'main' function defined")

        if isinstance(main, FunctionValue):
            return self.call_function(main, [])
        elif isinstance(main, NativeFunction):
            return main()
        else:
            raise RuntimeError("'main' is not a function")

    def call_function(self, fn: FunctionValue, args: List[Any]) -> Any:
        if self.trace:
            print(f"TRACE: call {fn.name} with args={args}")
        if len(args) != len(fn.params):
            raise RuntimeError(f"Function {fn.name} expected {len(fn.params)} args, got {len(args)}")
        env = Environment(parent=fn.env)
        for name, value in zip(fn.params, args):
            env.define(name, value)
        # push stack
        self.call_stack.append((fn.name, env))
        try:
            # execute body
            for stmt in fn.body:
                self.exec_count += 1
                self._profile_hit(fn.name)
                self.eval_statement(stmt, env)
            # if no explicit return, return None
            return None
        except ReturnSignal as rs:
            return rs.value
        finally:
            self.call_stack.pop()

    def call_native(self, native: NativeFunction, args: List[Any]) -> Any:
        if self.trace:
            print(f"TRACE: native call {native.name} args={args}")
        return native(*args)

    def _profile_hit(self, name: str):
        self.profile[name] = self.profile.get(name, 0) + 1

    def eval_statement(self, node: Node, env: Optional[Environment] = None) -> Any:
        env = env or self.global_env
        if isinstance(node, VarDecl):
            val = self.eval_expression(node.value, env) if node.value else None
            env.define(node.name, val)
            if self.debug:
                print(f"DEBUG: VarDecl {node.name} = {val}")
            return None
        if isinstance(node, ReturnStmt):
            val = self.eval_expression(node.value, env) if node.value else None
            raise ReturnSignal(val)
        if isinstance(node, IfStmt):
            cond = self.eval_expression(node.condition, env)
            if cond:
                for s in node.then_branch:
                    self.eval_statement(s, env)
            elif node.else_branch:
                for s in node.else_branch:
                    self.eval_statement(s, env)
            return None
        if isinstance(node, WhileStmt):
            while True:
                cond = self.eval_expression(node.condition, env)
                if not cond:
                    break
                try:
                    for s in node.body:
                        self.eval_statement(s, env)
                except BreakSignal:
                    break
                except ContinueSignal:
                    continue
            return None
        if isinstance(node, ForStmt):
            # simple "for i in iterable" implementation
            if node.iterator and node.iterable:
                iterable = self.eval_expression(node.iterable, env)
                if not hasattr(iterable, '__iter__'):
                    raise RuntimeError("for-loop iterable not iterable")
                for it in iterable:
                    env.define(node.iterator, it)
                    try:
                        for s in node.body:
                            self.eval_statement(s, env)
                    except BreakSignal:
                        break
                    except ContinueSignal:
                        continue
                return None
            raise RuntimeError("Unsupported for-loop construct")
        if isinstance(node, BreakStmt):
            raise BreakSignal()
        if isinstance(node, ContinueStmt):
            raise ContinueSignal()
        # expression statement
        if isinstance(node, Node):
            self.eval_expression(node, env)
            return None
        raise RuntimeError(f"Unknown statement type: {node.__class__.__name__}")

    def eval_expression(self, node: Node, env: Optional[Environment] = None) -> Any:
        env = env or self.global_env
        if isinstance(node, Literal):
            return node.value
        if isinstance(node, Identifier):
            return env.get(node.name)
        if isinstance(node, BinaryOp):
            left = self.eval_expression(node.left, env)
            right = self.eval_expression(node.right, env)
            op = node.op
            return self._apply_binop(op, left, right)
        if isinstance(node, UnaryOp):
            val = self.eval_expression(node.operand, env)
            return self._apply_unary(node.op, val)
        if isinstance(node, CallExpr):
            callee = self.eval_expression(node.callee, env)
            args = [self.eval_expression(a, env) for a in node.args]
            if isinstance(callee, FunctionValue):
                return self.call_function(callee, args)
            if isinstance(callee, NativeFunction):
                return self.call_native(callee, args)
            raise RuntimeError(f"Attempted to call non-function: {callee}")
        if isinstance(node, ArrayExpr):
            elements = [self.eval_expression(e, env) for e in node.elements]
            return ArrayValue(elements=elements)
        if isinstance(node, IndexExpr):
            coll = self.eval_expression(node.collection, env)
            idx = self.eval_expression(node.index, env)
            try:
                return coll[idx]
            except Exception as e:
                raise RuntimeError(f"Indexing error: {e}")
        raise RuntimeError(f"Unsupported expression type: {node.__class__.__name__}")

    def _apply_binop(self, op: str, a: Any, b: Any) -> Any:
        # map some operator symbols to python
        if op == TokenType.PLUS or op == '+':
            return a + b
        if op == TokenType.MINUS or op == '-':
            return a - b
        if op == TokenType.STAR or op == '*':
            return a * b
        if op == TokenType.SLASH or op == '/':
            return a / b
        if op == TokenType.MOD or op == '%':
            return a % b
        if op == TokenType.EQEQ or op == '==':
            return a == b
        if op == TokenType.NE or op == '!=':
            return a != b
        if op == TokenType.LT or op == '<':
            return a < b
        if op == TokenType.GT or op == '>':
            return a > b
        if op == TokenType.LTE or op == '<=':
            return a <= b
        if op == TokenType.GTE or op == '>=':
            return a >= b
        if op == TokenType.AND or op == '&&':
            return bool(a) and bool(b)
        if op == TokenType.OR or op == '||':
            return bool(a) or bool(b)
        # fallback: try python eval on small set
        try:
            return eval(f"{repr(a)} {op} {repr(b)}")
        except Exception as e:
            raise RuntimeError(f"Unsupported binary operator {op}: {e}")

    def _apply_unary(self, op: str, v: Any) -> Any:
        if op == TokenType.BANG or op == '!':
            return not bool(v)
        if op == TokenType.MINUS or op == '-':
            return -v
        if op == TokenType.PLUS or op == '+':
            return +v
        raise RuntimeError(f"Unsupported unary operator: {op}")

    def dump_stack(self) -> None:
        print("CALL STACK:")
        for name, env in reversed(self.call_stack):
            print(f" - {name} with locals: {list(env.values.keys())}")

    def profile_report(self) -> None:
        print("--- PROFILE REPORT ---")
        for name, cnt in sorted(self.profile.items(), key=lambda x: -x[1]):
            print(f"{name}: {cnt} hits")

    def repl(self, prompt: str = 'cyon> '):
        print("Cyon REPL. Type 'exit' or Ctrl-D to quit.")
        while True:
            try:
                line = input(prompt)
            except EOFError:
                break
            if not line:
                continue
            if line.strip() in ('exit', 'quit'):
                break
            try:
                toks = tokenize_text(line)
                prog = parse_text(line)
                result = self.run_program(prog)
                print(repr(result))
            except Exception as e:
                print(f"Error: {e}")
                traceback.print_exc()
        print("Bye")


try:
    from .lexer import tokenize_text
    from .parser import parse_text
except Exception:
    from core.lexer import tokenize_text
    from core.parser import parse_text

def run_source(src: str, *, debug: bool = False, trace: bool = False) -> Any:
    try:
        prog = parse_text(src)
    except Exception as e:
        raise RuntimeError(f"Parse error: {e}")
    interp = Interpreter(debug=debug, trace=trace)
    return interp.run_program(prog)

EXAMPLES: Dict[str, str] = {
    "hello": r'''func main() {
    print("Hello, world!")
}
''',

    "conditions": r'''func main() {
    let x = 10
    if x > 5:
        print("Big")
    else:
        print("Small")
}
''',

    "loops": r'''func main() {
    let i = 0
    while i < 5 {
        print(i)
        i = i + 1
    }
}
''',

    "arrays": r'''func main() {
    let a = [1, 2, 3]
    print(len(a))
    print(a[1])
}
''',

    "recursion": r'''func fact(n) {
    if n <= 1:
        return 1
    else:
        return n * fact(n - 1)
}
func main() {
    print(fact(5))
}
''',
}

# Build a large corpus by repeating examples and small variations
_corpus = []
for i in range(200):
    _cor = f"func gen{i}(x) {{\n    let s = 0\n    let j = 0\n    while j < x {{\n        s = s + j\n        j = j + 1\n    }}\n    return s\n}}\n"
    _cor += f"func main{i}() {{\n    print(gen{i}(5))\n}}\n"
    _corpus.append(_cor)
LARGE_CORPUS = "\n".join(_corpus)

class StepDriver:
    def __init__(self, program: Program, interp: Interpreter):
        self.program = program
        self.interp = interp
        self.history: List[Tuple[str, Any]] = []
        self.pc = 0
        # snapshot of global environment at start
        self.init_state = None

    def step_forward(self) -> None:
        # execute next top-level statement
        if self.pc >= len(self.program.body):
            print("End of program")
            return
        node = self.program.body[self.pc]
        print(f"Stepping: executing {node.__class__.__name__}")
        try:
            self.interp.eval_statement(node)
        except ReturnSignal as rs:
            print(f"ReturnSignal: {rs.value}")
        except Exception as e:
            print(f"Error during step: {e}")
        self.pc += 1

    def run_all(self) -> None:
        while self.pc < len(self.program.body):
            self.step_forward()

class Sandbox:
    def __init__(self, interp: Interpreter, time_limit: float = 2.0):
        self.interp = interp
        self.time_limit = time_limit

    def run(self, program: Program) -> Any:
        start = time.time()
        # simple execution with periodic time checks
        for node in program.body:
            elapsed = time.time() - start
            if elapsed > self.time_limit:
                raise RuntimeError("Time limit exceeded in sandbox")
            self.interp.eval_statement(node)
        # optionally call main
        main = self.interp.global_env.get('main')
        if isinstance(main, FunctionValue):
            return self.interp.call_function(main, [])
        return None

def run_interpreter_tests() -> Tuple[int, str]:
    failed = 0
    report_lines: List[str] = []
    interp = Interpreter(debug=False, trace=False)

    # Test basic program
    try:
        interp.run_program(parse_text(EXAMPLES['hello']))
        report_lines.append("hello ok")
    except Exception as e:
        failed += 1
        report_lines.append(f"hello failed: {e}")

    # Test recursion
    try:
        interp.run_program(parse_text(EXAMPLES['recursion']))
        report_lines.append("recursion ok")
    except Exception as e:
        failed += 1
        report_lines.append(f"recursion failed: {e}")

    # Test arrays
    try:
        interp.run_program(parse_text(EXAMPLES['arrays']))
        report_lines.append("arrays ok")
    except Exception as e:
        failed += 1
        report_lines.append(f"arrays failed: {e}")

    return failed, "\n".join(report_lines)

BIG_BLOCK = """
# BIG BLOCK START
"""

for i in range(500):
    BIG_BLOCK += f"\n// example {i}\nfunc auto_gen_{i}(n) {{\n    let acc = 0\n    let i = 0\n    while i < n {{\n        acc = acc + i\n        i = i + 1\n    }}\n    return acc\n}}\n\n"

BIG_BLOCK += "\n# BIG BLOCK END\n"

__all__ = [
    'Interpreter', 'run_source', 'FunctionValue', 'ArrayValue', 'NativeFunction',
    'Environment', 'ReturnSignal', 'BreakSignal', 'ContinueSignal',
    'StepDriver', 'Sandbox', 'run_interpreter_tests'
]
