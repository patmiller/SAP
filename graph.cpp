#include "graph.h"
#include "inport.h"
#include "outport.h"
#include "module.h"
#include "type.h"

PyTypeObject graph::Type;

void graph::setup() {
}

PyTypeObject* graph::basetype() {
  return &node::Type;
}

char const* graph::doc = "TBD Graph";
long graph::flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;

PyObject* addgraph(PyObject* self,PyObject*) {
  return PyErr_Format(PyExc_TypeError,"invalid for graphs");
}

PyMethodDef graph::methods[] = {
  {(char*)"addnode",graph::addnode,METH_O,(char*)"TBD addnode"},
  {(char*)"addgraph",addgraph,METH_VARARGS,(char*)"Not valid for graphs"},
  {nullptr}
};

PyObject* graph::addnode(PyObject* self, PyObject* arg) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  
  long opcode = PyInt_AsLong(arg);
  if (opcode < 0 && PyErr_Occurred()) return nullptr;

  auto N = std::make_shared<node>(opcode,cxx);

  PyOwned node(N->package());
  if (!node) return nullptr;

  if (PyList_Append(cxx->children.borrow(),node.borrow()) < 0) {
    return nullptr;
  }

  return node.incref();
}

PyGetSetDef graph::getset[] = {
  {(char*)"name",graph::get_name,graph::set_name,(char*)"TBD",nullptr},
  {(char*)"type",graph::get_type,nullptr,(char*)"TBD",nullptr},
  {(char*)"nodes",graph::get_nodes,nullptr,(char*)"TBD",nullptr},
  {(char*)"if1",graph::get_if1,nullptr,(char*)"TBD",nullptr},
  {nullptr}
};

PyObject* graph::get_name(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  std::string& name = cxx->name;
  if (name.size() == 0) Py_RETURN_NONE;
  return PyString_FromStringAndSize(name.c_str(),name.size());
}
int graph::set_name(PyObject* self,PyObject* name,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  if (name == nullptr || name == Py_None) {
    cxx->name = "";
    return 0;
  }
  if (!PyString_Check(name)) {
    PyErr_SetString(PyExc_TypeError,"name must be a string");
    return -1;
  }
  cxx->name = PyString_AS_STRING(name);
  return 0;
}

PyObject* graph::my_type() {
  STATIC_STR(ADDTYPECHAIN,"addtypechain");
  STATIC_STR(ADDTYPE,"addtype");
  static PyObject* pyIF_Function = nullptr;
  if (!pyIF_Function) {
    pyIF_Function = PyInt_FromLong(IF_Function);
    if (!pyIF_Function) return nullptr;
  }

  auto m = weakmodule.lock();
  if (!m) Py_RETURN_NONE;

  // Collect output types from the input edges (reversal!)
  // If no edges are wired in, we have no output type
  ssize_t outarity = inputs.size();
  if (outarity == 0) Py_RETURN_NONE;
  
  PyOwned outs(PyTuple_New(outarity));
  if (!outs) return nullptr;
  for(ssize_t port=1; port <= outarity; ++port) {
    auto p = inputs.find(port);
    if (p == inputs.end()) {
      return PyErr_Format(PyExc_ValueError,"function graph is missing an edge flowing to port %zd",port);
    }
    auto T = p->second->my_type();
    auto P = T->lookup();
    Py_INCREF(P);
    PyTuple_SET_ITEM(outs.borrow(),port-1,P);
  }

  PyOwned pm(m->package());
  if (!pm) return nullptr;
  PyOwned atc(PyObject_GetAttr(pm.borrow(),ADDTYPECHAIN));
  if (!atc) return nullptr;
  PyOwned otup(PyObject_Call(atc.borrow(),outs.borrow(),nullptr));
  if (!otup) return nullptr;

  ssize_t inarity = outputs.size();
  PyOwned ins(PyTuple_New(inarity));
  if (!ins) return nullptr;
  for(ssize_t port=1; port <= inarity; ++port) {
    auto p = outputs.find(port);
    if (p == outputs.end()) {
      return PyErr_Format(PyExc_ValueError,"function graph is missing an edge flowing from port %zd",port);
    }
    auto T = p->second->weaktype.lock();
    if (!T) {
      return PyErr_Format(PyExc_ValueError,"function graph with disconnected type at  port %zd",port);
    }
    auto P = T->lookup();
    Py_INCREF(P);
    PyTuple_SET_ITEM(ins.borrow(),port-1,P);
  }
  PyOwned itup(PyObject_Call(atc.borrow(),ins.borrow(),nullptr));
  if (!itup) return nullptr;
  
  PyOwned FT(
	     PyObject_CallMethodObjArgs(pm.borrow(),
					ADDTYPE,
					pyIF_Function,
					itup.borrow(),
					otup.borrow(),
					nullptr));
  if (!FT) return nullptr;
  return FT.incref();
}

