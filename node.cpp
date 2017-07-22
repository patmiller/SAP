#include "node.h"

PyTypeObject node::Type;

char const* node::doc = "TBD Node";

PyMethodDef node::methods[] = {
  {nullptr}
};

PyGetSetDef node::getset[] = {
  {(char*)"opcode",node::get_opcode,node::set_opcode,(char*)"TBD",nullptr},
  {(char*)"children",node::get_children,nullptr,(char*)"TBD",nullptr},
  {(char*)"pragmas",node::get_pragmas,nullptr,(char*)"TBD",nullptr},
  {(char*)"if1",node::get_if1,nullptr,(char*)"IF1 code for entire node",nullptr},
  {(char*)"inputs",node::get_inputs,nullptr,(char*)"TBD",nullptr},
  {(char*)"outputs",node::get_outputs,nullptr,(char*)"TBD",nullptr},
  {nullptr}
};

PyObject* node::get_opcode(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_opcode");
}
int node::set_opcode(PyObject* pySelf,PyObject*,void*) {
  PyErr_Format(PyExc_NotImplementedError,"set_if1");
  return -1;
}
PyObject* node::get_children(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_children");
}
PyObject* node::get_pragmas(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_pragmas");
}
PyObject* node::get_if1(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_if1");
}
PyObject* node::get_inputs(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_inputs");
}
PyObject* node::get_outputs(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_outputs");
}

node:: node(python* self, PyObject* args,PyObject* kwargs)
{
}

// TODO: chain c'tor
node:: node()
{
}

PyNumberMethods node::as_number;
PySequenceMethods node::as_sequence;
