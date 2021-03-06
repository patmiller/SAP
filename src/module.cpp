#include "module.h"
#include "type.h"
#include "graph.h"
#include "inport.h"
#include "outport.h"

PyTypeObject module::Type;

char const* module::doc = "TBD Module";

void module::setup() {
  as_sequence.sq_item = item;
}

PyObject* module::literal_to_python(std::shared_ptr<inport> const &p) {
  // We use this in converting some strings to Python values
  static PyObject* literal_eval = nullptr;
  if (!literal_eval) {
    PyOwned ast(PyImport_ImportModule("ast"));
    if (!ast) return nullptr;
    literal_eval = PyObject_GetAttrString(ast.borrow(),"literal_eval");
    if (!literal_eval) return nullptr;
  }

  auto T = p->weakliteral_type.lock();
  if (!T) return DISCONNECTED;
  switch(T->code) {
  case IF_Array: {
    // Check subtype as char
    auto subtype = T->parameter1.lock();
    if (!subtype) return DISCONNECTED;
    if (subtype->code == IF_Basic && subtype->aux == IF_Character) {
      return PyObject_CallFunction(literal_eval,(char*)"s",p->literal.c_str());
    }
    return PyErr_Format(PyExc_RuntimeError,"Invalid array literal subtype: %s",p->literal.c_str());
  }
  case IF_Basic: {
    switch(T->aux) {
    case IF_Boolean: {
      if (p->literal.size() && (p->literal[0] == 't' || p->literal[0] == 'T')) {
	Py_INCREF(Py_True);
	return Py_True;
      }
      Py_INCREF(Py_False);
      return Py_False;
    }
    case IF_Character: {
      return PyObject_CallFunction(literal_eval,(char*)"s",p->literal.c_str());
    }
    case IF_DoubleReal: {
      std::string copy = p->literal;
      auto i = copy.find('d');
      if (i != std::string::npos) copy[i] = 'e';
      i = copy.find('D');
      if (i != std::string::npos) copy[i] = 'e';

      auto v = ::atof(copy.c_str());
      return PyFloat_FromDouble(v);
    }
    case IF_Integer: {
      auto v = ::atol(p->literal.c_str());
      return PyInt_FromLong(v);
    }
    case IF_Null: Py_RETURN_NONE;
    case IF_Real: {
      auto v = ::atof(p->literal.c_str());
      return PyFloat_FromDouble(v);
    }
    case IF_WildBasic: return TODO("WildBasic");
    default: return TODO("unknown basic");
    }
  }
  case IF_Function: {
    ssize_t nfunctions = PyList_GET_SIZE(functions.borrow());
    for(ssize_t i=0;i<nfunctions;++i) {
      PyObject* P = PyList_GET_ITEM(functions.borrow(),i);
      if (!PyObject_TypeCheck(P,&graph::Type)) continue; // Skip weird stuff
      auto F = reinterpret_cast<graph::python*>(P)->cxx;
      if (F->name == p->literal) {
	Py_INCREF(P);
	return P;
      }
    }
    return PyErr_Format(PyExc_KeyError,"no such function %s",p->literal.c_str());
  }
  default: ;
  }
    
  return PyErr_Format(PyExc_TypeError,"Invalid literal type %ld:%s",T->code,p->literal.c_str());
}

