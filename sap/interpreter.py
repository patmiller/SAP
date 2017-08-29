import math


class Interpreter(object):
    def IFForall(self, m, n, *args): raise NotImplementedError("IFForall")

    def IFSelect(self, m, n, *args): raise NotImplementedError("IFSelect")

    def IFTagCase(self, m, n, *args): raise NotImplementedError("IFTagCase")

    def IFLoopA(self, m, n, *args): raise NotImplementedError("IFLoopA")

    def IFLoopB(self, m, n, *args): raise NotImplementedError("IFLoopB")

    def IFIfThenElse(self, m, n, *args):
        assert args
        if args[0]:
            g = n.children[0]
        else:
            g = n.children[1]
        return m.interpret(self, g, *args)

    def IFIterate(self, m, n, *args):
        values = list(args)
        while values and values[0]:
            body = n.children[0]
            iteration = m.interpret(self, body, *values)

            # update the values with whatever the graph actually generates
            # if we don't have an input edge, it is a "loop carried" value
            for input, v in zip(body.inputs, iteration):
                values[input.port - 1] = v
        return tuple(values)

    def IFWhileLoop(
        self, m, n, *args): raise NotImplementedError("IFWhileLoop")

    def IFRepeatLoop(
        self, m, n, *args): raise NotImplementedError("IFRepeatLoop")

    def IFSeqForall(
        self, m, n, *args): raise NotImplementedError("IFSeqForall")

    def IFUReduce(self, m, n, *args): raise NotImplementedError("IFUReduce")

    def IFAAddH(self, m, n, *args): raise NotImplementedError("IFAAddH")

    def IFAAddL(self, m, n, *args): raise NotImplementedError("IFAAddL")

    def IFAAdjust(self, m, n, *args): raise NotImplementedError("IFAAdjust")

    def IFABuild(self, m, n, *args): raise NotImplementedError("IFABuild")

    def IFACatenate(
        self, m, n, *args): raise NotImplementedError("IFACatenate")

    def IFAElement(self, m, n, *args): raise NotImplementedError("IFAElement")

    def IFAFill(self, m, n, *args): raise NotImplementedError("IFAFill")

    def IFAGather(self, m, n, *args): raise NotImplementedError("IFAGather")

    def IFAIsEmpty(self, m, n, *args): raise NotImplementedError("IFAIsEmpty")

    def IFALimH(self, m, n, *args): raise NotImplementedError("IFALimH")

    def IFALimL(self, m, n, *args): raise NotImplementedError("IFALimL")

    def IFARemH(self, m, n, *args): raise NotImplementedError("IFARemH")

    def IFARemL(self, m, n, *args): raise NotImplementedError("IFARemL")

    def IFAReplace(self, m, n, *args): raise NotImplementedError("IFAReplace")

    def IFAScatter(self, m, n, *args): raise NotImplementedError("IFAScatter")

    def IFASetL(self, m, n, *args): raise NotImplementedError("IFASetL")

    def IFASize(self, m, n, *args): raise NotImplementedError("IFASize")

    def IFAbs(self, m, n, a): return abs(a)

    def IFBindArguments(
        self, m, n, *args): raise NotImplementedError("IFBindArguments")

    def IFBool(self, m, n, a):
        if a == '\0':
            return False
        return bool(a)

    def IFCall(self, m, n, *args):
        f = args[0]
        return m.interpret(self, f, *args[1:])

    def IFChar(self, m, n, a):
        if isinstance(a, str):
            return a
        return chr(a)

    def IFDiv(self, m, n, a, b):
        if isinstance(a, bool) or isinstance(b, bool):
            return a and b
        return a / b

    def IFDouble(self, m, n, a):
        import numpy
        return numpy.float64(a)

    def IFEqual(self, m, n, a, b): return a == b

    def IFExp(self, m, n, a, b): return float(a**b)

    def IFFirstValue(
        self, m, n, *args): raise NotImplementedError("IFFirstValue")

    def IFFinalValue(
        self, m, n, *args): raise NotImplementedError("IFFinalValue")

    def IFFloor(self, m, n, a): return math.floor(a)

    def IFInt(self, m, n, a): return int(a)

    def IFIsError(self, m, n, *args): raise NotImplementedError("IFIsError")

    def IFLess(self, m, n, a, b): return a < b

    def IFLessEqual(self, m, n, a, b): return a <= b

    def IFGreat(self, m, n, a, b): return a > b

    def IFGreatEqual(self, m, n, a, b): return a >= b

    def IFMax(self, m, n, a, b): return max(a, b)

    def IFMin(self, m, n, a, b): return min(a, b)

    def IFMinus(self, m, n, a, b):
        if isinstance(a, bool) or isinstance(b, bool):
            return a or b
        return a - b

    def IFMod(self, m, n, a, b): return a % b

    def IFNeg(self, m, n, a): return -a

    def IFNoOp(self, m, n, *args):
        return args

    def IFNot(self, m, n, a): return not a

    def IFNotEqual(self, m, n, a, b): return a != b

    def IFPlus(self, m, n, a, b):
        if isinstance(a, bool) or isinstance(b, bool):
            return a or b
        return a + b

    def IFRangeGenerate(
        self, m, n, *args): raise NotImplementedError("IFRangeGenerate")

    def IFRBuild(self, m, n, *args): raise NotImplementedError("IFRBuild")

    def IFRElements(
        self, m, n, *args): raise NotImplementedError("IFRElements")

    def IFRReplace(self, m, n, *args): raise NotImplementedError("IFRReplace")

    def IFRedLeft(self, m, n, *args): raise NotImplementedError("IFRedLeft")

    def IFRedRight(self, m, n, *args): raise NotImplementedError("IFRedRight")

    def IFRedTree(self, m, n, *args): raise NotImplementedError("IFRedTree")

    def IFReduce(self, m, n, *args): raise NotImplementedError("IFReduce")

    def IFRestValues(
        self, m, n, *args): raise NotImplementedError("IFRestValues")

    def IFSingle(self, m, n, a):
        import numpy
        return numpy.float32(a)

    def IFTimes(self, m, n, a, b):
        if isinstance(a, bool) or isinstance(b, bool):
            return a and b
        return a * b

    def IFTrunc(self, m, n, a): return math.trunc(a)

    def IFPrefixSize(
        self, m, n, *args): raise NotImplementedError("IFPrefixSize")

    def IFError(self, m, n, *args): raise NotImplementedError("IFError")

    def IFReplaceMulti(
        self, m, n, *args): raise NotImplementedError("IFReplaceMulti")

    def IFConvert(self, m, n, *args): raise NotImplementedError("IFConvert")

    def IFCallForeign(
        self, m, n, *args): raise NotImplementedError("IFCallForeign")

    def IFAElementN(
        self, m, n, *args): raise NotImplementedError("IFAElementN")

    def IFAElementP(
        self, m, n, *args): raise NotImplementedError("IFAElementP")

    def IFAElementM(
        self, m, n, *args): raise NotImplementedError("IFAElementM")

    def IFGreat(self, m, n, a, b): return a > b

    def IFGreatEqual(self, m, n, a, b): return a >= b

    def IFAnd(self, m, n, a, b): return a and b

    def IFOr(self, m, n, a, b): return a or b
