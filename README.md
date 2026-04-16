# Toy Compiler

## CoraScript Interpreter

This repository now includes a working `corascript` interpreter with:

- Lexer
- Recursive-descent parser
- Runtime evaluator

### Supported syntax (C + Python style)

- Variable declarations: `let`, `int`, `float`, `bool`, `string`
- Assignment: `name = expr`
- Conditions: `if` / `elif` / `else`
- Loops: `while`, C-style `for (...)`, Python-style `for x in range(...)`
- Loop control: `break`, `continue`, `pass`
- Blocks using either:
	- C-style braces: `{ ... }`
	- Python-style indentation: `:` followed by indented lines
- Operators: `+ - * / %`, comparison, `and/or/not`, `&& || !`
- Strings with either `'single quotes'` or `"double quotes"`
- Output: `print(expr)`

### Build

Use CMake to configure and build the `corascript` target.

### Run sample

Run the executable with:

`examples/sample.cora`