PyObject* module::interpret_node(PyObject* self, PyObject* interpreter, PyObject* node, PyObject* args) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  auto N = reinterpret_cast<node::python*>(node)->cxx;
  std::string opname = cxx->lookup(N->opcode);

  // We will fill the frame from the extra values
  // in args.  It starts at pos 2 and moves to the
  // size
  ssize_t argpos = 2;
  ssize_t lastpos = PyTuple_GET_SIZE(args);

  // The inputs to the method are the node and all its inputs
  PyOwned inputs(PyTuple_New(N->inputs.size()+2));
  if (!inputs) return nullptr;
  Py_INCREF(self);
  PyTuple_SET_ITEM(inputs.borrow(),0,self);
  Py_INCREF(node);
  PyTuple_SET_ITEM(inputs.borrow(),1,node);
  ssize_t i = 2;
  for(auto x:N->inputs) {
    x.second->foffset = i;
    if (x.second->literal.size()) {
      auto p = cxx->literal_to_python(x.second);
      if (!p) return nullptr;
      PyTuple_SET_ITEM(inputs.borrow(),i++,p);
      if (PyErr_Occurred()) return nullptr;
    } else if (argpos == lastpos) {
      return PyErr_Format(PyExc_TypeError,"not all inputs were set");
    } else {
      PyObject* P = PyTuple_GET_ITEM(args,argpos++);
      Py_INCREF(P);
      PyTuple_SET_ITEM(inputs.borrow(),i++,P);
    }
  }
  if (argpos != lastpos) {
    return PyErr_Format(PyExc_TypeError,"not all inputs were used");
  }

  PyOwned method(PyObject_GetAttrString(interpreter,opname.c_str()));
  if (!method) return nullptr;

  // Call the node method
  PyOwned outputs(PyObject_CallObject(method.borrow(),inputs.borrow()));

  // Single output?  Pack it into a tuple for standardization
  if (!PyTuple_Check(outputs.borrow())) {
    if (N->outputs.size() != 1) {
      return PyErr_Format(PyExc_TypeError,"Expected %zd outputs, got one",N->outputs.size());
    }
    PyOwned one(PyTuple_New(1));
    if (!one) return nullptr;
    PyTuple_SET_ITEM(one.borrow(),0,outputs.incref());
    outputs = one.incref();
  }
  if (N->outputs.size() != PyTuple_GET_SIZE(outputs.borrow())) {
    return PyErr_Format(PyExc_TypeError,"Expected %zd outputs, got %zd",N->outputs.size(),PyTuple_GET_SIZE(outputs.borrow()));
  }
  return outputs.incref();
}

