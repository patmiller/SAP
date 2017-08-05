import sap.if1
import sap.symbol_table
import sap.error

class CompilerBase(object):
    def __init__(self):
        self.m = sap.if1.Module()
        self.symtab = sap.symbol_table.SymbolTable()
        self.error = sap.error.CompilerError(self)
        for type in ('boolean','character','doublereal',
                'integer','null','real','wild'):
            setattr(self,type,getattr(self.m,type))
        return

    def set_base_attr(self, f):
        self.function = f
        self.filename = f.func_code.co_filename
        return

    def visit(self,node,*args):
        mname = type(node).__name__
        method = getattr(self,mname,self.default)
        return method(node,*args)

    def default(self,node):
        self.error.semantic_error(node, 'not implemented %s' % type(node).__name__)
        return
