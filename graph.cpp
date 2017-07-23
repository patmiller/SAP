#include "graph.h"

PyTypeObject graph::Type;

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