PyObject* module::interpret_graph(PyObject* self, PyObject* interpreter, PyObject* node, PyObject* args) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  auto G = reinterpret_cast<graph::python*>(node)->cxx;
  
  // We will build a frame with all the inputs and outputs
  PyOwned frame(PyList_New(0));
  if (!frame) return nullptr;

  // We start with the graph outputs (function inputs)
  if (G->outputs.size() != PyTuple_GET_SIZE(args)-2) {
    return PyErr_Format(PyExc_TypeError,"graph expects %zd starting values, has %zd",
			G->outputs.size(),
			PyTuple_GET_SIZE(args)-2);
  }
  for(auto x:G->outputs) {
    ssize_t n = PyList_GET_SIZE(frame.borrow());
    x.second->foffset = n;
    PyObject* X = PyTuple_GET_ITEM(args,2+n);
    if (PyList_Append(frame.borrow(),X) != 0) return nullptr;
  }

  // A little lambda to deal with node inputs
  auto mod = G->my_module();
  if (!mod) return DISCONNECTED;
  auto add_to_frame = [mod](std::shared_ptr<nodebase> N,PyObject* F) {
    for(auto p:N->inputs) {
      if (p.second->literal.size()) {
	PyOwned lit(mod->literal_to_python(p.second));
	if (!lit) return reinterpret_cast<PyObject*>(0); // Arrgh!  C++11 lambda weirdness on return
	ssize_t n = PyList_GET_SIZE(F);
	p.second->foffset = n;
	if (PyList_Append(F,lit.borrow()) != 0) return reinterpret_cast<PyObject*>(0);
      } else {
	auto src = p.second->weakport.lock();
	if (!src) return DISCONNECTED;
	p.second->foffset = src->foffset;
      }
    }
    return Py_None;
  };

  // For every node, input edges refer to frame objects or literals (immediate frame fills)
  // output edges reserve an entry in the frame
  for(ssize_t i=0; i<PyList_GET_SIZE(G->children.borrow()); ++i) {
    PyObject* P = PyList_GET_ITEM(G->children.borrow(),i);
    if (!PyObject_TypeCheck(P,&node::Type)) continue; // ignore weird stuff
    auto N = reinterpret_cast<node::python*>(P)->cxx;

    // Inputs come from matching src or from the literal
    add_to_frame(N,frame.borrow());

    // Outputs are set to None in the frame
    for(auto p:N->outputs) {
      ssize_t n = PyList_GET_SIZE(frame.borrow());
      p.second->foffset = n;
      if (PyList_Append(frame.borrow(),Py_None) != 0) return nullptr;
    }
  }

  // Go through each node and execute it.  Outputs get directed to proper frame location
  for(ssize_t i=0; i<PyList_GET_SIZE(G->children.borrow()); ++i) {
    PyObject* P = PyList_GET_ITEM(G->children.borrow(),i);
    auto N = reinterpret_cast<node::python*>(P)->cxx;

    // Gather the inputs from the frame
    PyOwned args(PyTuple_New(N->inputs.size()+2));
    if (!args) return nullptr;

    ssize_t j = 0;
    Py_INCREF(self);
    PyTuple_SET_ITEM(args.borrow(),j++,self);
    Py_INCREF(P);
    PyTuple_SET_ITEM(args.borrow(),j++,P);
    for(auto p:N->inputs) {
      auto v = PyList_GET_ITEM(frame.borrow(),p.second->foffset);
      Py_INCREF(v);
      PyTuple_SET_ITEM(args.borrow(),j++,v);
    }
    std::string opname = cxx->lookup(N->opcode);
    PyOwned method(PyObject_GetAttrString(interpreter,opname.c_str()));
    if (!method) return nullptr;

    PyOwned out(PyObject_CallObject(method.borrow(),args.borrow()));
    if (!out) return nullptr;

    // If we get back a tuple, the size must match the number of outputs
    if ( PyTuple_Check(out.borrow()) ) {
      if (PyTuple_GET_SIZE(out.borrow()) != N->outputs.size()) {
	return PyErr_Format(PyExc_ValueError,"%s expected %zd returns, but returned tuple had %zd values",
			    opname.c_str(),N->outputs.size(),PyTuple_GET_SIZE(out.borrow()));
      }
      ssize_t i = 0;
      for(auto x:N->outputs) {
	PyObject* P = PyTuple_GET_ITEM(out.borrow(),i++);
	Py_INCREF(P);
	PyList_SET_ITEM(frame.borrow(),x.second->foffset,P);
      }
    }

    // Expected one output (and got back a non-tuple)
    else if (N->outputs.size() == 1) {
      auto outport = N->outputs.begin()->second->foffset;
      PyList_SET_ITEM(frame.borrow(),outport,out.incref());
    }

    else {
      return PyErr_Format(PyExc_ValueError,"%s expected %zd returns, but did not generate a tuple",
			  opname.c_str(),N->outputs.size());
    }
  }    

  auto inport_value = [mod](std::shared_ptr<inport> cxx,PyObject* F) {
    if (cxx->literal.size()) {
      return mod->literal_to_python(cxx);
    }

    // Connected to some source (value in frame)
    auto src = cxx->weakport.lock();
    if (!src) return DISCONNECTED;
    PyObject* v = PyList_GET_ITEM(F,src->foffset);
    Py_INCREF(v);
    return v;
  };

  // Create a tuple for the outputs (may be 0-ary (unlikely), 1-ary, or n-ary)
  PyOwned result(PyTuple_New(G->inputs.size()));
  ssize_t n = 0;
  for(auto port:G->inputs) {
    PyObject* x = inport_value(port.second,frame.borrow());
    if (!x) return nullptr;
    PyTuple_SET_ITEM(result.borrow(),n++,x);
  }
  return result.incref();
}

PyObject* module::interpret(PyObject* self,PyObject* args) {
  if (PyTuple_GET_SIZE(args) < 2) {
    return PyErr_Format(PyExc_TypeError,"interpret() takes 2 or more arguments");
  }

  PyObject* interpreter = PyTuple_GET_ITEM(args,0);
  PyObject* node = PyTuple_GET_ITEM(args,1);
  if (PyObject_TypeCheck(node,&graph::Type)) {
    return interpret_graph(self,interpreter,node,args);
  } else if (PyObject_TypeCheck(node,&node::Type)) {
    return interpret_node(self,interpreter,node,args);
  }
  return PyErr_Format(PyExc_ValueError,"2nd arg must be a node or graph");
}

