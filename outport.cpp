#include "outport.h"

PyTypeObject outport::Type;

char const* outport::doc = "TBD Outport";

PyMethodDef outport::methods[] = {
  {nullptr}
};

PyGetSetDef outport::getset[] = {
  {(char*)"node",outport::get_node,nullptr,(char*)"TBD node",nullptr},
  {(char*)"port",outport::get_port,nullptr,(char*)"TBD port",nullptr},
  {(char*)"type",outport::get_type,nullptr,(char*)"TBD type",nullptr},
  {(char*)"edges",outport::get_edges,nullptr,(char*)"TBD edges",nullptr},
  {(char*)"pragmas",outport::get_pragmas,nullptr,(char*)"TBD pragmas",nullptr},
  {nullptr}
};

PyObject* outport::get_node(PyObject*,void*) {
  return TODO("node");
}
PyObject* outport::get_port(PyObject*,void*) {
  return TODO("port");
}
PyObject* outport::get_type(PyObject*,void*) {
  return TODO("type");
}
PyObject* outport::get_edges(PyObject*,void*) {
  return TODO("edges");
}
PyObject* outport::get_pragmas(PyObject*,void*) {
  return TODO("pragmas");
}

outport::outport(python* self, PyObject* args,PyObject* kwargs)
  : pragmas(nullptr)
{
}

PyNumberMethods outport::as_number;
PySequenceMethods outport::as_sequence;
