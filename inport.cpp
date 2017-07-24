#include "inport.h"
#include "outport.h"
#include "node.h"
#include "graph.h"
#include "module.h"
#include "type.h"
#include "parser.h"

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
  if (cxx->literal.size() == 0) Py_RETURN_NONE;
  return PyString_FromStringAndSize(cxx->literal.c_str(),
				    cxx->literal.size());
}

// ----------------------------------------------------------------------
// The type was set with the literal value otherwise
// we query the port
// ----------------------------------------------------------------------
PyObject* inport::get_type(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  if (cxx->literal.size() == 0) Py_RETURN_NONE;
  auto type = cxx->weakliteral_type.lock();
  if (!type) Py_RETURN_NONE;
  return type->lookup();
}

PyObject* inport::get_src(PyObject* self,void*) {
  Py_RETURN_NONE;
}
PyObject* inport::get_oport(PyObject* self,void*) {
  Py_RETURN_NONE;
}
PyObject* inport::get_pragmas(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return cxx->pragmas.incref();
}

PyObject* inport::lshift(PyObject* self, PyObject* other) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  literal_parser_state_t flavor = ERROR;
  std::string literal;

  // Boolean literal?
  if ( other == Py_True ) {
    flavor = BOOLEAN_LITERAL;
    literal = "true";
  } else if ( other == Py_False ) {
    flavor = BOOLEAN_LITERAL;
    literal = "false";
  } else if ( PyInt_Check(other) ) {
    flavor = INTEGER_LITERAL;
    PyOwned s(PyObject_Str(other));
    if (!s) return nullptr;
    literal = PyString_AS_STRING(s.borrow());
  } else if ( PyFloat_Check(other) ) {
    flavor = DOUBLEREAL_LITERAL;
    PyOwned s(PyObject_Str(other));
    if (!s) return nullptr;
    // Have to fix +eNN to dNN and -eNN to dNN
    PyOwned noplus(PyObject_CallMethod(s.borrow(),(char*)"replace",(char*)"ss","+",""));
    if (!noplus) return nullptr;
    PyOwned noe(PyObject_CallMethod(noplus.borrow(),(char*)"replace",(char*)"ss","e","d"));
    if (!noe) return nullptr;
    PyOwned noE(PyObject_CallMethod(noe.borrow(),(char*)"replace",(char*)"ss","E","d"));
    if (!noE) return nullptr;
    literal = PyString_AS_STRING(noE.borrow());
    // Make sure there is a D in there somewhere
    if (literal.find('d') == literal.npos) literal += 'd';
  } else if ( PyString_Check(other) ) {
    flavor = parse_literal(PyString_AS_STRING(other));
    if (flavor == ERROR) {
      return PyErr_Format(PyExc_ValueError,"invalid literal %s",PyString_AS_STRING(other));
    }
    literal = PyString_AS_STRING(other);
  } else if ( PyObject_TypeCheck(other,&outport::Type) ) {
    return PyErr_Format(PyExc_ValueError,"gotta fix output port lshift");
  } else {
    return Py_NotImplemented;
  }

  // If I reach here, I have a literal
  std::shared_ptr<module> m;
  try {
    auto node = cxx->my_node();
    auto graph = node->my_graph();
    m = graph->my_module();
  } catch (PyObject*) {
    return nullptr;
  }
  const char* tname = nullptr;
  switch(flavor) {
  case BOOLEAN_LITERAL:
    tname = "boolean";
    break;
  case CHAR_LITERAL:
    tname = "character";
    break;
  case DOUBLEREAL_LITERAL:
    tname = "doublereal";
    break;
  case INTEGER_LITERAL:
    tname = "integer";
    break;
  case NULL_LITERAL:
    tname = "null";
    break;
  case REAL_LITERAL:
    tname = "real";
    break;
  case STRING_LITERAL:
    tname = "string";
    break;
  default:
    return PyErr_Format(PyExc_NotImplementedError,"parse code %d",flavor);
  }
  
  if (literal == "16.5") return nullptr;
  PyObject* T = PyDict_GetItemString(m->dict.borrow(),tname);
  if (!(T && PyObject_TypeCheck(T,&type::Type))) {
    return PyErr_Format(PyExc_AttributeError,"module does not define type %s",tname);
  }

  auto tp = reinterpret_cast<type::python*>(T)->cxx;
  cxx->literal = literal;
  cxx->weakliteral_type = tp;

  Py_INCREF(T);
  return T;
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

std::shared_ptr<nodebase> inport::my_node() {
  auto sp = weaknode.lock();
  if (!sp) throw PyErr_Format(PyExc_RuntimeError,"disconnected");
  return sp;
}

PyObject* inport::getattro(PyObject* self,PyObject* attr) {
  return PyObject_GenericGetAttr(self,attr);
}

int inport::setattro(PyObject* self,PyObject* attr,PyObject* rhs) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  if (PyDict_SetItem(cxx->pragmas.borrow(),attr,rhs) == 0) return 0;
  return PyObject_GenericSetAttr(self,attr,rhs);
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