PyObject* module::type_of_value(PyObject* self,PyObject* v) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  STATIC_STR(BOOLEAN,"boolean");
  STATIC_STR(INTEGER,"integer");
  STATIC_STR(DOUBLEREAL,"doublereal");
  STATIC_STR(STRING,"string");

  if (PyObject_TypeCheck(v,&outport::Type)) {
    return outport::get_type(v,nullptr);
  }

  else if (PyBool_Check(v)) {
    PyObject* T = PyDict_GetItem(cxx->dict.borrow(),BOOLEAN);
    if (T) { Py_INCREF(T); return T; }
  }
    
  else if (PyInt_Check(v)) {
    PyObject* T = PyDict_GetItem(cxx->dict.borrow(),INTEGER);
    if (T) { Py_INCREF(T); return T; }
  }

  else if (PyFloat_Check(v)) {
    PyObject* T = PyDict_GetItem(cxx->dict.borrow(),DOUBLEREAL);
    if (T) { Py_INCREF(T); return T; }
  }

  else if (PyString_Check(v)) {
    PyObject* T = PyDict_GetItem(cxx->dict.borrow(),STRING);
    if (T) { Py_INCREF(T); return T; }
  }
  
  return PyErr_Format(PyExc_RuntimeError,"Could not determine the type");
}

PyMethodDef module::methods[] = {
  {(char*)"interpret",module::interpret,METH_VARARGS,"interpret a node or function using an interpreter object"},
  {(char*)"addtype",(PyCFunction)module::addtype,METH_VARARGS|METH_KEYWORDS,"add a type"},
  {(char*)"addtypechain",(PyCFunction)module::addtypechain,METH_VARARGS|METH_KEYWORDS,"create a type chain (tuple, tags, fields)"},
  {(char*)"addfunction",module::addfunction,METH_VARARGS,"add a new function"},
  {(char*)"type_of_value",module::type_of_value,METH_O,"determine type of a value"},
  {nullptr}
};

PyObject* module::addtype(PyObject* self,PyObject* args,PyObject* kwargs) {
  STATIC_STR(NAME,"name");

  // Required argument/s
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  long code;
  PyObject* o1 = nullptr;
  PyObject* o2 = nullptr;
  if (!PyArg_ParseTuple(args,"l|OO",&code,&o1,&o2)) {
    return nullptr;
  }

  // Some are optionals
  char const* name = nullptr;
  if (kwargs) {
    PyObject* optName = PyDict_GetItem(kwargs,NAME);
    if (optName) {
      if (!PyString_Check(optName)) {
	return PyErr_Format(PyExc_TypeError,"name must be a string");
      }
      name = PyString_AS_STRING(optName);
    }
  }

  // The p1/p2 may be missing (nullptr), Py_None, or a type
  auto fixtype = [](PyObject* o,char const* required) {
    std::shared_ptr<type> answer;
    if (o) {
      if (PyObject_TypeCheck(o,&type::Type)) {
	answer = reinterpret_cast<type::python*>(o)->cxx;
      } else if (o != Py_None) {
	PyErr_SetString(PyExc_TypeError,"bad type");
      }
    }
    if (!answer && required) {
      PyErr_Format(PyExc_TypeError,"%s is required here",required);
    }
    return answer;
  };

  ssize_t n = 0;
  std::shared_ptr<type> p1,p2;
  long aux = 0;
  switch(code) {
  case IF_Wild:
    if (o1 || o2) {
      return PyErr_Format(PyExc_TypeError,"invalid extra args");
    }
    n = std::make_shared<type>(code)->connect(cxx,name);
    break;
  case IF_Basic:
    if (!o1) {
      return PyErr_Format(PyExc_TypeError,"missing aux arg");
    }
    if (o2) {
      return PyErr_Format(PyExc_TypeError,"invalid extra arg");
    }
    aux = PyInt_AsLong(o1);
    if (aux < 0 && PyErr_Occurred()) return nullptr;
    n = std::make_shared<type>(code,aux)->connect(cxx,name);
    break;
  case IF_Array:
  case IF_Multiple:
  case IF_Record:
  case IF_Stream:
  case IF_Union:
    if (!o1) {
      return PyErr_Format(PyExc_TypeError,"missing base type arg");
    }
    if (o2) {
      return PyErr_Format(PyExc_TypeError,"invalid extra arg");
    }
    p1 = fixtype(o1,"base type");
    if (!p1) return nullptr;
    n = std::make_shared<type>(code,aux,p1)->connect(cxx,name);
    break;

  case IF_Field:
  case IF_Tag:
  case IF_Tuple:
    p1 = fixtype(o1,"entry");
    if (!p1) return nullptr;
    p2 = fixtype(o2,nullptr);
    if (!p2 && PyErr_Occurred()) return nullptr;
    n = std::make_shared<type>(code,aux,p1,p2)->connect(cxx,name);
    break;

  case IF_Function:
    p1 = fixtype(o1,nullptr);
    if (!p1 && PyErr_Occurred()) return nullptr;
    p2 = fixtype(o2,"outputs");
    if (!p2) return nullptr;
    n = std::make_shared<type>(code,aux,p1,p2)->connect(cxx,name);
    break;

  default:
    return PyErr_Format(PyExc_NotImplementedError,"invalid type code %ld",code);
  }
    
  if (n < 0) return nullptr;

  // Fetch that item from our list (previous version or newly created)
  PyObject* T = PyList_GET_ITEM(cxx->types.borrow(),n);
  Py_INCREF(T);
  return T;
}

