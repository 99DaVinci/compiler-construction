Arithmetic Expression Compiler in C++

A five-stage arithmetic expression compiler written in C++11 for a Compiler Construction project. Instead of evaluating math strings directly in one pass, it implements the full compilation pipeline from raw source text to stack-machine bytecode execution.

Supported Features

Operators: Addition (+), Subtraction (-), Multiplication (*), Division (/), and Unary + / -

Grouping: Nested parentheses ( ... ) with standard PEMDAS/BODMAS precedence

Numbers: Integers and floating-point decimals

Error Handling: Reports exact character positions for lexical errors, syntax errors, and division-by-zero runtime errors

Compiler Pipeline

Stage

Component

Output

1. Lexical Analysis

Lexer

Stream of tokens (NUMBER, PLUS, MINUS, MUL, DIV, LPAREN, RPAREN, EOF)

2. Syntax Analysis

Parser

Abstract Syntax Tree (AST) via recursive descent

3. Intermediate Code

TACGenerator

Three-Address Code (t1 = 3 + 4, t2 = t1 * 2, ...)

4. Target Code

AssemblyGenerator

Stack assembly instructions (PUSH, ADD, SUB, MUL, DIV, NEG)

5. Execution

StackVM

Evaluates the stack bytecode and prints the final result

How to Compile & Run

Using Terminal (GCC / MinGW / Clang)

g++ -std=c++11 arithmetic_compiler.cpp -o arithmetic_compiler


Run the compiled program:

# Linux / macOS
./arithmetic_compiler

# Windows (Command Prompt / PowerShell)
arithmetic_compiler.exe


Using Dev-C++, Code::Blocks, or OnlineGDB

Dev-C++ / Code::Blocks: Open arithmetic_compiler.cpp, make sure -std=c++11 is enabled in compiler settings, and press F11 (Dev-C++) or F9 (Code::Blocks).

VS Code: Enable code-runner.runInTerminal in Settings so input runs in the interactive Terminal tab.

OnlineGDB: Paste arithmetic_compiler.cpp into the OnlineGDB C++ Compiler and click Run.

Example Usage

After starting the program, type any arithmetic expression at the Enter expression > prompt (or type q to quit):

Enter expression > (3 + 4) * 2 - 5

============================================================
SOURCE EXPRESSION: (3 + 4) * 2 - 5
============================================================

[Phase 1] Lexical Analysis (Tokens):
  Token(LPAREN, '(')
  Token(NUMBER, 3)
  Token(PLUS, '+')
  Token(NUMBER, 4)
  Token(RPAREN, ')')
  Token(MUL, '*')
  Token(NUMBER, 2)
  Token(MINUS, '-')
  Token(NUMBER, 5)
  Token(EOF)

[Phase 2] Syntax Analysis (Abstract Syntax Tree):
`-- BinOp(-)
    |-- BinOp(*)
    |   |-- BinOp(+)
    |   |   |-- Number(3)
    |   |   `-- Number(4)
    |   `-- Number(2)
    `-- Number(5)

[Phase 3] Intermediate Code Generation (Three-Address Code):
  01: t1 = 3 + 4
  02: t2 = t1 * 2
  03: t3 = t2 - 5
  Result held in: t3

[Phase 4] Target Code Generation (Stack Assembly):
  01: PUSH 3
  02: PUSH 4
  03: ADD
  04: PUSH 2
  05: MUL
  06: PUSH 5
  07: SUB

[Phase 5] Final Answer: 9
============================================================


Repository Files

arithmetic_compiler.cpp — Complete C++11 compiler source code
