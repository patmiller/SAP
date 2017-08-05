import ast

import sap.compiler_base

class BaseError(Exception):
    def __init__(self, *args):
        raise NotImplementedError('BaseError cannot be initialized - use SemanticError instead')

    def make_message(self, compiler, node):
        prefix, suffix = self.prefix_suffix(compiler, node)
        self.message = prefix + self.msg + suffix
        return

    def prefix_suffix(self, compiler, node):
        if (compiler is None) or (not hasattr(compiler, 'function')):
            return '', ''
        try:
            fname = compiler.function.func_name
        except:
            return '', ''

        if node is None:
            return '%s: ' % fname, ''
        lineno = getattr(node,'lineno',None)
        col_offset = getattr(node,'col_offset',0)

        prefix = '%s: ' % fname
        suffix = ''
        if lineno is not None:
            try:
                prefix = '\n%s:%d:%s: ' % (compiler.filename,lineno,fname)
            except:
                prefix = '%s: ' % fname
            try:
                suffix = '\n'+[x for x in open(compiler.filename)][lineno-1] + (' ' * col_offset + '^')
            except:
                pass
        return prefix, suffix

class SemanticError(BaseError):
    def __init__(self, compiler, node, msg):
        if hasattr(self, 'msg'):
            self.msg += msg
        else:
            self.msg = msg
        self.make_message(compiler, node)
        return

class ArityError(SemanticError):
    def __init__(self, compiler, node, actual, expected):
        assert actual != expected, 'Trying to throw an ArityError where actual is equal to expected'
        SemanticError.__init__(self, compiler, node, 'Expected arity of %d, actual arity of %d' % (expected, actual))
        return

class AssignmentArityError(SemanticError):
    def __init__(self, compiler, node, lhs, rhs):
        assert len(lhs) != len(rhs), 'Trying to throw an AssignmentArityError where lhs and rhs have same len'
        assert isinstance(node, ast.Assign), 'node passed into AssignmentArityError must be of type ast.Assign'
        SemanticError.__init__(self, compiler, node, 'lhs has arity %d, rhs has arity %d' % (len(lhs), len(rhs)))
        return

class NotSupportedError(SemanticError):
    def __init__(self, compiler, node):
        SemanticError.__init__(self, compiler, node, 'Not supported in SAP')
        return

class SymbolTableError(SemanticError):
    def __init__(self, compiler, node):
        if hasattr(compiler, 'symtab'):
            if hasattr(self, 'msg'):
                self.msg += '\nSymbol table has %d levels' % compiler.symtab.num_levels
            else:
                self.msg = '\nSymbol table has %d levels' % compiler.symtab.num_levels
        SemanticError.__init__(self, compiler, node, '')
        return

class SingleAssignmentError(SymbolTableError):
    def __init__(self, compiler, node):
        self.msg = 'Single Assignment Violation, %s exists' % node.id
        if hasattr(compiler, 'symtab'):
            self.msg += ' in level %d of the symbol table' % compiler.symtab.find(node.id)
        SymbolTableError.__init__(self, compiler, node)
        return

class SymbolLookupError(SymbolTableError):
    def __init__(self, compiler, node):
        self.msg = 'Name %s not found in symbol table' % node.id
        SymbolTableError.__init__(self, compiler, node)
        return

class PlaceholderError(SymbolTableError):
    def __init__(self, compiler, node):
        self.msg = 'Name %s is assigned to a placeholder value' % node.id
        SymbolTableError.__init__(self, compiler, node)
        return

class SymbolNameError(SymbolTableError):
    def __init__(self, compiler, node):
        self.msg = 'LHS must be a name'
        SymbolTableError.__init__(self, compiler, node)
        return

class SAPTypeError(SemanticError):
    def __init__(self, compiler, node):
        SemanticError.__init__(self, compiler, node, 'Types do not match')
        return

class CompilerError(object):
    """A class which contains methods for Compiler error calls"""
    def __init__(self, compiler):
        assert isinstance(compiler, sap.compiler_base.CompilerBase), 'Compiler passed in to CompilerError is not valid'
        self.compiler = compiler
        return

    def semantic_error(self, node, msg):
        raise SemanticError(self.compiler, node, msg)

    def arity_error(self, node, actual, expected):
        raise ArityError(self.compiler, node, actual, expected)

    def assignment_arity_error(self, node, lhs, rhs):
        raise AssignmentArityError(self.compiler, node, lhs, rhs)

    def not_supported_error(self, node):
        raise NotSupportedError(self.compiler, node)

    def symbol_table_error(self, node):
        raise SymbolTableError(self.compiler, node)

    def single_assignment_error(self, node):
        raise SingleAssignmentError(self.compiler, node)

    def symbol_lookup_error(self, node):
        raise SymbolLookupError(self.compiler, node)

    def placeholder_error(self, node):
        raise PlaceholderError(self.compiler, node)

    def symbol_name_error(self, node):
        raise SymbolNameError(self.compiler, node)

    def type_error(self, node):
        raise SAPTypeError(self.compiler, node)