PyObject* module::addtypechain(PyObject* self,PyObject* args,PyObject* kwargs) {
  STATIC_STR(CODE,"code");
  STATIC_STR(NAME,"name");
  STATIC_STR(NAMES,"names");
  STATIC_STR(ADDTYPE,"addtype");
  static PyObject* default_code = nullptr;
  if (!default_code) {
    default_code = PyInt_FromLong(IF_Tuple);
    if (!default_code) return nullptr;
  }

  ssize_t n = PyTuple_GET_SIZE(args);
  // If we have no entries, just return None
  if (n == 0) Py_RETURN_NONE;

  // Make sure we have a type code and the optional names
  PyObject* code = (kwargs)?PyDict_GetItem(kwargs,CODE):nullptr;
  if (!code) code = default_code;
  if (!PyInt_Check(code)) {
    return PyErr_Format(PyExc_TypeError,"code must be an integer");
  }
  PyObject* names = (kwargs)?PyDict_GetItem(kwargs,NAMES):nullptr;
  PyOwned name;
  if (names) {
    PyOwned iter(PyObject_GetIter(names));
    if (!iter) return nullptr;
    if (PyDict_SetItem(kwargs,NAMES,iter.borrow()) < 0) {
      return nullptr;
    }
    name = PyIter_Next(iter.borrow());
    if (!name) return nullptr;
  }

  PyObject* T = PyTuple_GET_ITEM(args,0);
  PyOwned rest(PyTuple_GetSlice(args,1,n));
  if (!rest) return nullptr;

  PyOwned next(addtypechain(self,rest.borrow(),kwargs));
  if (!next) return nullptr;

  PyOwned result(PyObject_CallMethodObjArgs(self,ADDTYPE,code,T,next.borrow(),nullptr));
  if (!result) return nullptr;
  if (name) {
    if (PyObject_SetAttr(result.borrow(),NAME,name.borrow()) < 0) return nullptr;
  }
  return result.incref();
}

PyObject* module::addfunction(PyObject* self,PyObject* args) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;

  char const* name = nullptr;
  long opcode = IFXGraph;
  if (!PyArg_ParseTuple(args,"s|l",&name,&opcode)) return nullptr;
  auto p = std::make_shared<graph>(opcode,cxx,name);

  PyOwned f(p->package());

  if (PyList_Append(cxx->functions.borrow(),f.borrow()) < 0) return nullptr;
  return f.incref();
}

PyGetSetDef module::getset[] = {
  {(char*)"if1",module::get_if1,nullptr,(char*)"IF1 code for entire module",nullptr},
  {(char*)"types",module::get_types,nullptr,(char*)"types",nullptr},
  {(char*)"pragmas",module::get_pragmas,nullptr,(char*)"whole program pragma dictionary (char->string)",nullptr},
  {(char*)"functions",module::get_functions,nullptr,(char*)"top level functions",nullptr},
  {(char*)"opcodes",module::get_opcodes,module::set_opcodes,(char*)"name to opcode map (standard IF1 by default)",nullptr},
  {nullptr}
};

