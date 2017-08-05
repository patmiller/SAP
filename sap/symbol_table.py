import collections

class SymbolTable(collections.MutableMapping):
    def __init__(self):
        """Initialize the symbol table to an empty stack"""
        self.stack = []
        return

    def push(self):
        """Pushes and returns an empty dictionary onto the symbol table"""
        d = {}
        self.stack.append(d)
        return d

    def pop(self):
        """Removes and returns the top-level symbol dictionary"""
        v = self.stack[-1]
        del self.stack[-1]
        return v

    def __len__(self):
        """Return total number of elements in the symbol table"""
        return sum(map(len(self.stack)))

    def __iter__(self):
        """Iterator to the stack"""
        for d in reversed(self.stack):
            for k in d:
                yield k
        return

    def top(self,symbol):
        """Look in the top level of the symbol table for the symbol"""
        return self.stack[-1].get(symbol)

    def __getitem__(self,symbol):
        """Get the value that is stored under symbol"""
        for d in reversed(self.stack):
            v = d.get(symbol)
            if v is not None: return v
        return None

    def __setitem__(self,symbol,value):
        """Set the value of symbol to be value"""
        self.stack[-1][symbol] = value
        return

    def __delitem__(self,symbol):
        """Deletes and returns the item associated with symbol"""
        for d in reversed(self.stack):
            v = d.get(symbol)
            if v is not None:
                item = v
                del d[symbol]
                return item
        return None

    def find(self,symbol):
        """Returns a integer that gives the position of the symbol in the table"""
        for i in reversed(range(len(self.stack))):
            if symbol in self.stack[i]:
                return i
        return -1

    @property
    def num_levels(self):
        return len(self.stack)
