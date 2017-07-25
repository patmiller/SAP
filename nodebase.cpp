#include "nodebase.h"
#include "graph.h"
#include "type.h"
#include "inport.h"

nodebase::nodebase(long opcode,std::shared_ptr<nodebase> parent)
  : opcode(opcode),
    weakparent(parent)
{
  children = PyList_New(0);
  if (!children) throw PyErr_Occurred();

  pragmas = PyDict_New();
  if (!pragmas) throw PyErr_Occurred();
}

PyObject* nodebase::string() {
  auto p = opcode_to_name.find(opcode);
  if (p == opcode_to_name.end()) {
    return PyString_FromFormat("<Node %ld>",opcode);
  }
  return PyString_FromString(p->second.c_str());
}

nodebase::operator long() {
  throw TODO("long");
}

PyObject* nodebase::lookup() {
  return TODO("node lookup");
}

std::shared_ptr<graph> nodebase::my_graph() {
  auto sp = std::dynamic_pointer_cast<graph>(weakparent.lock());
  if (!sp) throw PyErr_Format(PyExc_RuntimeError,"disconnected");
  return sp;
}

PyObject* nodebase::edge_if1(long label) {
  PyOwned if1(PyString_FromString(""));
  // Any inputs?
  for(auto x:inputs) {
    long port = x.first;
    auto& input = *x.second;

    if (input.oport > 0) {
      return TODO("finish");
    } else {
      auto tp = input.weakliteral_type.lock();
      if (!tp) continue;
      long typeno = tp->operator long();
      auto eif1 = PyString_FromFormat("\nL     %ld %ld %ld \"%s\"",label,port,typeno,input.literal.c_str());
      if (!eif1) return nullptr;
      PyString_ConcatAndDel(if1.addr(),eif1);
      PyObject* prags = inport::pragma_string(input.pragmas.borrow());
      if (!prags) return nullptr;
      PyString_ConcatAndDel(if1.addr(),prags);
    }
  }
  return if1.incref();
}

