#include "outport.h"
#include "type.h"
#include "nodebase.h"

void outport::setup() {
  as_number.nb_int = (unaryfunc)get_port;
  Type.tp_richcompare = richcompare;
}

PyTypeObject outport::Type;

PyObject* outport::string(PyObject*) {
  auto node = weaknode.lock();
  if (!node) return PyErr_Format(PyExc_RuntimeError,"disconnected");
  PyOwned base(node->string());
  long port = operator long();
  return PyString_FromFormat("%s:%ld",PyString_AS_STRING(base.borrow()),port);
}

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
    {(char*)"foffset",outport::get_foffset,nullptr,(char*)"TBD foffset",nullptr},
  {nullptr}
};

PyObject* outport::get_node(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  auto np = cxx->weaknode.lock();
  if (!np) {
    return PyErr_Format(PyExc_RuntimeError,"disconnected type");
  }
  return np->lookup();
}

PyObject* outport::get_foffset(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return PyInt_FromLong(cxx->foffset);
}

PyObject* outport::get_port(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  long port = cxx->operator long();
  if (!port) return PyErr_Format(PyExc_RuntimeError,"disconnected");
  return PyInt_FromLong(port);
}

PyObject* outport::get_type(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  auto tp = cxx->weaktype.lock();
  if (!tp) {
    return PyErr_Format(PyExc_RuntimeError,"disconnected type");
  }
  return tp->lookup();
}
PyObject* outport::get_edges(PyObject*,void*) {
  return TODO("edges");
}
PyObject* outport::get_pragmas(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return cxx->pragmas.incref();
}

outport::operator long() {
  auto n = weaknode.lock();
  if (!n) return 0;
  for(auto x:n->outputs) {
    if (x.second.get() == this) return x.first;
  }
  return 0;
}

PyObject* outport::richcompare(PyObject* self,PyObject* other,int op) {
  // Only compare to other outports
  if (!PyObject_TypeCheck(other,&outport::Type)) return Py_NotImplemented;

  std::shared_ptr<outport> left;
  std::shared_ptr<outport> right;

  PyObject* yes = Py_True;
  PyObject* no = Py_False;

  switch(op) {
  case Py_LT:
  case Py_LE:
  case Py_GT:
  case Py_GE:
    return Py_NotImplemented;
  case Py_NE:
    yes = Py_False;
    no = Py_True;
  case Py_EQ:
    left = reinterpret_cast<python*>(self)->cxx;
    right = reinterpret_cast<python*>(other)->cxx;
    if (left.get() == right.get()) return yes;
    return no;
  }
  return nullptr;
}

outport::outport(python* self, PyObject* args,PyObject* kwargs)
  : pragmas(nullptr)
{
}

PyNumberMethods outport::as_number;
PySequenceMethods outport::as_sequence;
