#include "graph.h"
#include "inport.h"
#include "module.h"

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
  return PyErr_Format(PyExc_NotImplementedError,"get_name");
}
int graph::set_name(PyObject* self,PyObject*,void*) {
  PyErr_Format(PyExc_NotImplementedError,"set_name");
  return -1;
}
PyObject* graph::get_type(PyObject* self,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_type");
}
PyObject* graph::get_nodes(PyObject* self,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_nodes");
}
PyObject* graph::get_if1(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;

  // TODO: Have to deal with I graphs
  char tag = (cxx->opcode == IFXGraph)?'X':'G';

  // TODO: make an automatic type for top levels
  long typeno = 0;

  PyOwned prefix;
  if (cxx->name.size()) {
    prefix = PyString_FromFormat("%c %ld \"%s\"",
				 tag,typeno,cxx->name.c_str());
  } else {
    prefix = PyString_FromFormat("%c %ld",tag,typeno);
  }
  if (!prefix) return nullptr;

  PyObject* prags = pragma_string(cxx->pragmas.borrow());
  if (!prags) return nullptr;
  
  PyObject* result = prefix.incref();
  PyString_Concat(&result,prags);
  return result;
}

PyObject* graph::string(PyObject*) {
  return nodebase::string();
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
  return PyErr_Format(PyExc_NotImplementedError,"graph lookup");
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