PyObject* graph::get_type(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return cxx->my_type();
}

PyObject* graph::get_nodes(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return cxx->children.incref();
}
PyObject* graph::get_if1(PyObject* self,void*) {
  STATIC_STR(NL,"\n");
  STATIC_STR(IF1,"if1");

  auto cxx = reinterpret_cast<python*>(self)->cxx;

  // Pick the tag to use
  char tag = 'G';
  switch(cxx->opcode) {
  case IFXGraph: tag = 'X'; break;
  case IFIGraph: tag = 'I'; break;
  default: ;
  }

  // Compute the type for this graph
  PyOwned graphtype(cxx->my_type());
  if (!graphtype) return nullptr;
  long typeno = 0;
  if (graphtype.borrow() != Py_None) {
    typeno = reinterpret_cast<type::python*>(graphtype.borrow())->cxx->operator long();
  }

  PyOwned result;
  if (cxx->name.size()) {
    result = PyString_FromFormat("%c %ld \"%s\"",
				 tag,typeno,cxx->name.c_str());
  } else {
    result = PyString_FromFormat("%c %ld",tag,typeno);
  }
  if (!result) return nullptr;

  // Pragmas on the node
  PyOwned prags(pragma_string(cxx->pragmas.borrow()));
  if (!prags) return nullptr;

  // We also need the input edges (we are label 0)
  PyOwned edges(cxx->edge_if1(0L));
  if (!edges) return nullptr;
  
  PyString_Concat(result.addr(),prags.borrow());
  if (!result) return nullptr;
  PyString_Concat(result.addr(),edges.borrow());
  if (!result) return nullptr;

  // Finally, all the nodes have to be emitted
  for(ssize_t i=0;i<PyList_GET_SIZE(cxx->children.borrow()); ++i) {
    PyString_Concat(result.addr(),NL);
    PyObject* T = PyList_GET_ITEM(cxx->children.borrow(),i);
    PyObject* N_if1 = PyObject_GetAttr(T,IF1);
    if (!N_if1) return nullptr;
    PyString_ConcatAndDel(result.addr(),N_if1);
  }

  return result.incref();
}

PyObject* graph::string(PyObject*) {
  return nodebase::string();
}

graph::operator long() {
  return 0;
}

PyObject* graph::lookup() {
  // If we have a module, we're a top level function
  auto module = weakmodule.lock();
  if (module) {
    PyObject* functions = module->functions.borrow();
    for(ssize_t i=0;i<PyList_GET_SIZE(functions);++i) {
      auto P = PyList_GET_ITEM(functions,i);
      auto F = reinterpret_cast<graph::python*>(P);
      if (F->cxx.get() == this) {
	Py_INCREF(P);
	return P;
      }
    }
  }

  // Must be a subgraph of a compound
  auto parent = weakparent.lock();
  if (!parent) return PyErr_Format(PyExc_RuntimeError,"disconnected");
  for(ssize_t i=0;i<PyList_GET_SIZE(parent->children.borrow());++i) {
    PyObject* G = PyList_GET_ITEM(parent->children.borrow(),i);
    if (PyObject_TypeCheck(G,&graph::Type) &&
	reinterpret_cast<graph::python*>(G)->cxx.get() == this) {
      Py_INCREF(G);
      return G;
    }
  }
      
  return PyErr_Format(PyExc_RuntimeError,"disconnected");
}

graph::graph(python* self, PyObject* args,PyObject* kwargs)
{
  throw PyErr_Format(PyExc_TypeError,"Cannot create new graphes this way.  see Module.addfunction() or Node.addgraph()");
}

graph::graph(long opcode,
	     std::shared_ptr<module> module,
	     std::string const& name)
  : nodebase(opcode),
    weakmodule(module),
    name(name)
{
}

graph::graph(long opcode,
	     std::shared_ptr<nodebase> parent) 
  : nodebase(opcode,parent)
{
}

PyNumberMethods graph::as_number;
PySequenceMethods graph::as_sequence;
