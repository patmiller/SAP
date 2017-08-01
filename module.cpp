#include "module.h"
#include "type.h"
#include "graph.h"

PyTypeObject module::Type;

char const* module::doc = "TBD Module";

void module::setup() {
  as_sequence.sq_item = item;
}

PyMethodDef module::methods[] = {
  {(char*)"addtype",(PyCFunction)module::addtype,METH_VARARGS|METH_KEYWORDS,"add a type"},
    {(char*)"addtypechain",(PyCFunction)module::addtypechain,METH_VARARGS|METH_KEYWORDS,"create a type chain (tuple, tags, fields)"},
  {(char*)"addfunction",module::addfunction,METH_VARARGS,"add a new function"},
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
  for(ssize_t i=0; i<PyList_GET_SIZE(cxx->functions.borrow()); ++i) {
    PyObject* /*owned*/ if1 = PyObject_GetAttrString(PyList_GET_ITEM(cxx->functions.borrow(),i),"if1");
    if (!if1) return nullptr;
    PyString_ConcatAndDel(&functions,if1);
    if (!functions) return nullptr;
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
    if (PyString_Check(key) && PyInt_Check(value) && PyInt_AsLong(value) == code) {
      return PyString_AS_STRING(key);
    }
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
    return PyErr_Format(PyExc_IndexError,"No such opcode %xd",opcode);
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