PyObject* module::get_if1(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;

  STATIC_STR(NEWLINE,"\n");
  STATIC_STR(C_DOLLAR_SPACESPACE,"C$  ");
  STATIC_STR(SPACE," ");
  
  // Do the functions first so that any new types get generated first
  PyObject* /*owned*/ functions = PyString_FromString("");
  if (!functions) return nullptr;
  ssize_t nfunctions = PyList_GET_SIZE(cxx->functions.borrow());
  for(ssize_t i=0; i<nfunctions; ++i) {
    PyObject* /*owned*/ if1 = PyObject_GetAttrString(PyList_GET_ITEM(cxx->functions.borrow(),i),"if1");
    if (!if1) return nullptr;
    PyString_ConcatAndDel(&functions,if1);
    if (!functions) return nullptr;
    if (i != nfunctions-1) {
      PyString_Concat(&functions,NEWLINE);
      if (!functions) return nullptr;
    }
  }
  PyOwned suffix(functions);  // functions now borrowed

    // Add if1 for all the types
  PyOwned result(PyString_FromString(""));
  if (!result) return nullptr;
  for(ssize_t i=0;i<PyList_GET_SIZE(cxx->types.borrow());++i) {
    PyObject* if1 /*owned*/ = PyObject_GetAttrString(PyList_GET_ITEM(cxx->types.borrow(),i),"if1");
    PyString_ConcatAndDel(result.addr(),if1);
    if (!result) return nullptr;
    PyString_Concat(result.addr(),NEWLINE);
    if (!result) return nullptr;
  }

  // And now the pragmas
  PyObject* key;
  PyObject* remark;
  Py_ssize_t pos = 0;
  while (PyDict_Next(cxx->pragmas.borrow(), &pos, &key, &remark)) {
    // Only care about 1 char keys with string values
    if (!PyString_Check(key) ||
	PyString_GET_SIZE(key) != 1 ||
	!PyString_Check(remark)) {
      continue;
    }
    PyString_Concat(result.addr(),C_DOLLAR_SPACESPACE);
    if (!result) return nullptr;
    PyString_Concat(result.addr(),key);
    if (!result) return nullptr;
    PyString_Concat(result.addr(),SPACE);
    if (!result) return nullptr;
    PyString_Concat(result.addr(),remark);
    if (!result) return nullptr;
    PyString_Concat(result.addr(),NEWLINE);
    if (!result) return nullptr;
  }
  PyString_Concat(result.addr(),functions);

  return result.incref();
}

PyObject* module::get_types(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return cxx->types.incref();
}

PyObject* module::get_functions(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return cxx->functions.incref();
}

PyObject* module::get_pragmas(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return cxx->pragmas.incref();
}

PyObject* module::get_opcodes(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return cxx->opcodes.incref();
}
int module::set_opcodes(PyObject* self,PyObject* attr,void*) {
  TODO("set opcodes");
  return -1;
}

static char parse_if1_line(PyObject* line,std::vector<long>& longs,std::vector<std::string>& strings) {
  longs.clear();
  strings.clear();
  PyObject* linesplit /*owned*/ = PyObject_CallMethod(line,(char*)"split",(char*)"");
  if (!linesplit) return 0;
  for(ssize_t i=0; i < PyList_GET_SIZE(linesplit); ++i) {
    PyObject* token /*borrowed*/ = PyList_GET_ITEM(linesplit,i);
    PyObject* asInt /*owned*/ = PyNumber_Int(token);
    if (asInt) {
      long ival = PyInt_AsLong(asInt);
      Py_DECREF(asInt);
      longs.push_back(ival);
    } else {
      PyErr_Clear();
      strings.push_back(std::string(PyString_AS_STRING(token),PyString_GET_SIZE(token)));
    }
  }
  if (strings.size() == 0) return 0;
  return strings[0][0];
}

// Remove blank lines
bool module::strip_blanks(PyObject* lines) {
  STATIC_STR(STRIP,"strip");
  for(ssize_t i=0;i<PyList_GET_SIZE(lines);) {
    PyObject* s = PyList_GET_ITEM(lines,i);
    PyOwned stripped(PyObject_CallMethodObjArgs(s,STRIP,nullptr));
    if (!stripped) return false;
    if (PyString_GET_SIZE(stripped.borrow()) > 0) {
      Py_DECREF(s);
      PyList_SET_ITEM(lines,i,stripped.incref());
      ++i;
    } else {
      if (PySequence_DelItem(lines,i) < 0) return false;
    }
  }
  return true;
}

