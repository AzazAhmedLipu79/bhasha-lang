# Bhasha

> **"Programming in the language children think in."**

Bhasha is a Bangla-first educational programming language and compiler designed to
introduce Bangla-speaking children and beginners to programming and computational
thinking — without the English-language barrier that most programming languages
impose.

This repository contains a small, self-contained C++17 implementation of the
Bhasha compiler. It transpiles Bhasha source (`.bng` files) into Python 3 source
and can run the generated program directly.

## Features

- Two numeric/string types: `সংখ্যা` (integer) and `লেখা` (string)
- Variables, assignment, and `দেখাও(...)` print statements
- Arithmetic: `+  -  *  /` (`/` is integer division)
- Comparisons: `>  <  >=  <=  ==  !=`
- Control flow: `যদি / নাহলে` (if / else / else-if) and `যতক্ষণ` (while)
- Bangla-language error messages with helpful suggestions
- Multi-error reporting and crash-safe parsing of malformed input
- No external dependencies — single `bhashac` binary

## Example

```bn
সংখ্যা বয়স = 12;

যদি (বয়স >= 10) {
    দেখাও("তুমি ১০ বা তার বেশি বয়সী");
} নাহলে {
    দেখাও("তুমি ১০ বছরের কম");
}
```

Compile and run:

```bash
./bhashac hello.bng --run
```

Output:

```
Bhasha Compiler

Source: hello.bng

Lexing       ✓
Parsing      ✓
Semantic     ✓
Codegen      ✓

Generated: hello.py

--- Running generated Python ---
হ্যালো বিশ্ব!
```

## Build

Requires a C++17 compiler (clang++ or g++) and `make`. No third-party libraries.

```bash
make            # builds ./bhashac
make clean      # removes build artifacts
make test       # runs the full test suite
make run-demo   # compiles and runs examples/hello.bng
```

## Compiler Pipeline

```
.bng source
    │
    ▼
┌────────┐    ┌────────┐    ┌──────────┐    ┌─────────────┐
│ Lexer  │ ─▶ │ Parser │ ─▶ │ Semantic │ ─▶ │ Code        │
│        │    │        │    │ Analyzer │    │ Generator   │
└────────┘    └────────┘    └──────────┘    └─────────────┘
                                                   │
                                                   ▼
                                              .py source ──▶ python3
```

All four phases are implemented from scratch (no parser generators, no LLVM).
The AST is hand-rolled. Errors are reported in Bangla with line numbers.

## Project Layout

```
bhasha/
├── Makefile
├── src/
│   ├── main.cpp                # CLI driver
│   ├── lexer/                  # Token + Lexer
│   ├── parser/                 # Recursive-descent parser
│   ├── ast/                    # AST node types
│   ├── semantic/               # Type checker + symbol table
│   └── codegen/                # Python source generator
├── examples/                   # Sample .bng programs
└── tests/
    └── run_tests.sh            # Test harness (19 tests)
```

## Tests

```bash
make test
```

The test suite verifies:

1. **Valid programs** compile and produce the expected output (`hello.bng`,
   `variables.bng`, `arithmetic.bng`, `calculator.bng`, `counting.bng`,
   `factorial.bng`, `sum_numbers.bng`, `age_check.bng`, `grade_check.bng`,
   `multiplication_table.bng`).
2. **Error programs** are rejected with non-zero exit, no crash
   (`type_error.bng`, `syntax_error.bng`).
3. **Crash-safety**: malformed inputs (binary garbage, lone braces, unterminated
   tokens, empty files) exit cleanly with code ≤ 1.

All 19 tests pass.

## Limitations (by design)

This is a minimal educational language. It deliberately omits:

- Functions and modules
- Arrays, lists, and dictionaries
- Floating-point numbers
- A REPL or interactive mode

These are intentional scope choices to keep the language small enough for a
beginner to hold in their head.

## License

Educational project. Use freely.