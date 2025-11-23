# Cyon Programming Language Examples

This directory contains working examples of Cyon programs that demonstrate the language's features.

## Features Implemented

### ✅ Input Functions
- `input_int(prompt)` - Read integer input from user
- `input_double(prompt)` - Read floating point input from user  
- `input_str(prompt)` - Read string input from user

### ✅ Print Statements  
- `print(args...)` - Print multiple values with automatic formatting
- Supports strings, integers, and variables

### ✅ Variable Declarations
- `int variable_name = value;` - Integer variables
- `double variable_name = value;` - Floating point variables
- `string variable_name = value;` - String variables

### ✅ Control Flow
- `if condition: statement;` - Simple if statements
- Proper semicolon (`;`) handling for statement termination

### ✅ Arithmetic Operations
- Basic math: `+`, `-`, `*`, `/`, `%`
- Comparison operators: `==`, `!=`, `<`, `>`, `<=`, `>=`

## Working Examples

### Basic Examples
- `hello_world.cyon` - Simple Hello World program
- `simple_input_test.cyon` - Basic input/output test

### Interactive Programs  
- `calculator_interactive.cyon` - Interactive calculator with user input
- `math_quiz.cyon` - Simple math quiz game
- `counting_game.cyon` - Number counting demonstration

### Previous Examples (updated to work with current compiler)
- `calculator_simple.cyon` - Basic calculator operations
- `fibonacci.cyon` - Fibonacci sequence generator
- `array_operations.cyon` - Simple math operations demo
- `control_structures.cyon` - Control flow examples

## How to Use

### Compile and Run
```bash
# Build a program
python compiler.py build examples/calculator_interactive.cyon

# Build and run immediately  
python compiler.py run examples/simple_input_test.cyon
```

### Compilation Process
1. Cyon source (`.cyon`) → C code (`.c`)
2. C code → Native executable
3. Input functions are automatically included

## Language Syntax

### Variable Declaration with Input
```cyon
int age = input_int("Enter your age: ");
double price = input_double("Enter price: ");
string name = input_str("Enter name: ");
```

### Print Statements
```cyon
print("Hello, World!");
print("Your age is: ", age);
print("Result: ", a, " + ", b, " = ", result);
```

### Conditional Statements
```cyon
if age >= 18:
    print("You are an adult!");
```

### Arithmetic
```cyon
int sum = a + b;
int product = x * y;
int remainder = num % 2;
```

## Notes

- **Semicolons (`;`)** properly end statements and create line breaks in generated C code
- **Input functions** work interactively - program waits for user input
- **Type system** supports int, double, and string types
- **Real-time compilation** - changes are immediately reflected in compiled output
- **Cross-platform** - generates standard C code that compiles on Windows/Linux/macOS

## Example Session

```
$ python compiler.py run examples/calculator_interactive.cyon
Interactive Calculator
====================
Enter first number: 15
Enter second number: 3
Results:
--------
Addition: 15 + 3 = 18
Subtraction: 15 - 3 = 12
Multiplication: 15 * 3 = 45
Division: 15 / 3 = 5
Calculator finished!
```

This demonstrates the Cyon language working as intended with proper input functionality and semicolon handling!