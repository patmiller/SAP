#include "nodebase.h"
#include "module.h"
#include "graph.h"
#include "type.h"
#include "inport.h"
#include "outport.h"

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
  // Get a handle to our module and use that to look up
  // the opname
  auto m = my_module();
  auto name = m?m->lookup(opcode):"";
  if (name.size() == 0) {
    return PyString_FromFormat("<Node %ld>",opcode);
  }
  return PyString_FromString(name.c_str());
}

nodebase::operator long() {
  auto parent = weakparent.lock();
  if (!parent) return 0;

  for(ssize_t i=0;i<PyList_GET_SIZE(parent->children.borrow());++i) {
    PyObject* N = PyList_GET_ITEM(parent->children.borrow(),i);
    if (!PyObject_TypeCheck(N,&node::Type)) continue;
    auto np = reinterpret_cast<node::python*>(N)->cxx;
    if (np.get() == this) return i+1;
  }
  puts("miss");
  return 0;
}

PyObject* nodebase::lookup() {
  // Plain nodes here must be inside a graph (the parent)
  auto np = weakparent.lock();
  if (!np) return PyErr_Format(PyExc_RuntimeError,"disconnected");
  auto gp = std::dynamic_pointer_cast<graph>(np);
  if (!gp) return PyErr_Format(PyExc_RuntimeError,"disconnected");
  for(ssize_t i=0;i<PyList_GET_SIZE(gp->children.borrow());++i) {
    auto N = PyList_GET_ITEM(gp->children.borrow(),i);
    if (reinterpret_cast<node::python*>(N)->cxx.get() == this) {
      Py_INCREF(N);
      return N;
    }
  }
  return PyErr_Format(PyExc_RuntimeError,"disconnected");
}

std::shared_ptr<graph> nodebase::my_graph() {
  auto sp = std::dynamic_pointer_cast<graph>(weakparent.lock());
  if (!sp) throw PyErr_Format(PyExc_RuntimeError,"disconnected");
  return sp;
}

std::shared_ptr<module> nodebase::my_module() {
  auto p = this;
  while(1) {
    auto next = p->weakparent.lock();
    if (!next) break;
    p = next.get();
  }
  // Unless disconnected, we've climbed to the top function graph
  auto g = dynamic_cast<graph*>(p);
  if (!g) return nullptr;
  return g->weakmodule.lock();
}

PyObject* nodebase::edge_if1(long label) {
  PyOwned if1(PyString_FromString(""));
  // Any inputs?
  for(auto x:inputs) {
    long port = x.first;
    auto& input = *x.second;

    PyOwned aux;

    if (input.weakport.use_count() > 0) {
      auto out = input.weakport.lock();
      if (!out) return PyErr_Format(PyExc_RuntimeError,"disconnected");
      auto tp = out->weaktype.lock();
      if (!tp) continue;
      auto np = out->weaknode.lock();
      if (!np) continue;

      long slabel = np->operator long();
      long oport = out->operator long();
      long typeno = tp->operator long();
      auto eif1 = PyString_FromFormat("\nE %ld %ld %ld %ld %ld",
				      slabel,
				      oport,
				      label,
				      port,
				      typeno);
      if (!eif1) return nullptr;
      PyString_ConcatAndDel(if1.addr(),eif1);

      // Add any pragma strings (merging input and output sides)
      PyObject* prags = inport::pragma_string(input.pragmas.borrow(),out->pragmas.borrow());
      if (!prags) return nullptr;
      PyString_ConcatAndDel(if1.addr(),prags);

    } else {
      auto tp = input.weakliteral_type.lock();
      if (!tp) continue;
      long typeno = tp->operator long();
      auto eif1 = PyString_FromFormat("\nL     %ld %ld %ld \"%s\"",label,port,typeno,input.literal.c_str());
      if (!eif1) return nullptr;
      PyString_ConcatAndDel(if1.addr(),eif1);

      // Add any pragma strings (input side only)
      PyObject* prags = inport::pragma_string(input.pragmas.borrow());
      if (!prags) return nullptr;
      PyString_ConcatAndDel(if1.addr(),prags);
    }


  }
  return if1.incref();
}