bool module::read_types(PyObject* lines,
			std::map<long,std::shared_ptr<type>>& typemap) {

  // a quick lambda to make sure a type exists
  auto fix = [&](long x) {
    std::shared_ptr<type> result;
    if (x <= 0) return result;
    auto p = typemap.find(x);
    if (p != typemap.end()) return p->second;
    result = typemap[x] = std::make_shared<type>(IF_Wild);
    return result;
  };
  
  // Here, we know we have no empty strings so we can
  // pull out the type lines
  for(ssize_t i=0;i<PyList_GET_SIZE(lines);++i) {
    std::vector<long> longs;
    std::vector<std::string> strings;
    PyObject* T = PyList_GET_ITEM(lines,i);
    char flavor = parse_if1_line(T,longs,strings);
    if (!flavor) return false;
    if (flavor != 'T') continue;
    if (longs.size() < 2) {
      PyErr_Format(PyExc_TypeError,"bad type line: %s",
		   PyString_AS_STRING(T));
      return false;
    }

    auto label = longs[0];
    auto code = longs[1];
    auto type = fix(label);
    switch(code) {
    case IF_Basic:
      if (longs.size() != 3) {
	PyErr_Format(PyExc_ValueError,
		     "Invalid basic type label %ld",
		     label);
	return false;
      }
      type->code = code;
      type->aux = longs[2];
      break;
    case IF_Array:
    case IF_Multiple:
    case IF_Record:
    case IF_Stream:
    case IF_Union:
      if (longs.size() != 3) {
	PyErr_Format(PyExc_ValueError,
		     "Invalid container type label %ld",
		     label);
	return false;
      }
      type->code = code;
      type->parameter1 = fix(longs[2]);
      break;
    case IF_Field:
    case IF_Tag:
    case IF_Tuple:
      if (longs.size() != 4 || longs[2] <= 0) {
	PyErr_Format(PyExc_ValueError,
		     "Invalid chain type label %ld",
		     label);
	return false;
      }
      type->code = code;
      type->parameter1 = fix(longs[2]);
      type->parameter2 = fix(longs[3]);
      break;

    case IF_Function:
      if (longs.size() != 4 || longs[3] <= 0) {
	PyErr_Format(PyExc_ValueError,
		     "Invalid function type label %ld",
		     label);
	return false;
      }
      type->code = code;
      type->parameter1 = fix(longs[2]);
      type->parameter2 = fix(longs[3]);
      break;
    case IF_Wild:
      if (longs.size() != 2) {
	PyErr_Format(PyExc_ValueError,
		     "Invalid wild type label %ld",
		     label);
	return false;
      }
      type->code = code;
      break;
    default:
      PyErr_Format(PyExc_TypeError,
		   "unknown typecode %ld %ld",label,code);
      return false;
    }

    for(auto& x:strings) {
      char const* p = x.c_str();
      if (x.size() < 4) continue;
      if (x[0] != '%') continue;
      if (x[3] != '=') continue;
      PyOwned key(PyString_FromStringAndSize(p+1,2));
      if (!key) return false;
      PyOwned value(PyString_FromString(p+4));
      if (!value) return false;
      PyOwned ivalue(PyNumber_Int(value.borrow()));
      if (ivalue) {
	PyDict_SetItem(type->pragmas.borrow(),key.borrow(),ivalue.borrow());
      } else {
	PyErr_Clear();
	PyDict_SetItem(type->pragmas.borrow(),key.borrow(),value.borrow());
      }
    }
  }

  // Now, we connect the types to the module and
  // append them to the type list
  auto cxx = shared();
  for(auto x:typemap) {
    std::shared_ptr<type>& T = x.second;
    T->weakmodule = cxx;
    PyOwned element(T->package());
    if (PyList_Append(types.borrow(),element.borrow()) < 0) {
      return false;
    }
  }
  return true;
}
bool module::read_pragmas(PyObject* lines) {
  for(ssize_t i=0;i<PyList_GET_SIZE(lines);++i) {
  }
  return true;
}
bool module::read_functions(PyObject* lines,std::map<long,std::shared_ptr<type>>& typemap) {
  for(ssize_t i=0;i<PyList_GET_SIZE(lines);++i) {
  }
  return true;
}

