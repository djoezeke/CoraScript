# CoraScript 

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
- Module import: `import module.name`
- Member access: `module.value`, `module.function()`, `module.Class()`
- Operators: `+ - * / %`, comparison, `and/or/not`, `&& || !`
- Strings with either `'single quotes'` or `"double quotes"`
- Output: `print(expr)`

### Builtins

The runtime now includes easier helpers for registering builtins and a larger Python-like builtin set:

- Functions: `print`, `len`, `type`, `repr`, `str`, `int`, `float`, `bool`, `abs`, `pow`, `min`, `max`, `sum`, `range_obj`
- Callable builtin types: `object`, `dict`, `list`, `range_obj`, `creator`
- Object helpers: `get`, `set`, `has`, `delete`, `clear`, `keys`, `len`, `toString`

### Standard modules

The runtime also exposes importable standard modules:

- `io`: `print`, `println`, `input`, `read_file`, `write_file`, `exists`
- `math` and `maths`: `pi`, `tau`, `e`, `abs`, `pow`, `sqrt`, `min`, `max`, `sum`, `floor`, `ceil`, `round`, `sin`, `cos`, `tan`
- `strings`: `lower`, `upper`, `trim`, `contains`, `replace`
- `time`: `now_ms`, `now_seconds`, `sleep_ms`, `sleep_seconds`
- `std`: umbrella module containing `io`, `math`, `maths`, `strings`, and `time`

### lib/std modules

The repository now also ships script-based standard modules under `lib/std`:

- `lib/std/math.cora`
- `lib/std/io.cora`
- `lib/std/strings.cora`
- `lib/std/std.cora`
- `lib/std/collections.cora`
- `lib/std/fs.cora`
- `lib/std/time.cora`

These can be imported directly, for example: `import std.math`, `import std.io`, `import std.strings`, `import std.collections`, `import std.fs`, `import std.time`.

See [examples/builtins.cora](examples/builtins.cora) for a full demo.

See [examples/import_demo.cora](examples/import_demo.cora) and [examples/modules/geometry.cora](examples/modules/geometry.cora) for module usage.

See [examples/lib_std_demo.cora](examples/lib_std_demo.cora) and [examples/namespace_scope_demo.cora](examples/namespace_scope_demo.cora) for `lib/std` imports and namespace/class/function scope behavior.

### Build

Use CMake to configure and build the `corascript` target.

### Run sample

Run the executable with:

`examples/sample.cora`
