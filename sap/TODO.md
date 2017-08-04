# TODO List

## Language Updates
- Build up all types of expressions - Different class maybe??
- Make docstrings better
- if-elif-else statements
- external math functions - sin, cos, etc.??
- Notion of state - old()
- while loops
- for loops
- type casting
- Remove self.semantic\_error calls from compiler.py and replace with better errors

## symbol\_table.py
- SymbolTable(collections.MutableMapping)

## compiler.py
- Compiler(sap.error.CompilerError)
  - field: symbol\_table = sap.symbol\_table.SymbolTable()

## expression.py
- Expression class
- extended by Compiler

## Tests
- Test API for symbol\_table.py
- Test compiler.py
