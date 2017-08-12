import unittest

from sap.symbol_table import *


class SymbolTableTest(unittest.TestCase):
    """Test the SymbolTable API"""

    def test_simple_symbol_table(self):
        """Test simple parts of symbol table"""
        sym = SymbolTable()
        self.assertEqual(1, sym.num_levels)
        self.assertEqual(0, len(sym))

        sym['v'] = 3
        self.assertEqual(3, sym['v'])
        self.assertEqual(3, sym.top('v'))
        self.assertEqual(0, sym.find('v'))
        self.assertEqual(None, sym['k'])
        self.assertEqual(-1, sym.find('k'))
        self.assertEqual(1, len(sym))
        sym.push()
        self.assertEqual(2, sym.num_levels)
        self.assertEqual(1, len(sym))
        sym.pop()

        del sym['v']
        self.assertEqual(None, sym['v'])

        sym.pop()
        self.assertEqual(0, sym.num_levels)
        self.assertEqual(0, len(sym))
        return
