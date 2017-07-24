#include "inport.h"
#include "node.h"
#include "type.h"

long inport::flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES;

PyTypeObject inport::Type;

char const* inport::doc = "TBD Inport";

void inport::setup() {
  as_number.nb_int = (unaryfunc)get_port;
  as_number.nb_lshift = lshift;
}

PyMethodDef inport::methods[] = {
  {nullptr}
};

PyGetSetDef inport::getset[] = {
  {(char*)"dst",inport::get_dst,nullptr,(char*)"TBD dst",nullptr},
  {(char*)"port",inport::get_port,nullptr,(char*)"TBD port",nullptr},
  {(char*)"literal",inport::get_literal,nullptr,(char*)"TBD literal",nullptr},
  {(char*)"type",inport::get_type,nullptr,(char*)"TBD type",nullptr},
  {(char*)"src",inport::get_src,nullptr,(char*)"TBD src",nullptr},
  {(char*)"oport",inport::get_oport,nullptr,(char*)"TBD oport",nullptr},
  {(char*)"pragmas",inport::get_pragmas,nullptr,(char*)"TBD pragmas",nullptr},
  {nullptr}
};

PyObject* inport::get_dst(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  auto node = cxx->weaknode.lock();

  return node->lookup();
}

PyObject* inport::get_port(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return PyInt_FromLong(cxx->port());
}

// ----------------------------------------------------------------------
// If the literal value is set, return it.  Otherwise None
// ----------------------------------------------------------------------
PyObject* inport::get_literal(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  if (cxx->literal.size() == 0) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  return PyString_FromStringAndSize(cxx->literal.c_str(),
				    cxx->literal.size());
}

// ----------------------------------------------------------------------
// The type was set with the literal value otherwise
// we query the port
// ----------------------------------------------------------------------
PyObject* inport::get_type(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  if (cxx->literal.size() == 0) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  auto type = cxx->weakliteral_type.lock();
  if (!type) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  return type->lookup();
}

PyObject* inport::get_src(PyObject* self,void*) {
  Py_INCREF(Py_None);
  return Py_None;
}
PyObject* inport::get_oport(PyObject* self,void*) {
  Py_INCREF(Py_None);
  return Py_None;
}
PyObject* inport::get_pragmas(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return cxx->pragmas.incref();
}

PyObject* inport::lshift(PyObject* self, PyObject* other) {
  return PyInt_FromLong(1234);
  puts("HERE");
  return PyErr_Format(PyExc_NotImplementedError,"lshift");
}

long inport::port() {
  auto node = weaknode.lock();
  if (!node) return 0;
  for(auto x:node->inputs) {
    if (x.second.get() == this) return x.first;
  }
  return 0;
}

PyObject* inport::string(PyObject* self) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  auto node = weaknode.lock();
  if (!node) return PyErr_Format(PyExc_RuntimeError,"disconnected");

  PyOwned s(node->string());
  if (!s) return nullptr;

  return PyString_FromFormat("%ld:%s",port(),PyString_AS_STRING(s.borrow()));
}

inport::inport(python* self, PyObject* args,PyObject* kwargs)
{
  throw PyErr_Format(PyExc_TypeError,"Cannot create InPorts");
}

inport::inport(std::shared_ptr<nodebase> node)
  : weaknode(node),oport(-1)
{
  pragmas = PyDict_New();
  if (!pragmas) throw PyErr_Occurred();
}

PyNumberMethods inport::as_number;
PySequenceMethods inport::as_sequence;
