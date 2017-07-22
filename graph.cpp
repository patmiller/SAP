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
  {(char*)"type",graph::get_type,graph::set_type,(char*)"TBD",nullptr},
  {(char*)"nodes",graph::get_nodes,nullptr,(char*)"TBD",nullptr},
  {nullptr}
};

PyObject* graph::get_name(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_name");
}
int graph::set_name(PyObject* pySelf,PyObject*,void*) {
  PyErr_Format(PyExc_NotImplementedError,"set_name");
  return -1;
}
PyObject* graph::get_type(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_type");
}
int graph::set_type(PyObject* pySelf,PyObject*,void*) {
  PyErr_Format(PyExc_NotImplementedError,"set_type");
  return -1;
}
PyObject* graph::get_nodes(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_nodes");
}

graph:: graph(python* self, PyObject* args,PyObject* kwargs)
{
}

PyNumberMethods graph::as_number;
PySequenceMethods graph::as_sequence;
