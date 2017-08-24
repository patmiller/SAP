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
  {(char*)"foffset",inport::get_foffset,nullptr,(char*)"TBD foffset"},
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
  return TODO("src");
}

PyObject* inport::get_oport(PyObject* self,void*) {
  return TODO("oport");
}

PyObject* inport::get_foffset(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return PyInt_FromLong(cxx->foffset);
}

PyObject* inport::get_pragmas(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return cxx->pragmas.incref();
}

PyObject* inport::cross_graph(std::shared_ptr<outport> out,
			      std::shared_ptr<nodebase> src,
			      std::shared_ptr<graph> src_g,
			      std::shared_ptr<inport> in,
			      std::shared_ptr<nodebase> dst,
			      std::shared_ptr<graph> dst_g) {
  auto wire = out;
  
  // See if we can "climb" up from the dst to the src_g
  std::vector<std::shared_ptr<nodebase>> path;
  bool good = false;
  for(std::shared_ptr<nodebase> p=dst_g;p;p=p->weakparent.lock()) {
    if (p.get() == src_g.get()) { good = true; break; }
    path.push_back(p);
  }
  if (!good || path.size()%2) return DISCONNECTED;

  // At this point, the path should look like:
  // { G,C,  G,C,   G,C... G,C }
  // where each G is an inner graph and each C is a compound
  // node along the path from the source to the destination
  // So, we simply have find a free port for each compound,
  // mirror it to each of its subgraphs, and create the
  // wire
  PyOwned portpath(PyTuple_New(path.size()+1));
  ssize_t nport = 0;
  for(auto it=path.rbegin(); it != path.rend(); it += 2) {
    auto C = *it;
    auto G = *(it+1);

    // Find the first free port in the compound (and children)
    long lastport = 0;
    auto last = C->inputs.rbegin();
    if (last != C->inputs.rend()) {
      auto mx = last->first;
      if (mx > lastport) lastport = mx;
    }
    std::vector<std::shared_ptr<graph>> subgraphs;
    for(ssize_t i=0;i<PyList_GET_SIZE(C->children.borrow());++i) {
      auto p = PyList_GET_ITEM(C->children.borrow(),i);
      if (!PyObject_TypeCheck(p,&graph::Type)) continue;
      auto subg = reinterpret_cast<graph::python*>(p)->cxx;
      subgraphs.push_back(subg);
      auto last = subg->outputs.rbegin();
      if (last != subg->outputs.rend()) {
	auto mx = last->first;
	if (mx > lastport) lastport = mx;
      }
    }

    // The next available port
    long freeport = lastport+1;

    // Create an inport on the compound there and
    // forward the last outport here
    auto port = C->inputs[freeport] = std::make_shared<inport>(C);
    PyTuple_SET_ITEM(portpath.borrow(),nport++,port->package());
    port->literal.clear();
    port->weakliteral_type.reset();
    port->weakport = wire;

    // Create outports with the correct type at ALL subgraphs
    // The wire will continue from only one of these graphs
    for(auto sg:subgraphs) {
      auto oport = sg->outputs[freeport] = std::make_shared<outport>(sg);
      oport->weaktype = wire->weaktype;
      if (sg == G) {
	wire = oport;
	PyTuple_SET_ITEM(portpath.borrow(),nport++,oport->package());
      }
    }
    
  }

  // now make the final wire connection to the destination
  in->literal.clear();
  in->weakliteral_type.reset();
  in->weakport = wire;
  PyTuple_SET_ITEM(portpath.borrow(),nport++,in->package());
  return portpath.incref();
}


