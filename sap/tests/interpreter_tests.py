import unittest
from sap.interpreter import Interpreter
from sap.if1 import Module

class TestInterpreter(unittest.TestCase):
    def disabled_test_IFForall(self):
        raise NotImplementedError("IFForall")
    def disabled_test_IFSelect(self):
        raise NotImplementedError("IFSelect")
    def disabled_test_IFTagCase(self):
        raise NotImplementedError("IFTagCase")
    def disabled_test_IFLoopA(self):
        raise NotImplementedError("IFLoopA")
    def disabled_test_IFLoopB(self):
        raise NotImplementedError("IFLoopB")
    def disabled_test_IFIfThenElse(self):
        raise NotImplementedError("IFIfThenElse")
    def test_IFIterate(self):
        # main(integer n)
        #   i = 0
        #   sum = 0
        #   while i < n:
        #      i = old i + 1
        #      sum = old sum + i
        #      yield sum
        #   return sum
        m = Module()
        main = m.addfunction("main")
        main[1] = m.integer

        test = main.addnode(m.IFLess)  # i < n
        test[1] = m.boolean
        test(1) << 0  # i
        test(2) << main[1] # n
        
        it = main.addnode(m.IFIterate)
        it(1) << test[1] # test
        it(2) << 0       # i
        it(3) << main[1] # n
        it(4) << 0       # sum

        it[1] = m.boolean # test
        it[2] = m.integer # i
        it[3] = m.integer # n
        it[4] = m.integer # sum


        g = it.addgraph()
        g[1] = m.boolean  # old test
        g[2] = m.integer  # old i
        g[3] = m.integer  # old n
        g[4] = m.integer  # old sum

        plus_i = g.addnode(m.IFPlus) # new i = old i + 1
        plus_i(1) << g[2] # old i
        plus_i(2) << 1    # 1
        plus_i[1] = m.integer # new i

        plus_sum = g.addnode(m.IFPlus) # new sum = old sum + i
        plus_sum(1) << g[4] # old sum
        plus_sum(2) << plus_i[1] # i
        plus_sum[1] = m.integer

        lt = g.addnode(m.IFLess) # new test = new i < n
        lt(1) << plus_i[1]
        lt(2) << g[3]
        lt[1] = m.boolean

        g(1) << lt[1]       # new test
        g(2) << plus_i[1]   # new i
        g(4) << plus_sum[1] # new sum

        main(1) << it[4]
        self.assertEquals(m.interpret(Interpreter(),main,10),(55,))
        return

    def disabled_test_IFWhileLoop(self):
        raise NotImplementedError("IFWhileLoop")
    def disabled_test_IFRepeatLoop(self):
        raise NotImplementedError("IFRepeatLoop")
    def disabled_test_IFSeqForall(self):
        raise NotImplementedError("IFSeqForall")
    def disabled_test_IFUReduce(self):
        raise NotImplementedError("IFUReduce")
    def disabled_test_IFAAddH(self):
        raise NotImplementedError("IFAAddH")
    def disabled_test_IFAAddL(self):
        raise NotImplementedError("IFAAddL")
    def disabled_test_IFAAdjust(self):
        raise NotImplementedError("IFAAdjust")
    def disabled_test_IFABuild(self):
        raise NotImplementedError("IFABuild")
    def disabled_test_IFACatenate(self):
        raise NotImplementedError("IFACatenate")
    def disabled_test_IFAElement(self):
        raise NotImplementedError("IFAElement")
    def disabled_test_IFAFill(self):
        raise NotImplementedError("IFAFill")
    def disabled_test_IFAGather(self):
        raise NotImplementedError("IFAGather")
    def disabled_test_IFAIsEmpty(self):
        raise NotImplementedError("IFAIsEmpty")
    def disabled_test_IFALimH(self):
        raise NotImplementedError("IFALimH")
    def disabled_test_IFALimL(self):
        raise NotImplementedError("IFALimL")
    def disabled_test_IFARemH(self):
        raise NotImplementedError("IFARemH")
    def disabled_test_IFARemL(self):
        raise NotImplementedError("IFARemL")
    def disabled_test_IFAReplace(self):
        raise NotImplementedError("IFAReplace")
    def disabled_test_IFAScatter(self):
        raise NotImplementedError("IFAScatter")
    def disabled_test_IFASetL(self):
        raise NotImplementedError("IFASetL")
    def disabled_test_IFASize(self):
        raise NotImplementedError("IFASize")
    def disabled_test_IFAbs(self):
        raise NotImplementedError("IFAbs")
    def disabled_test_IFBindArguments(self):
        raise NotImplementedError("IFBindArguments")
    def disabled_test_IFBool(self):
        raise NotImplementedError("IFBool")
    def disabled_test_IFCall(self):
        raise NotImplementedError("IFCall")
    def disabled_test_IFChar(self):
        raise NotImplementedError("IFChar")
    def disabled_test_IFDiv(self):
        raise NotImplementedError("IFDiv")
    def disabled_test_IFDouble(self):
        raise NotImplementedError("IFDouble")
    def disabled_test_IFEqual(self):
        raise NotImplementedError("IFEqual")
    def disabled_test_IFExp(self):
        raise NotImplementedError("IFExp")
    def disabled_test_IFFirstValue(self):
        raise NotImplementedError("IFFirstValue")
    def disabled_test_IFFinalValue(self):
        raise NotImplementedError("IFFinalValue")
    def disabled_test_IFFloor(self):
        raise NotImplementedError("IFFloor")
    def disabled_test_IFInt(self):
        raise NotImplementedError("IFInt")
    def disabled_test_IFIsError(self):
        raise NotImplementedError("IFIsError")
    def disabled_test_IFLess(self):
        raise NotImplementedError("IFLess")
    def disabled_test_IFLessEqual(self):
        raise NotImplementedError("IFLessEqual")
    def disabled_test_IFGreat(self):
        raise NotImplementedError("IFGreat")
    def disabled_test_IFGreatEqual(self):
        raise NotImplementedError("IFGreatEqual")
    def disabled_test_IFMax(self):
        raise NotImplementedError("IFMax")
    def disabled_test_IFMin(self):
        raise NotImplementedError("IFMin")
    def disabled_test_IFMinus(self):
        raise NotImplementedError("IFMinus")
    def disabled_test_IFMod(self):
        raise NotImplementedError("IFMod")
    def disabled_test_IFNeg(self):
        raise NotImplementedError("IFNeg")
    def disabled_test_IFNoOp(self):
        raise NotImplementedError("IFNoOp")
    def disabled_test_IFNot(self):
        raise NotImplementedError("IFNot")
    def disabled_test_IFNotEqual(self):
        raise NotImplementedError("IFNotEqual")
    def disabled_test_IFPlus(self):
        raise NotImplementedError("IFPlus")
    def disabled_test_IFRangeGenerate(self):
        raise NotImplementedError("IFRangeGenerate")
    def disabled_test_IFRBuild(self):
        raise NotImplementedError("IFRBuild")
    def disabled_test_IFRElements(self):
        raise NotImplementedError("IFRElements")
    def disabled_test_IFRReplace(self):
        raise NotImplementedError("IFRReplace")
    def disabled_test_IFRedLeft(self):
        raise NotImplementedError("IFRedLeft")
    def disabled_test_IFRedRight(self):
        raise NotImplementedError("IFRedRight")
    def disabled_test_IFRedTree(self):
        raise NotImplementedError("IFRedTree")
    def disabled_test_IFReduce(self):
        raise NotImplementedError("IFReduce")
    def disabled_test_IFRestValues(self):
        raise NotImplementedError("IFRestValues")
    def disabled_test_IFSingle(self):
        raise NotImplementedError("IFSingle")
    def disabled_test_IFTimes(self):
        raise NotImplementedError("IFTimes")
    def disabled_test_IFTrunc(self):
        raise NotImplementedError("IFTrunc")
    def disabled_test_IFPrefixSize(self):
        raise NotImplementedError("IFPrefixSize")
    def disabled_test_IFError(self):
        raise NotImplementedError("IFError")
    def disabled_test_IFReplaceMulti(self):
        raise NotImplementedError("IFReplaceMulti")
    def disabled_test_IFConvert(self):
        raise NotImplementedError("IFConvert")
    def disabled_test_IFCallForeign(self):
        raise NotImplementedError("IFCallForeign")
    def disabled_test_IFAElementN(self):
        raise NotImplementedError("IFAElementN")
    def disabled_test_IFAElementP(self):
        raise NotImplementedError("IFAElementP")
    def disabled_test_IFAElementM(self):
        raise NotImplementedError("IFAElementM")
