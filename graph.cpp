#include "graph.h"
#include "inport.h"
#include "module.h"
#include "type.h"

PyTypeObject graph::Type;

void graph::setup() {
  Type.tp_call = graph::call;
}

PyObject* graph::call(PyObject* self,PyObject* args,PyObject* kwargs) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;

  long port;
  char* keywords[] = {(char*)"port",nullptr};
  if (!PyArg_ParseTupleAndKeywords(args,kwargs,"l",keywords,&port)) return nullptr;
  if (port <= 0) {
    return PyErr_Format(PyExc_TypeError,"invalid port %ld",port);
  }

  // We either have a port here or we must make one
  auto p = cxx->inputs.find(port);
  std::shared_ptr<inport> E;
  if (p != cxx->inputs.end()) {
    E = p->second;
  }
  else {
    E = cxx->inputs[port] = std::make_shared<inport>(cxx);
  }

  return E->package();
}

PyTypeObject* graph::basetype() {
  return &node::Type;
}

char const* graph::doc = "TBD Graph";
long graph::flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;

PyMethodDef graph::methods[] = {
  {nullptr}
};

PyGetSetDef graph::getset[] = {
  {(char*)"name",graph::get_name,graph::set_name,(char*)"TBD",nullptr},
  {(char*)"type",graph::get_type,nullptr,(char*)"TBD",nullptr},
  {(char*)"nodes",graph::get_nodes,nullptr,(char*)"TBD",nullptr},
  {(char*)"if1",graph::get_if1,nullptr,(char*)"TBD",nullptr},
  {nullptr}
};

PyObject* graph::get_name(PyObject* self,void*) {
  return TODO("get name");
}
int graph::set_name(PyObject* self,PyObject*,void*) {
  TODO("set name");
  return -1;
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
    return TODO("function inputs");
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
  return TODO("get nodes");
}
PyObject* graph::get_if1(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;

  // TODO: Have to deal with I graphs
  char tag = (cxx->opcode == IFXGraph)?'X':'G';

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

  return result.incref();
}

PyObject* graph::string(PyObject*) {
  return nodebase::string();
}

graph::operator long() {
  throw TODO("long");
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
  return TODO("graph lookup");
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

PyNumberMethods graph::as_number;
PySequenceMethods graph::as_sequence;