int module::init(PyObject* self,PyObject* args,PyObject* kwargs) {
  auto status = IF1<module>::init(self,args,kwargs);
  if (status < 0) return status;
  auto cxx = reinterpret_cast<python*>(self)->cxx;

  PyObject* source = nullptr;
  static char* keywords[] = {(char*)"source",nullptr};
  if (!PyArg_ParseTupleAndKeywords(args,kwargs,"|O!",keywords,&PyString_Type,&source)) throw PyErr_Occurred();

  if (source) {
    PyOwned split(PyObject_CallMethod(source,(char*)"split",(char*)"s","\n"));
    std::map<long,std::shared_ptr<type>> typemap;
    if (!split) return -1;
    if (!cxx->strip_blanks(split.borrow())) return -1;
    if (!cxx->read_types(split.borrow(),typemap)) return -1;
    if (!cxx->read_pragmas(split.borrow())) return -1;
    if (!cxx->read_functions(split.borrow(),typemap)) return -1;
  } else {

    // We create the built in types here (not in c'tor) because we need to set a weak
    // pointer to the module (can't do with c'tor incomplete)
    std::make_shared<type>(IF_Basic,IF_Boolean)->connect(cxx,"boolean");
    auto character = std::make_shared<type>(IF_Basic,IF_Character);
    character->connect(cxx,"character");
    std::make_shared<type>(IF_Basic,IF_DoubleReal)->connect(cxx,"doublereal");
    std::make_shared<type>(IF_Basic,IF_Integer)->connect(cxx,"integer");
    std::make_shared<type>(IF_Basic,IF_Null)->connect(cxx,"null");
    std::make_shared<type>(IF_Basic,IF_Real)->connect(cxx,"real");
    std::make_shared<type>(IF_Basic,IF_WildBasic)->connect(cxx,"wildbasic");
    std::make_shared<type>(IF_Wild,0)->connect(cxx,"wild");
    std::make_shared<type>(IF_Array,0,character)->connect(cxx,"string");
  }
  
  // Copy the type names (if any) into the dict (for getattro)
  for(ssize_t i=0;i<PyList_GET_SIZE(cxx->types.borrow());++i) {
    PyObject* T = PyList_GET_ITEM(cxx->types.borrow(),i);
    auto p = type::borrow(T);
    PyObject* na = PyDict_GetItemString(p->pragmas.borrow(),"na");
    if (na) {
      PyDict_SetItem(cxx->dict.borrow(),na,T);
    }
  }
  return 0;
}

PyObject* module::getattro(PyObject* o,PyObject* attr) {
  auto cxx = reinterpret_cast<python*>(o)->cxx;
  // Type lookup?
  PyObject* p /*borrowed*/ = PyDict_GetItem(cxx->dict.borrow(),attr);
  if (p) { Py_INCREF(p); return p; }

  // Opcode lookup?
  p = PyDict_GetItem(cxx->opcodes.borrow(),attr);
  if (p) { Py_INCREF(p); return p; }

  // normal lookup
  return PyObject_GenericGetAttr(o,attr);
}

std::string module::lookup(long code) {
  PyObject* key;
  PyObject* value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(opcodes.borrow(), &pos, &key, &value)) {
    if (!PyString_Check(key)) continue;
    if (PyString_GET_SIZE(key) < 3) continue;
    char const* s = PyString_AS_STRING(key);
    if ( s[0] != 'I' || s[1] != 'F' || s[2] == '_' ) continue;
    if (PyInt_Check(value) && PyInt_AsLong(value) == code) return s;
  }
  return "";
}

long module::lookup(std::string const& name) {
  return 12345;
}

PyObject* module::item(PyObject* self,ssize_t opcode) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  auto name = cxx->lookup(opcode);
  if (name.size() == 0) {
    return PyErr_Format(PyExc_IndexError,"No such opcode %xd",(unsigned int)opcode);
  }
  return PyString_FromString(name.c_str());
}


module:: module(python* self, PyObject* args,PyObject* kwargs)
  : types(nullptr), pragmas(nullptr), functions(nullptr), dict(nullptr)
{
  types = PyList_New(0);
  if (!types) throw PyErr_Occurred();

  pragmas = PyDict_New();
  if (!pragmas) throw PyErr_Occurred();

  functions = PyList_New(0);
  if (!functions) throw PyErr_Occurred();

  dict = PyDict_New();
  if (!dict) throw PyErr_Occurred();

  Py_INCREF(DEFAULT_OPCODES);
  opcodes = DEFAULT_OPCODES;
}

PyNumberMethods module::as_number;
PySequenceMethods module::as_sequence;
