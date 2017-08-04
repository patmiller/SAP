# TODO List

## Language Updates
- Make Compiler have a field which is CompilerError
  - redo tests for CompilerError
  - call field self.error so that something like self.error.semantic\_error(stuff) makes sense
- Build SymbolTable to always have len at least 1
- Test API for symbol\_table.py
- Build symbol table tests
- Make docstrings better
  - Come up with a template for docstrings
  - Maybe use the sphinx template so that documentation can be generated??
- Remove self.semantic\_error calls from compiler.py and replace with better errors
- Test compiler.py
- Build up all types of expressions - Different class maybe??
- if-elif-else statements
- Notion of state - old()
- while loops
- for loops
- type casting
- external math functions - sin, cos, etc.??
