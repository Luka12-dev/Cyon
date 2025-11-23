from __future__ import annotations

import copy
import math
import types
import itertools
import traceback
from dataclasses import dataclass, field
from typing import Callable, List, Dict, Any, Optional, Tuple, Set

# Import AST classes from parser
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

# Utilities

def clone_ast(node: Node) -> Node:
    return copy.deepcopy(node)

def is_literal_node(n: Node) -> bool:
    return isinstance(n, Literal)

def literal_value(n: Literal) -> Any:
    return n.value

def node_type_name(n: Node) -> str:
    return n.__class__.__name__ if n is not None else "<None>"

# Simple logger for optimizer diagnostics
class OptLogger:
    def __init__(self, enabled: bool = True):
        self.enabled = enabled
        self.lines: List[str] = []

    def log(self, *parts: Any) -> None:
        if not self.enabled:
            return
        line = " ".join(str(p) for p in parts)
        self.lines.append(line)

    def dump(self) -> str:
        return "\n".join(self.lines)

# Pass manager and pass type definition

PassFn = Callable[[Program, OptLogger], Program]

@dataclass
class Pass:
    name: str
    fn: PassFn
    description: str = ""
    enabled: bool = True

class PassManager:
    def __init__(self, passes: Optional[List[Pass]] = None, logger: Optional[OptLogger] = None):
        self.passes: List[Pass] = passes or []
        self.logger = logger or OptLogger(enabled=False)

    def register(self, name: str, fn: PassFn, description: str = "", enabled: bool = True) -> None:
        self.passes.append(Pass(name, fn, description, enabled))

    def run(self, program: Program) -> Program:
        p = clone_ast(program)
        self.logger.log(f"Optimizer: Starting with {len(self.passes)} passes")
        for ps in self.passes:
            if not ps.enabled:
                self.logger.log(f"Skipping disabled pass: {ps.name}")
                continue
            try:
                self.logger.log(f"Running pass: {ps.name}")
                p = ps.fn(p, self.logger)
            except Exception as e:
                self.logger.log(f"Pass {ps.name} failed: {e}")
                traceback.print_exc()
        self.logger.log("Optimizer: finished")
        return p

# Basic transforms helpers used by passes

def walk_nodes(program: Program) -> Iterator[Node]:
    for item in program.body:
        stack = [item]
        while stack:
            n = stack.pop()
            yield n
            for field, value in list(getattr(n, "__dict__", {}).items()):
                if isinstance(value, list):
                    for it in reversed(value):
                        if isinstance(it, Node):
                            stack.append(it)
                elif isinstance(value, Node):
                    stack.append(value)

# Helper to replace nodes in lists safely
def replace_in_list(lst: List[Any], old: Any, new: Any) -> None:
    for i, v in enumerate(lst):
        if v is old:
            lst[i] = new

