#include "nodebase.h"
#include "graph.h"

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

PyObject* nodebase::lookup() {
  return PyErr_Format(PyExc_NotImplementedError,"node lookup");
}

std::shared_ptr<graph> nodebase::my_graph() {
  auto sp = std::dynamic_pointer_cast<graph>(weakparent.lock());
  if (!sp) throw PyErr_Format(PyExc_RuntimeError,"disconnected");
  return sp;
}