PyObject* inport::lshift(PyObject* self, PyObject* other) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  std::string literal;
  std::shared_ptr<type> T;

  auto lookup = [cxx](const char* tname) {
    std::shared_ptr<type> result;
    // Get the module
    std::shared_ptr<module> m;
    try {
      auto node = cxx->my_node();
      auto graph = node->my_graph();
      m = graph->my_module();
    } catch (PyObject*) {
      return result;
    }

    // Find it in the module dictionary
    PyObject* T = PyDict_GetItemString(m->dict.borrow(),tname);
    if (!(T && PyObject_TypeCheck(T,&type::Type))) {
      PyErr_Format(PyExc_AttributeError,"module does not define type %s",tname);
      return result;
    }

    if (!PyObject_TypeCheck(T,&type::Type)) {
      PyErr_Format(PyExc_ValueError,"%s is not a type is this module",tname);
      return result;
    }
    
    auto tp = reinterpret_cast<type::python*>(T)->cxx;
    return tp;
  };

  // If it is None, we delete the edge
  literal_parser_state_t flavor = ERROR;
  if ( other == Py_None ) {
    return TODO("delete inedge");
  }

  // Some kind of literal?
  else if ( other == Py_True ) {
    flavor = BOOLEAN_LITERAL;
    T = lookup("boolean");
  } else if ( other == Py_False ) {
    flavor = BOOLEAN_LITERAL;
    T = lookup("boolean");
  } else if ( PyInt_Check(other) ) {
    PyOwned s(PyObject_Str(other));
    if (!s) return nullptr;
    literal = PyString_AS_STRING(s.borrow());
    T = lookup("integer");
  } else if ( PyFloat_Check(other) ) {
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

    T = lookup("doublereal");
  } else if ( PyString_Check(other) ) {
    flavor = parse_literal(PyString_AS_STRING(other));
    switch(flavor) {
    case BOOLEAN_LITERAL: T = lookup("boolean"); break;
    case CHAR_LITERAL: T = lookup("character"); break;
    case DOUBLEREAL_LITERAL: T = lookup("doublereal"); break;
    case INTEGER_LITERAL: T = lookup("integer"); break;
    case NULL_LITERAL: T = lookup("null"); break;
    case REAL_LITERAL: T = lookup("real"); break;
    case STRING_LITERAL: T = lookup("string"); break;
    case ERROR:
    default:
      return PyErr_Format(PyExc_ValueError,"invalid literal %s",PyString_AS_STRING(other));
    }
    literal = PyString_AS_STRING(other);
  } else if ( PyObject_TypeCheck(other,&graph::Type) ) {
    auto function = reinterpret_cast<graph::python*>(other)->cxx;
    literal = function->name.c_str();

    PyOwned PT(function->my_type());
    if (!(PT && PyObject_TypeCheck(PT.borrow(),&type::Type))) {
      PyErr_Format(PyExc_ValueError,"%s does not have a type",literal.c_str());
      return nullptr;
    }
    
    T = reinterpret_cast<type::python*>(PT.borrow())->cxx;
  }

  // An out port?
  else if ( PyObject_TypeCheck(other,&outport::Type) ) {
    auto out = reinterpret_cast<outport::python*>(other)->cxx;
    auto src = out->weaknode.lock();
    if (!src) return DISCONNECTED;

    auto dst = cxx->weaknode.lock();
    if (!dst) return DISCONNECTED;
    
    // We can only wire within the same graph.  If the
    // source lives in another graph, we have to wire
    // our way down to the compound.
    auto src_graph = src->my_graph();
    auto dst_graph = dst->my_graph();
    if (src_graph.get() != dst_graph.get()) {
      return cross_graph(out,src,src_graph,
			 cxx,dst,dst_graph);
    }
    
    cxx->literal.clear();
    cxx->weakliteral_type.reset();
    cxx->weakport = out;

    Py_INCREF(self);
    return self;
  }

  // Something else?
  else {
    return (Py_INCREF(Py_NotImplemented),Py_NotImplemented);
  }

  cxx->literal = literal;
  cxx->weakliteral_type = T;
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
  if (!PyObject_TypeCheck(other,&inport::Type)) return (Py_INCREF(Py_NotImplemented),Py_NotImplemented);

  std::shared_ptr<inport> left;
  std::shared_ptr<inport> right;

  PyObject* yes = Py_True;
  PyObject* no = Py_False;

  switch(op) {
  case Py_LT:
  case Py_LE:
  case Py_GT:
  case Py_GE:
    return (Py_INCREF(Py_NotImplemented),Py_NotImplemented);
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
