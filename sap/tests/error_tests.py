import unittest
import ast

from sap.error import *
from sap.compiler_base import *

def get_func_code():
    ret = 'def three():\n'
    ret += '\tv = 3\n'
    ret += '\treturn v'
    return ret

def get_func():
    def three():
        v = 3
        return v
    return three

class TestError(unittest.TestCase):
    """Test the Errors API"""

    def setUp(self):
        """Set up a simple SAP object to pass into errors"""
        self.base = CompilerBase()
        self.base.function = get_func()
        self.base.symtab.push()
        self.base.symtab['v'] = 3
        src = ast.parse(get_func_code())
        self.func_node = src.body[0]
        self.assign_node = self.func_node.body[0]
        self.target = self.assign_node.targets[0]
        return

    def tearDown(self):
        """Tear down the SAP object"""
        self.base = None
        return

    def test_BaseError(self):
        """Make sure that BaseError cannot be created"""
        with self.assertRaises(NotImplementedError) as e:
            a = BaseError()
        self.assertIn('BaseError cannot be initialized - use SemanticError instead', e.exception.message)
        return

    def test_SemanticError(self):
        """Test the SemanticError API"""
        se = SemanticError(self.base, self.func_node, 'message')
        self.assertIn('message', se.message)
        return

    def test_ArityError(self):
        """Test the ArityError API"""
        ae = ArityError(self.base, self.func_node, 1, 2)
        self.assertIn('Expected arity of 2, actual arity of 1', ae.message)
        with self.assertRaises(AssertionError) as e:
            ae = ArityError(self.base, self.func_node, 2, 2)
        self.assertIn('Trying to throw an ArityError where actual is equal to expected', e.exception.message)
        return

    def test_AssignmentArityError(self):
        """Test the AssignmentArityError API"""
        aae = AssignmentArityError(self.base, self.assign_node, [1, 2], [1, 2, 3])
        self.assertIn('lhs has arity 2, rhs has arity 3', aae.message)
        with self.assertRaises(AssertionError) as e:
            aae = AssignmentArityError(self.base, self.func_node, [1, 2], [1, 2, 3])
        self.assertIn('node passed into AssignmentArityError must be of type ast.Assign', e.exception.message)
        return

    def test_NotSupportedError(self):
        """Test the NotSupportedError API"""
        nse = NotSupportedError(self.base, self.func_node)
        self.assertIn('Not supported in SAP', nse.message)
        return

    def test_SymbolTableError(self):
        """Test the SymbolTableError API"""
        ste = SymbolTableError(self.base, self.func_node)
        self.assertIn('Symbol table has 1 levels', ste.message)
        return

    def test_SingleAssignmentError(self):
        """Test the SingleAssignmentError API"""
        sae = SingleAssignmentError(self.base, self.target)
        self.assertIn('Single Assignment Violation, v exists in level 0 of the symbol table', sae.message)
        return

    def test_SymbolLookupError(self):
        """Test the SymbolLookupError API"""
        sle = SymbolLookupError(self.base, self.target)
        self.assertIn('Name v not found in symbol table', sle.message)
        return

    def test_PlaceholderError(self):
        """Test the PlaceholderError API"""
        pe = PlaceholderError(self.base, self.target)
        self.assertIn('Name v is assigned to a placeholder value', pe.message)
        return

    def test_SymbolNameError(self):
        """Test the SymbolNameError API"""
        sne = SymbolNameError(self.base, self.target)
        self.assertIn('LHS must be a name', sne.message)
        return

    def test_SAPTypeError(self):
        """Test the SAPTypeError API"""
        ste = SAPTypeError(self.base, self.assign_node)
        self.assertIn('Types do not match', ste.message)
        return

class CompilerErrorTest(unittest.TestCase):
    """Test the CompilerError API"""

    def setUp(self):
        """Set up a simple CompilerBase object to pass into errors"""
        self.base = CompilerBase()
        self.base.function = get_func()
        self.base.symtab.push()
        self.base.symtab['v'] = 3
        src = ast.parse(get_func_code())
        self.func_node = src.body[0]
        self.assign_node = self.func_node.body[0]
        self.target = self.assign_node.targets[0]
        return

    def tearDown(self):
        """Tear down the CompilerBase object"""
        self.base = None
        return

    def test_init(self):
        """Test initialization of CompilerBase"""
        with self.assertRaises(AssertionError) as e:
            a = CompilerError([1, 2, 3])
        self.assertIn('Compiler passed in to CompilerError is not valid', e.exception.message)
        return

    def test_semantic_error(self):
        """Check that CompilerError makes a SemanticError"""
        with self.assertRaises(SemanticError):
            self.base.error.semantic_error(self.func_node, 'message')
        return

    def test_arity_error(self):
        """Check that CompilerError makes a ArityError"""
        with self.assertRaises(ArityError):
            self.base.error.arity_error(self.func_node, 0, 1)
        return

    def test_assignment_arity_error(self):
        """Check that CompilerError makes a AssignmentArityError"""
        with self.assertRaises(AssignmentArityError):
            self.base.error.assignment_arity_error(self.assign_node, [1, 2, 3], [1, 2])
        return

    def test_not_supported_error(self):
        """Check that CompilerError makes a NotSupportedError"""
        with self.assertRaises(NotSupportedError):
            self.base.error.not_supported_error(self.func_node)
        return

    def test_symbol_table_error(self):
        """Check that CompilerError makes a SymbolTableError"""
        with self.assertRaises(SymbolTableError):
            self.base.error.symbol_table_error(self.target)
        return

    def test_single_assignment_error(self):
        """Check that CompilerError makes a SingleAssignmentError"""
        with self.assertRaises(SingleAssignmentError):
            self.base.error.single_assignment_error(self.target)
        return

    def test_symbol_lookup_error(self):
        """Check that CompilerError makes a SymbolLookupError"""
        with self.assertRaises(SymbolLookupError):
            self.base.error.symbol_lookup_error(self.target)
        return

    def test_placeholder_error(self):
        """Check that CompilerError makes a PlaceholderError"""
        with self.assertRaises(PlaceholderError):
            self.base.error.placeholder_error(self.target)
        return

    def test_symbol_name_error(self):
        """Check that CompilerError makes a SymbolNameError"""
        with self.assertRaises(SymbolNameError):
            self.base.error.symbol_name_error(self.target)
        return

    def test_type_error(self):
        """Check that CompilerError makes a SAPTypeError"""
        with self.assertRaises(SAPTypeError):
            self.base.error.type_error(self.assign_node)
        return
