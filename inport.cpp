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
  Type.tp_richcompare = richcompare;
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

std::shared_ptr<type> inport::my_type() {
  // Literals harvest type directly
  if (literal.size()) {
    auto tp = weakliteral_type.lock();
    return tp;
  }

  // Full edges can ask the source
  if (weakport.use_count()) {
    auto op = weakport.lock();
    if (!op) return nullptr;
    return op->weaktype.lock();
  }

  // Edge was never connected
  return nullptr;
}

// ----------------------------------------------------------------------
// The type was set with the literal value otherwise
// we query the port
// ----------------------------------------------------------------------
PyObject* inport::get_type(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  try {
    auto tp = cxx->my_type();
    if (tp) return tp->lookup();
  } catch (PyObject*) {
  }
  Py_RETURN_NONE;
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

  // If it is None, we delete the edge
  if ( other == Py_None ) {
    return TODO("delete inedge");
  }

  // Some kind of literal?
  else if ( other == Py_True ) {
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
  }

  // An out port?
  else if ( PyObject_TypeCheck(other,&outport::Type) ) {
    auto op = reinterpret_cast<outport::python*>(other)->cxx;
    cxx->literal.clear();
    cxx->weakliteral_type.reset();
    cxx->weakport = op;

    Py_INCREF(self);
    return self;
  }

  // Something else?
  else {
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
  
  PyObject* T = PyDict_GetItemString(m->dict.borrow(),tname);
  if (!(T && PyObject_TypeCheck(T,&type::Type))) {
    return PyErr_Format(PyExc_AttributeError,"module does not define type %s",tname);
  }

  auto tp = reinterpret_cast<type::python*>(T)->cxx;
  cxx->literal = literal;
  cxx->weakliteral_type = tp;
  cxx->weakport.reset();

  Py_INCREF(self);
  return self;
}

long inport::port() {
  auto node = weaknode.lock();
  if (!node) return 0;
  for(auto x:node->inputs) {
    if (x.second.get() == this) return x.first;
  }
  return 0;
}

PyObject* inport::string(PyObject*) {
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

PyObject* inport::richcompare(PyObject* self,PyObject* other,int op) {
  // Only compare to other inports
  if (!PyObject_TypeCheck(other,&inport::Type)) return Py_NotImplemented;

  std::shared_ptr<inport> left;
  std::shared_ptr<inport> right;

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

inport::inport(python* self, PyObject* args,PyObject* kwargs)
{
  throw PyErr_Format(PyExc_TypeError,"Cannot create InPorts");
}

inport::inport(std::shared_ptr<nodebase> node)
  : weaknode(node)
{
  pragmas = PyDict_New();
  if (!pragmas) throw PyErr_Occurred();
}

PyNumberMethods inport::as_number;
PySequenceMethods inport::as_sequence;
