#include "inport.h"

PyTypeObject inport::Type;

char const* inport::doc = "TBD Inport";

PyMethodDef inport::methods[] = {
  {nullptr}
};

PyGetSetDef inport::getset[] = {
  {(char*)"node",inport::get_node,nullptr,(char*)"TBD node",nullptr},
  {(char*)"port",inport::get_port,nullptr,(char*)"TBD port",nullptr},
  {(char*)"literal",inport::get_literal,nullptr,(char*)"TBD literal",nullptr},
  {(char*)"type",inport::get_type,nullptr,(char*)"TBD type",nullptr},
  {(char*)"src",inport::get_src,nullptr,(char*)"TBD src",nullptr},
  {(char*)"oport",inport::get_oport,nullptr,(char*)"TBD oport",nullptr},
  {(char*)"pragmas",inport::get_pragmas,nullptr,(char*)"TBD pragmas",nullptr},
  {nullptr}
};

PyObject* inport::get_node(PyObject*,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"node");
}
PyObject* inport::get_port(PyObject*,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"port");
}
PyObject* inport::get_literal(PyObject*,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"literal");
}
PyObject* inport::get_type(PyObject*,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"type");
}
PyObject* inport::get_src(PyObject*,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"src");
}
PyObject* inport::get_oport(PyObject*,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"oport");
}
PyObject* inport::get_pragmas(PyObject*,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"pragmas");
}

inport::inport(python* self, PyObject* args,PyObject* kwargs)
  : pragmas(nullptr)
{
}

PyNumberMethods inport::as_number;
PySequenceMethods inport::as_sequence;