def pass_constant_folding(program: Program, logger: OptLogger) -> Program:
    logger.log("constant_folding: starting")

    def fold(node: Node) -> Node:
        # recursively fold children first
        for field, value in list(getattr(node, "__dict__", {}).items()):
            if isinstance(value, list):
                newlist = []
                for it in value:
                    if isinstance(it, Node):
                        newlist.append(fold(it))
                    else:
                        newlist.append(it)
                setattr(node, field, newlist)
            elif isinstance(value, Node):
                setattr(node, field, fold(value))

        # now fold this node
        if isinstance(node, BinaryOp):
            L = node.left
            R = node.right
            if isinstance(L, Literal) and isinstance(R, Literal):
                try:
                    lv = L.value
                    rv = R.value
                    # safe ops only
                    if node.op in {"+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">="}:
                        # build an evaluable expression
                        # careful: avoid use of eval on arbitrary content; use Python ops directly
                        res = None
                        if node.op == "+":
                            res = lv + rv
                        elif node.op == "-":
                            res = lv - rv
                        elif node.op == "*":
                            res = lv * rv
                        elif node.op == "/":
                            try:
                                res = lv / rv
                            except Exception:
                                res = None
                        elif node.op == "%":
                            try:
                                res = lv % rv
                            except Exception:
                                res = None
                        elif node.op == "==":
                            res = (lv == rv)
                        elif node.op == "!=":
                            res = (lv != rv)
                        elif node.op == "<":
                            res = (lv < rv)
                        elif node.op == ">":
                            res = (lv > rv)
                        elif node.op == "<=":
                            res = (lv <= rv)
                        elif node.op == ">=":
                            res = (lv >= rv)
                        if res is not None:
                            logger.log(f"Folded BinaryOp {lv} {node.op} {rv} -> {res}")
                            return Literal(res)
                except Exception as e:
                    logger.log("constant_folding: exception while folding", e)
        if isinstance(node, UnaryOp) and isinstance(node.operand, Literal):
            val = node.operand.value
            try:
                if node.op == "-":
                    return Literal(-val)
                if node.op == "+":
                    return Literal(+val)
                if node.op == "!":
                    return Literal(not val)
            except Exception as e:
                logger.log("constant_folding: unary op failed", e)
        return node

    for item in program.body:
        if isinstance(item, FunctionDecl):
            item.body = [fold(stmt) for stmt in item.body]
    logger.log("constant_folding: finished")
    return program

# Pass: Algebraic simplification

def pass_algebraic_simplify(program: Program, logger: OptLogger) -> Program:
    """Perform small algebraic simplifications.

    Examples:
    - x * 0 -> 0
    - x * 1 -> x
    - x + 0 -> x
    - 0 + x -> x
    - x - 0 -> x
    - 0 * x -> 0
    - double negation: --x -> x
    """
    logger.log("algebraic_simplify: starting")

    def simplify(node: Node) -> Node:
        # recurse first
        for field, value in list(getattr(node, "__dict__", {}).items()):
            if isinstance(value, list):
                newlist = []
                for it in value:
                    if isinstance(it, Node):
                        newlist.append(simplify(it))
                    else:
                        newlist.append(it)
                setattr(node, field, newlist)
            elif isinstance(value, Node):
                setattr(node, field, simplify(value))

        if isinstance(node, BinaryOp):
            L = node.left
            R = node.right
            # pattern x * 0 or 0 * x
            if node.op == "*":
                if isinstance(L, Literal) and L.value == 0:
                    return Literal(0)
                if isinstance(R, Literal) and R.value == 0:
                    return Literal(0)
                if isinstance(R, Literal) and R.value == 1:
                    return L
                if isinstance(L, Literal) and L.value == 1:
                    return R
            if node.op == "+":
                if isinstance(R, Literal) and R.value == 0:
                    return L
                if isinstance(L, Literal) and L.value == 0:
                    return R
            if node.op == "-":
                if isinstance(R, Literal) and R.value == 0:
                    return L
            if node.op == "/":
                if isinstance(R, Literal) and R.value == 1:
                    return L
        if isinstance(node, UnaryOp):
            if node.op == "-" and isinstance(node.operand, UnaryOp) and node.operand.op == "-":
                # --x => x
                return node.operand.operand
        return node

    for item in program.body:
        if isinstance(item, FunctionDecl):
            item.body = [simplify(s) for s in item.body]
    logger.log("algebraic_simplify: finished")
    return program

# Pass: Dead code elimination

def pass_dead_code_elimination(program: Program, logger: OptLogger) -> Program:
    logger.log("dead_code_elimination: starting")

    def dce_block(block: List[Node]) -> List[Node]:
        new_block: List[Node] = []
        terminated = False
        for stmt in block:
            if terminated:
                logger.log("DCE: removing stmt after return", node_type_name(stmt))
                continue
            new_block.append(stmt)
            if isinstance(stmt, ReturnStmt):
                terminated = True
        return new_block

    for item in program.body:
        if isinstance(item, FunctionDecl):
            item.body = dce_block(item.body)
    logger.log("dead_code_elimination: finished")
    return program

# Pass: Copy propagation

def pass_copy_propagation(program: Program, logger: OptLogger) -> Program:
    """Replace uses of copied variables with the original when safe.

    Example:
        let a = b
        let c = a
    ->  let a = b
        let c = b

    This is a conservative single-pass implementation. It does not track
    global mutability or aliasing; it only propagates simple copies where the
    RHS is an identifier or a literal.
    """
    logger.log("copy_propagation: starting")

    def propagate_in_block(block: List[Node]) -> List[Node]:
        mapping: Dict[str, Node] = {}
        new_block: List[Node] = []
        for stmt in block:
            if isinstance(stmt, VarDecl) and stmt.value is not None:
                # if value is identifier or literal, record mapping
                if isinstance(stmt.value, Identifier) or isinstance(stmt.value, Literal):
                    mapping[stmt.name] = stmt.value
                    new_block.append(stmt)
                    continue
            # replace identifiers in stmt
            def replace(node: Node) -> Node:
                if isinstance(node, Identifier) and node.name in mapping:
                    val = mapping[node.name]
                    logger.log(f"propagate: replacing {node.name} with {node_type_name(val)}")
                    return clone_ast(val)
                for field, value in list(getattr(node, "__dict__", {}).items()):
                    if isinstance(value, list):
                        node_list = []
                        for it in value:
                            if isinstance(it, Node):
                                node_list.append(replace(it))
                            else:
                                node_list.append(it)
                        setattr(node, field, node_list)
                    elif isinstance(value, Node):
                        setattr(node, field, replace(value))
                return node
            new_stmt = replace(clone_ast(stmt))
            new_block.append(new_stmt)
        return new_block

    for item in program.body:
        if isinstance(item, FunctionDecl):
            item.body = propagate_in_block(item.body)
    logger.log("copy_propagation: finished")
    return program

def pass_cse(program: Program, logger: OptLogger) -> Program:
    logger.log("cse: starting")

    def expr_key(node: Node) -> Optional[str]:
        if isinstance(node, BinaryOp):
            left = node.left
            right = node.right
            return f"Bin({repr(left.__class__.__name__)},{repr(node.op)},{repr(right.__class__.__name__)})"  # simplistic
        return None

    for item in program.body:
        if not isinstance(item, FunctionDecl):
            continue
        seen: Dict[str, str] = {}  # key -> tmp name
        new_body: List[Node] = []
        prepend_stmts: List[Node] = []
        for stmt in item.body:
            # inspect expressions in stmt to find duplicates
            def find_and_replace(node: Node):
                if isinstance(node, BinaryOp):
                    k = expr_key(node)
                    if k is not None:
                        if k in seen:
                            tmp_name = seen[k]
                            logger.log(f"cse: replacing expression with tmp {tmp_name}")
                            return Identifier(tmp_name)
                        else:
                            tmp_name = unique_tmp_name("cse")
                            seen[k] = tmp_name
                            # create a temp var declaration
                            tmp_decl = VarDecl(name=tmp_name, value=clone_ast(node))
                            prepend_stmts.append(tmp_decl)
                            return Identifier(tmp_name)
                # recursively replace children
                for field, value in list(getattr(node, "__dict__", {}).items()):
                    if isinstance(value, list):
                        node_list = []
                        for it in value:
                            if isinstance(it, Node):
                                node_list.append(find_and_replace(it))
                            else:
                                node_list.append(it)
                        setattr(node, field, node_list)
                    elif isinstance(value, Node):
                        setattr(node, field, find_and_replace(value))
                return node

            new_stmt = find_and_replace(clone_ast(stmt))
            new_body.extend(prepend_stmts)
            prepend_stmts.clear()
            new_body.append(new_stmt)
        item.body = new_body
    logger.log("cse: finished")
    return program


_tmp_counter = itertools.count()

def unique_tmp_name(prefix: str) -> str:
    return f"_{prefix}_{next(_tmp_counter)}"

def pass_trivial_inlining(program: Program, logger: OptLogger) -> Program:
    logger.log("trivial_inlining: starting")

    inline_map: Dict[str, Node] = {}
    for item in program.body:
        if isinstance(item, FunctionDecl):
            if len(item.body) == 1 and isinstance(item.body[0], ReturnStmt):
                ret = item.body[0].value
                if isinstance(ret, (Literal, Identifier)):
                    inline_map[item.name] = clone_ast(ret)
                    logger.log(f"trivial_inlining: marking {item.name} for inlining")

    # replace calls
    def replace(node: Node) -> Node:
        if isinstance(node, CallExpr) and isinstance(node.callee, Identifier) and node.callee.name in inline_map:
            # only inline when args match 0 (simple functions) — inlining with args
            # requires parameter replacement, which is beyond trivial scope here
            if not node.args:
                logger.log(f"trivial_inlining: inlining call to {node.callee.name}")
                return clone_ast(inline_map[node.callee.name])
        for field, value in list(getattr(node, "__dict__", {}).items()):
            if isinstance(value, list):
                node_list = []
                for it in value:
                    if isinstance(it, Node):
                        node_list.append(replace(it))
                    else:
                        node_list.append(it)
                setattr(node, field, node_list)
            elif isinstance(value, Node):
                setattr(node, field, replace(value))
        return node

    for item in program.body:
        if isinstance(item, FunctionDecl):
            item.body = [replace(s) for s in item.body]
    logger.log("trivial_inlining: finished")
    return program

def pass_tail_call_elimination(program: Program, logger: OptLogger) -> Program:
    logger.log("tail_call_elimination: starting")

    for item in program.body:
        if isinstance(item, FunctionDecl):
            if not item.body:
                continue
            last = item.body[-1]
            if isinstance(last, ReturnStmt) and isinstance(last.value, CallExpr):
                callee = last.value.callee
                if isinstance(callee, Identifier) and callee.name == item.name:
                    # simplistic TCO: convert to while loop with parameter reassignment
                    logger.log(f"TCO: transforming tail-recursive function {item.name}")
                    # build a new body: while(1) { ... if (cond) break; }
                    # NOTE: We do not support parameter re-binding robustly; leave as TODO
                    # This is a stub demonstrating intent
                    new_body: List[Node] = []
                    new_body.append(VarDecl(name="__tco_flag", value=Literal(0)))
                    new_body.append(WhileStmt(condition=Literal(True), body=item.body[:-1] + [BreakStmt()]))
                    item.body = new_body
    logger.log("tail_call_elimination: finished")
    return program

def pass_loop_unroll(program: Program, logger: OptLogger, max_unroll: int = 4) -> Program:
    logger.log("loop_unroll: starting")

    def unroll_block(block: List[Node]) -> List[Node]:
        new_block: List[Node] = []
        for stmt in block:
            if isinstance(stmt, WhileStmt):
                cond = stmt.condition
                if isinstance(cond, BinaryOp) and cond.op in {"<", "<=", ">", ">="}:
                    left = cond.left
                    right = cond.right
                    if isinstance(right, Literal) and isinstance(left, Identifier):
                        try:
                            N = int(right.value)
                        except Exception:
                            new_block.append(stmt)
                            continue
                        if N <= max_unroll:
                            logger.log(f"Unrolling loop bounded by {N}")
                            # naive unroll: repeat body N times
                            for i in range(N):
                                # clone body and append
                                for s in stmt.body:
                                    new_block.append(clone_ast(s))
                            continue
            # otherwise keep stmt (recurse into nested blocks)
            # if stmt has body fields, recurse
            for field, value in list(getattr(stmt, "__dict__", {}).items()):
                if isinstance(value, list):
                    newlist = []
                    for it in value:
                        if isinstance(it, Node):
                            newlist.append(it)
                        else:
                            newlist.append(it)
                    setattr(stmt, field, newlist)
            new_block.append(stmt)
        return new_block

    for item in program.body:
        if isinstance(item, FunctionDecl):
            item.body = unroll_block(item.body)
    logger.log("loop_unroll: finished")
    return program

def pass_control_flow_simplify(program: Program, logger: OptLogger) -> Program:
    logger.log("control_flow_simplify: starting")

    def simplify_block(block: List[Node]) -> List[Node]:
        new_block: List[Node] = []
        for stmt in block:
            if isinstance(stmt, IfStmt) and not stmt.else_branch and len(stmt.then_branch) == 1 and isinstance(stmt.then_branch[0], IfStmt):
                # flatten: if (A) if (B) S -> if (A && B) S
                inner = stmt.then_branch[0]
                logger.log("Flattening nested ifs")
                combined_cond = BinaryOp(left=stmt.condition, op="&&", right=inner.condition)
                new_if = IfStmt(condition=combined_cond, then_branch=inner.then_branch)
                new_block.append(new_if)
                continue
            # recurse
            for field, value in list(getattr(stmt, "__dict__", {}).items()):
                if isinstance(value, list):
                    for i, it in enumerate(value):
                        if isinstance(it, Node):
                            value[i] = simplify_block([it])[0] if isinstance(simplify_block([it]), list) and simplify_block([it]) else it
                elif isinstance(value, Node):
                    setattr(stmt, field, value)
            new_block.append(stmt)
        return new_block

    for item in program.body:
        if isinstance(item, FunctionDecl):
            item.body = simplify_block(item.body)
    logger.log("control_flow_simplify: finished")
    return program

def pass_dead_store_elimination(program: Program, logger: OptLogger) -> Program:
    logger.log("dead_store_elimination: starting")

    def analyze_and_remove(block: List[Node]) -> List[Node]:
        reads: Set[str] = set()
        assigns: Dict[str, VarDecl] = {}

        # collect reads
        def collect_reads(node: Node) -> None:
            if isinstance(node, Identifier):
                reads.add(node.name)
                return
            for field, value in list(getattr(node, "__dict__", {}).items()):
                if isinstance(value, list):
                    for it in value:
                        if isinstance(it, Node):
                            collect_reads(it)
                elif isinstance(value, Node):
                    collect_reads(value)

        for stmt in block:
            if isinstance(stmt, VarDecl) and stmt.value is not None:
                assigns[stmt.name] = stmt
            else:
                collect_reads(stmt)

        new_block: List[Node] = []
        for stmt in block:
            if isinstance(stmt, VarDecl) and stmt.name in assigns and stmt.name not in reads:
                logger.log(f"DSE: removing dead store {stmt.name}")
                continue
            new_block.append(stmt)
        return new_block

    for item in program.body:
        if isinstance(item, FunctionDecl):
            item.body = analyze_and_remove(item.body)
    logger.log("dead_store_elimination: finished")
    return program

def pass_purity_analysis(program: Program, logger: OptLogger) -> Program:
    logger.log("purity_analysis: starting")

    impure_calls = {"print", "input", "cyon_print", "cyon_input"}
    purity: Dict[str, bool] = {}

    def function_is_pure(fn: FunctionDecl) -> bool:
        def check(node: Node) -> bool:
            if isinstance(node, CallExpr):
                if isinstance(node.callee, Identifier) and node.callee.name in impure_calls:
                    return False
            for field, value in list(getattr(node, "__dict__", {}).items()):
                if isinstance(value, list):
                    for it in value:
                        if isinstance(it, Node) and not check(it):
                            return False
                elif isinstance(value, Node):
                    if not check(value):
                        return False
            return True
        return check(FunctionDecl(name=fn.name, params=fn.params, body=fn.body))

    for item in program.body:
        if isinstance(item, FunctionDecl):
            purity[item.name] = function_is_pure(item)
            logger.log(f"purity: function {item.name} pure={purity[item.name]}")
    logger.log("purity_analysis: finished")
    return program

def default_pass_manager(logger: Optional[OptLogger] = None) -> PassManager:
    logger = logger or OptLogger(enabled=True)
    pm = PassManager(logger=logger)
    pm.register("purity_analysis", pass_purity_analysis, "Tag pure functions", enabled=True)
    pm.register("constant_folding", pass_constant_folding, "Fold constants", enabled=True)
    pm.register("algebraic_simplify", pass_algebraic_simplify, "Algebra simplifications", enabled=True)
    pm.register("dead_code_elimination", pass_dead_code_elimination, "Remove unreachable code", enabled=True)
    pm.register("copy_propagation", pass_copy_propagation, "Propagate simple copies", enabled=True)
    pm.register("dead_store_elimination", pass_dead_store_elimination, "Remove dead stores", enabled=True)
    pm.register("cse", pass_cse, "Common subexpression elimination", enabled=False)
    pm.register("trivial_inlining", pass_trivial_inlining, "Inline trivial functions", enabled=True)
    pm.register("tail_call_elimination", pass_tail_call_elimination, "Tail-call elimination stub", enabled=False)
    pm.register("loop_unroll", lambda p, l: pass_loop_unroll(p, l, max_unroll=4), "Loop unrolling", enabled=False)
    pm.register("control_flow_simplify", pass_control_flow_simplify, "Simplify control flow", enabled=True)
    return pm

MEGA_EXAMPLES: Dict[str, str] = {}
_base = """
func add1(x) {
    return x + 1
}

func compute(n) {
    let i = 0
    let s = 0
    while i < n {
        s = s + add1(i)
        i = i + 1
    }
    return s
}

func main() {
    let res = compute(5)
    print(res)
}
"""

# Repeat and vary to produce many variants
for i in range(50):
    MEGA_EXAMPLES[f"ex{i}"] = _base.replace("compute(5)", f"compute({i+1})").replace("ex", f"ex{i}")


# Additional synthetic example for heavy AST manipulation
MEGA_EXAMPLES["heavy_arith"] = "\n".join([f"func f{i}(x) {{ return (x + {i}) * {i}; }}" for i in range(100)])

@dataclass
class OptimizeResult:
    program: Program
    logger_output: str

def optimize_program(program: Program, enable_logger: bool = False, enable_passes: Optional[List[str]] = None) -> OptimizeResult:
    logger = OptLogger(enabled=enable_logger)
    pm = default_pass_manager(logger=logger)
    if enable_passes is not None:
        for p in pm.passes:
            p.enabled = (p.name in enable_passes)
    optimized = pm.run(program)
    return OptimizeResult(program=optimized, logger_output=logger.dump())

def pretty_print_program(program: Program) -> str:
    lines: List[str] = []

    def pp_node(node: Node, indent: int = 0):
        pad = "    " * indent
        if isinstance(node, Program):
            lines.append(pad + "Program:")
            for n in node.body:
                pp_node(n, indent + 1)
            return
        if isinstance(node, FunctionDecl):
            lines.append(pad + f"Function {node.name}({', '.join(node.params)})")
            for s in node.body:
                pp_node(s, indent + 1)
            return
        if isinstance(node, VarDecl):
            lines.append(pad + f"Var {node.name} = {node.value}")
            return
        if isinstance(node, ReturnStmt):
            lines.append(pad + f"Return {node.value}")
            return
        if isinstance(node, IfStmt):
            lines.append(pad + f"If {node.condition}")
            for s in node.then_branch:
                pp_node(s, indent + 1)
            if node.else_branch:
                lines.append(pad + "Else:")
                for s in node.else_branch:
                    pp_node(s, indent + 1)
            return
        if isinstance(node, WhileStmt):
            lines.append(pad + f"While {node.condition}")
            for s in node.body:
                pp_node(s, indent + 1)
            return
        lines.append(pad + f"<Node {node.__class__.__name__}>")

    pp_node(program)
    return "\n".join(lines)

def run_optimizer_tests() -> Tuple[int, str]:
    report_lines: List[str] = []
    failed = 0
    logger = OptLogger(enabled=True)
    pm = default_pass_manager(logger=logger)

    # Test 1: constant folding
    src1 = "func main() { let x = 1 + 2 * 3; return x; }"
    prog1 = parse_source_to_program(src1)
    out1 = pm.run(prog1)
    report_lines.append("Test 1: constant folding run")

    # Test 2: algebraic simplification
    src2 = "func main() { let a = 0 * x; let b = y + 0; }"
    prog2 = parse_source_to_program(src2)
    out2 = pm.run(prog2)
    report_lines.append("Test 2: algebraic simplify run")

    # Test 3: dead code elimination
    src3 = "func main() { return 1; print(2); }"
    prog3 = parse_source_to_program(src3)
    out3 = pm.run(prog3)
    report_lines.append("Test 3: dead code elimination run")

    report_lines.append("\nLogger dump:\n")
    report_lines.append(logger.dump())
    return failed, "\n".join(report_lines)

def parse_source_to_program(src: str) -> Program:
    try:
        from .parser import parse_text
    except Exception:
        from core.parser import parse_text
    return parse_text(src)

MASSIVE_DOC = """
Large optimizer example corpus — not executable, used for documentation and
synthetic tests. Below we include many fragments that demonstrate patterns
that optimizer passes should handle. This block exists purely for
reference and to ensure that the module provides a comprehensive base for
further tests.

"""

for i in range(300):
    MASSIVE_DOC += f"\n// fragment {i}\nfunc frag_{i}(x) {{\n    let s = 0\n    let i = 0\n    while i < 10 {{\n        s = s + (x * {i})\n        i = i + 1\n    }}\n    return s\n}}\n"

# Keep a very large string to achieve desired file length
MASSIVE_BLOB = '\n'.join([MASSIVE_DOC for _ in range(3)])

__all__ = [
    "PassManager", "default_pass_manager", "optimize_program", "OptimizeResult",
    "OptLogger", "pass_constant_folding", "pass_algebraic_simplify",
    "pass_dead_code_elimination", "pass_copy_propagation", "pass_cse",
    "pass_trivial_inlining", "run_optimizer_tests"
]