#include "Python.h"
#include "structmember.h"
#include "IFX.h"

typedef struct {
  PyObject_HEAD
  PyObject* module;
  unsigned long code;
  unsigned long aux;
  PyObject* weak1;
  PyObject* weak2;
  PyObject* name;

  // Weak reference slot
  PyObject* weak;
} pIF1_TypeObject;
const char* pIF1_type_doc =
  "TODO: Add docsn for type\n"
  ;

static PyMemberDef type_members[] = {
  {(char*)"code",T_LONG,offsetof(pIF1_TypeObject,code),READONLY,(char*)"type code: IFArray, IFBasic..."},
  {(char*)"name",T_OBJECT,offsetof(pIF1_TypeObject,name),0,(char*)"type name or None"},
  {NULL}  /* Sentinel */
};

typedef struct {
  PyObject_HEAD
  PyObject* dict;
  PyObject* types;
  PyObject* pragmas;

  // Weak reference slot
  PyObject* weak;
} pIF1_ModuleObject;
const char* pIF1_module_doc =
  "TODO: Add docs for module\n"
  ;
static PyMemberDef module_members[] = {
  {(char*)"types",T_OBJECT_EX,offsetof(pIF1_ModuleObject,types),0,(char*)"type table"},
  {(char*)"pragmas",T_OBJECT_EX,offsetof(pIF1_ModuleObject,pragmas),0,(char*)"pragma set"},
  {(char*)"__dict__",T_OBJECT_EX,offsetof(pIF1_ModuleObject,dict),0,(char*)"instance dictionary"},
  {NULL}  /* Sentinel */
};


static PyTypeObject pIF1_TypeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pIF1.Type",             /* tp_name */
    sizeof(pIF1_TypeObject), /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    pIF1_type_doc,             /* tp_doc */
    0,                       // tp_traverse
    0,                       // tp_clear
    0,                       // tp_richcompare
    offsetof(pIF1_TypeObject,weak) // tp_weaklistoffset
};

PyNumberMethods type_number;

void type_dealloc(PyObject* pySelf) {
  pIF1_TypeObject* self = reinterpret_cast<pIF1_TypeObject*>(pySelf);
  if (self->weak) PyObject_ClearWeakRefs(pySelf);

  Py_XDECREF(self->module);
  Py_XDECREF(self->weak1);
  Py_XDECREF(self->weak2);
  Py_XDECREF(self->name);
  Py_TYPE(pySelf)->tp_free(pySelf);
}

long type_label(PyObject* pySelf) {
  pIF1_TypeObject* self = reinterpret_cast<pIF1_TypeObject*>(pySelf);
  PyObject* w = PyWeakref_GET_OBJECT(self->module);
  if (w == Py_None) { PyErr_SetString(PyExc_RuntimeError,"disconnected type"); return -1; }
  pIF1_ModuleObject* m = reinterpret_cast<pIF1_ModuleObject*>(w);
  for(long label=0; label < PyList_GET_SIZE(m->types); ++label) {
    if (PyList_GET_ITEM(m->types,label) == pySelf) return label+1;
  }
  PyErr_SetString(PyExc_RuntimeError,"disconnected type");
  return -1;
}


PyObject* type_int(PyObject* self) {
  long label = type_label(self);
  if (label <= 0) {
    PyErr_SetString(PyExc_TypeError,"disconnected type");
    return NULL;
  }
  return PyInt_FromLong(label);
}

PyObject* type_str(PyObject* pySelf) {
  pIF1_TypeObject* self = reinterpret_cast<pIF1_TypeObject*>(pySelf);

  static const char* flavor[] = {"array","basic","field","function","multiple","record","stream","tag","tuple","union"};
  static PyObject* arrow = NULL;
  if (!arrow) {
    arrow = PyString_InternFromString("->");
    if (!arrow) return NULL;
  }

  static PyObject* colon = NULL;
  if (!colon) {
    colon = PyString_InternFromString(":");
    if (!colon) return NULL;
  }

  static PyObject* comma = NULL;
  if (!comma) {
    comma = PyString_InternFromString(",");
    if (!comma) return NULL;
  }

  PyObject* lparen = NULL;
  if (!lparen) {
    lparen = PyString_InternFromString("(");
    if (!lparen) return NULL;
  }

  PyObject* rparen = NULL;
  if (!rparen) {
    rparen = PyString_InternFromString(")");
    if (!rparen) return NULL;
  }

  // If we named the type, just use the name (unless it is a tuple, field, or tag)
  if (self->name && self->code != IFTuple && self->code != IFField && self->code != IFTag) {
    Py_INCREF(self->name); 
    return self->name;
  }

  switch(self->code) {
  case IFRecord:
  case IFUnion: {
    PyObject* basetype = PyWeakref_GET_OBJECT(self->weak1);
    if (basetype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected structure");
    PyObject* base /*owned*/ = PyObject_Str(basetype);
    if (!base) return NULL;
    PyObject* result = PyString_FromFormat("%s%s",flavor[self->code],PyString_AS_STRING(base));
    Py_DECREF(base);
    return result;
  }

  case IFArray:
  case IFMultiple:
  case IFStream: {
    PyObject* basetype = PyWeakref_GET_OBJECT(self->weak1);
    if (basetype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected base type");
    PyObject* base /*owned*/ = PyObject_Str(basetype);
    if (!base) return NULL;
    PyObject* result = PyString_FromFormat("%s[%s]",flavor[self->code],PyString_AS_STRING(base));
    Py_DECREF(base);
    return result;
  }
  case IFBasic:
    return PyString_FromFormat("Basic(%ld)",self->aux);
  case IFField:
  case IFTag:
  case IFTuple: {
    PyObject* result = PyString_FromString("");
    if (!result) return NULL;
    PyString_Concat(&result,lparen);
    if (!result) return NULL;

    for(;self;) {
      if (self->name) {
	PyObject* fname = PyObject_Str(self->name);
	if (!fname) { Py_DECREF(result); return NULL; }
	PyString_ConcatAndDel(&result,fname);
	if (!result) return NULL;
	PyString_Concat(&result,colon);
	if (!result) return NULL;
      }

      PyObject* elementtype = PyWeakref_GET_OBJECT(self->weak1);
      if (elementtype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected tuple element type");

      PyObject* element /*owned*/ = PyObject_Str(elementtype);
      if (!element) return NULL;

      PyString_ConcatAndDel(&result,element);
      if (!result) return NULL;

      PyObject* next = self->weak2?PyWeakref_GET_OBJECT(self->weak2):NULL;
      if (next == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected tuple element link");
      self = reinterpret_cast<pIF1_TypeObject*>(next);
      if (self) PyString_Concat(&result,comma);
      if (!result) return NULL;
    }
    PyString_Concat(&result,rparen);
    return result;
  }
  case IFFunction: {
    PyObject* intype = (self->weak1)?PyWeakref_GET_OBJECT(self->weak1):NULL;
    if (intype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected function input");

    PyObject* outtype = PyWeakref_GET_OBJECT(self->weak2);
    if (outtype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected function output");

    PyObject* ins /*owned*/ = (intype)?PyObject_Str(intype):PyString_FromString("");
    if (!ins) return NULL;
    PyObject* outs /*owned*/ = PyObject_Str(outtype);
    if (!outs) { Py_DECREF(ins); return NULL; }

    PyString_Concat(&ins,arrow);
    PyString_ConcatAndDel(&ins,outs);
    return ins;
  }
  }
  return PyErr_Format(PyExc_NotImplementedError,"Code %ld",self->code);
}

PyObject* type_get_if1(PyObject* pySelf,void*) {
  pIF1_TypeObject* self = reinterpret_cast<pIF1_TypeObject*>(pySelf);
  long label = type_label(pySelf);
  if (label < 0) return NULL;

  switch(self->code) {
    // Wild is a special case
  case IFWild:
    if (self->name && PyString_Check(self->name))
      return PyString_FromFormat("T %ld %ld %%na=%s",label,self->code,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld",label,self->code);

    // Basic uses the aux, not the weak fields
  case IFBasic:
    if (self->name && PyString_Check(self->name)) 
      return PyString_FromFormat("T %ld %ld %ld %%na=%s",label,self->code,self->aux,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld %ld",label,self->code,self->aux);

    // One parameter case
  case IFArray:
  case IFMultiple:
  case IFRecord:
  case IFStream:
  case IFUnion: {
    PyObject* one = PyWeakref_GET_OBJECT(self->weak1);
    if (one == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected type");
    long one_label = type_label(one);
    if ( one_label < 0 ) return NULL;
    if (self->name && PyString_Check(self->name)) 
      return PyString_FromFormat("T %ld %ld %ld %%na=%s",label,self->code,one_label,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld %ld",label,self->code,one_label);
  }

    // Two parameter case for the rest
  default: {
    long one_label = 0;
    if (self->weak1) {
      PyObject* one = PyWeakref_GET_OBJECT(self->weak1);
      if (one == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected type");
      one_label = type_label(one);
      if ( one_label < 0 ) return NULL;
    }

    long two_label = 0;
    if (self->weak2) {
      PyObject* two = PyWeakref_GET_OBJECT(self->weak2);
      if (two == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected type");
      two_label = type_label(two);
      if ( two_label < 0 ) return NULL;
    }

    if (self->name && PyString_Check(self->name)) 
      return PyString_FromFormat("T %ld %ld %ld %ld %%na=%s",label,self->code,one_label,two_label,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld %ld %ld",label,self->code,one_label,two_label);
  }
  }
  return PyErr_Format(PyExc_NotImplementedError,"finish get if1 for %ld\n",self->code);

#if 0
  pIF1_TypeObject* self = reinterpret_cast<pIF1_TypeObject*>(pySelf);

  switch(self->code) {
    // No parameter case
  case IFWild:
    if (self->name && PyString_Check(self->name)) 
      return PyString_FromFormat("T %ld %ld %%na=%s",type_label(pySelf),self->code,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld",type_label(pySelf),self->code);

    // One parameter case (int)
  case IFArray:
  case IFBasic:
  case IFStream:
    if (self->name && PyString_Check(self->name)) 
      return PyString_FromFormat("T %ld %ld %ld %%na=%s",type_label(pySelf),self->code,self->parameter1,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld %ld",type_label(pySelf),self->code,self->parameter1);

  default:
    // Two parameter case
    if (self->name && PyString_Check(self->name)) 
      return PyString_FromFormat("T %ld %ld %ld %ld %%na=%s",type_label(pySelf),self->code,self->parameter1,self->parameter2,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld %ld %ld",type_label(pySelf),self->code,self->parameter1,self->parameter2);

  }
  return PyErr_Format(PyExc_NotImplementedError,"Unknown type code %ld",self->code);
#endif
}

PyObject* type_get_aux(PyObject* pySelf,void*) {
  pIF1_TypeObject* self = reinterpret_cast<pIF1_TypeObject*>(pySelf);
  if (self->code == IFBasic) return PyInt_FromLong(self->aux);
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* type_get_parameter1(PyObject* pySelf,void*) {
  pIF1_TypeObject* self = reinterpret_cast<pIF1_TypeObject*>(pySelf);
  if (!self->weak1) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  PyObject* strong = PyWeakref_GET_OBJECT(self->weak1);
  if (strong == Py_None) {
    return PyErr_Format(PyExc_RuntimeError,"disconnected parameter1");
  }
  Py_INCREF(strong);
  return strong;
}

PyObject* type_get_parameter2(PyObject* pySelf,void*) {
  pIF1_TypeObject* self = reinterpret_cast<pIF1_TypeObject*>(pySelf);
  if (!self->weak2) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  PyObject* strong = PyWeakref_GET_OBJECT(self->weak2);
  if (strong == Py_None) {
    return PyErr_Format(PyExc_RuntimeError,"disconnected parameter2");
  }
  Py_INCREF(strong);
  return strong;
}

PyObject* type_get_label(PyObject* self,void*) {
  return type_int(self);
}
PyGetSetDef type_getset[] = {
  {(char*)"aux",type_get_aux,NULL,(char*)"aux code (basic only)",NULL},
  {(char*)"parameter1",type_get_parameter1,NULL,(char*)"parameter1 type (if any)",NULL},
  {(char*)"parameter2",type_get_parameter2,NULL,(char*)"parameter2 type (if any)",NULL},
  {(char*)"if1",type_get_if1,NULL,(char*)"if1 code",NULL},
  {(char*)"label",type_get_label,NULL,(char*)"type label",NULL},
  {NULL}
};

static PyTypeObject pIF1_ModuleType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pIF1.Module",             /* tp_name */
    sizeof(pIF1_ModuleObject), /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    pIF1_module_doc,           /* tp_doc */
    0,                       // tp_traverse
    0,                       // tp_clear
    0,                       // tp_richcompare
    offsetof(pIF1_ModuleObject,weak) // tp_weaklistoffset
};

static PyObject* rawtype(PyObject* module,
                         unsigned long code,
                         unsigned long aux,
                         PyObject* strong1,
                         PyObject* strong2,
                         PyObject* name,
                         const char* cname) {

  PyObject* typelist /*borrowed*/ = reinterpret_cast<pIF1_ModuleObject*>(module)->types;

  // We may already have this type (possibly with a different name). In IF1 those are equivalent,
  // so return that
  for(ssize_t i=0;i<PyList_GET_SIZE(typelist);++i) {
    PyObject* p /*borrowed*/ = PyList_GET_ITEM(typelist,i);
    pIF1_TypeObject* t = reinterpret_cast<pIF1_TypeObject*>(p);
    if (t->code != code) continue;
    if (t->aux != aux) continue;
    PyObject* strong = (t->weak1)?PyWeakref_GET_OBJECT(t->weak1):NULL;
    if (strong1 != strong) continue;
    strong = (t->weak2)?PyWeakref_GET_OBJECT(t->weak2):NULL;
    if (strong2 != strong) continue;
    Py_INCREF(p);
    return p;
  }

  PyObject* result /*owned*/ = PyType_GenericNew(&pIF1_TypeType,NULL,NULL);
  if (!result) return NULL;
  pIF1_TypeObject* T = reinterpret_cast<pIF1_TypeObject*>(result);
  T->module = NULL;
  T->weak1 = NULL;
  T->weak2 = NULL;
  T->name = NULL;

  T->module = PyWeakref_NewRef(module,NULL);
  if (!T->module) { Py_DECREF(T); return NULL;}
  T->code = code;
  T->aux = aux;
  T->weak1 = strong1?PyWeakref_NewRef(strong1,NULL):NULL;
  if (strong1 && !T->weak1) { Py_DECREF(T); return NULL;}
  T->weak2 = strong2?PyWeakref_NewRef(strong2,NULL):NULL;
  if (strong2 && !T->weak2) { Py_DECREF(T); return NULL;}
  if (name) {
    Py_INCREF(T->name = name);
  } else if (cname) {
    T->name = PyString_FromString(cname);
    if (!T->name) { Py_DECREF(T); return NULL;}
  } else {
    T->name = NULL;
  }

  if (PyList_Append(typelist,result) != 0) { Py_DECREF(T); return NULL;}

  return result;
}

static PyObject* rawtype(pIF1_ModuleObject* module,
                         unsigned long code,
                         unsigned long aux,
                         PyObject* sub1,
                         PyObject* sub2,
                         PyObject* name) {
  return rawtype(reinterpret_cast<PyObject*>(module),
		 code,
		 aux,
		 sub1,
		 sub2,
		 name,
		 NULL);
}

#if 0
static ssize_t addtype(PyObject* weak, PyObject* types, unsigned long code, unsigned long parameter1, unsigned long parameter2, PyObject* name, const char* cname) {
  // If we already have an item, return it's offset
  for(ssize_t i=0;i<PyList_GET_SIZE(types);++i) {
    PyObject* p /*borrowed*/ = PyList_GET_ITEM(types,i);
    pIF1_TypeObject* t = reinterpret_cast<pIF1_TypeObject*>(p);
    if (t->code == code && parameter1 == t->parameter1 && parameter2 == t->parameter2 ) return i;
  }

  PyObject* a = PyType_GenericNew(&pIF1_TypeType,NULL,NULL);
  Py_INCREF(((pIF1_TypeObject*)a)->module = weak);
  ((pIF1_TypeObject*)a)->code = code;
  ((pIF1_TypeObject*)a)->parameter1 = parameter1;
  ((pIF1_TypeObject*)a)->parameter2 = parameter2;
  if (name) {
    Py_INCREF(((pIF1_TypeObject*)a)->name = name);
  } else if (cname) {
    ((pIF1_TypeObject*)a)->name = PyString_FromString(cname);
    if (!((pIF1_TypeObject*)a)->name) return -1;
  } else {
    ((pIF1_TypeObject*)a)->name = NULL;
  }
  if (PyList_Append(types,a) != 0) return -1;
  Py_DECREF(a);
  return PyList_GET_SIZE(types)-1;
}

static PyObject* addtype(pIF1_ModuleObject* self, unsigned long code, unsigned long parameter1, unsigned long parameter2, PyObject* name) {
  PyObject* weak /*owned*/= PyWeakref_NewRef((PyObject*)self,NULL);
  if (!weak) return NULL;
  ssize_t idx = addtype(weak,self->types,code,parameter1,parameter2,name,NULL);
  if (idx < 0) { Py_DECREF(weak); return NULL; }
  PyObject* t = PyList_GetItem(self->types,idx);
  Py_XINCREF(t);
  Py_DECREF(weak);
  return t;
}
#endif

int module_init(PyObject* pySelf, PyObject* args, PyObject* kwargs) {
  pIF1_ModuleObject* self = reinterpret_cast<pIF1_ModuleObject*>(pySelf);
  self->types = NULL;
  self->pragmas = NULL;

  self->dict = PyDict_New();
  if (!self->dict) return -1;

  self->types = PyList_New(0);
  if (!self->types) return -1;

  self->pragmas = PyDict_New();
  if (!self->pragmas) return -1;

  PyObject* boolean /*owned*/ = rawtype(pySelf,IFBasic,IFBoolean,NULL,NULL,NULL,"boolean");
  if (!boolean) return -1;
  Py_DECREF(boolean);

  PyObject* character /*owned*/ = rawtype(pySelf,IFBasic,IFCharacter,NULL,NULL,NULL,"character");
  if (!character) return -1;
  Py_DECREF(character);

  PyObject* doublereal /*owned*/ = rawtype(pySelf,IFBasic,IFDoubleReal,NULL,NULL,NULL,"doublereal");
  if (!doublereal) return -1;
  Py_DECREF(doublereal);

  PyObject* integer /*owned*/ = rawtype(pySelf,IFBasic,IFInteger,NULL,NULL,NULL,"integer");
  if (!integer) return -1;
  Py_DECREF(integer);

  PyObject* null /*owned*/ = rawtype(pySelf,IFBasic,IFNull,NULL,NULL,NULL,"null");
  if (!null) return -1;
  Py_DECREF(null);

  PyObject* real /*owned*/ = rawtype(pySelf,IFBasic,IFReal,NULL,NULL,NULL,"real");
  if (!real) return -1;
  Py_DECREF(real);

  PyObject* wildbasic /*owned*/ = rawtype(pySelf,IFBasic,IFWildBasic,NULL,NULL,NULL,"wildbasic");
  if (!wildbasic) return -1;
  Py_DECREF(wildbasic);

  PyObject* wild /*owned*/ = rawtype(pySelf,IFWild,0,NULL,NULL,NULL,"wild");
  if (!wild) return -1;
  Py_DECREF(wild);

  for(int i=0;i<PyList_GET_SIZE(self->types);++i) {
    PyObject* p /*borrowed*/ = PyList_GET_ITEM(self->types,i);
    pIF1_TypeObject* t /*borrowed*/ = reinterpret_cast<pIF1_TypeObject*>(p);
    if (t->name) PyDict_SetItem(self->dict,t->name,p);
  }
  return 0;
}

void module_dealloc(PyObject* pySelf) {
  pIF1_ModuleObject* self = reinterpret_cast<pIF1_ModuleObject*>(pySelf);
  if (self->weak) PyObject_ClearWeakRefs(pySelf);

  Py_XDECREF(self->types);
  Py_TYPE(pySelf)->tp_free(pySelf);
}

PyObject* module_str(PyObject* pySelf) {
  return PyString_FromString("<module>");
}

PyObject* module_if1(PyObject* pySelf,void*) {
  pIF1_ModuleObject* self = reinterpret_cast<pIF1_ModuleObject*>(pySelf);
  PyObject* result = PyList_New(0);
  if (!result) return NULL;

  // Add if1 for all the types
  PyObject* titer /*owned*/ = PyObject_GetIter(self->types);
  if (!titer) { Py_DECREF(result); return NULL; }
  PyObject* T;
  PyErr_Clear();
  while((T = PyIter_Next(titer))) {
    PyObject* if1 /*owned*/ = PyObject_GetAttrString(T,"if1");
    Py_DECREF(T);
    int a = PyList_Append(result,if1);
    Py_DECREF(if1);
    if (a) {Py_DECREF(titer); Py_DECREF(result); return NULL; }
  }
  Py_DECREF(titer);

  // Add all the pragmas
  PyObject* key;
  PyObject* remark;
  Py_ssize_t pos = 0;
  while (PyDict_Next(self->pragmas, &pos, &key, &remark)) {
    if (!PyString_Check(key)) {
      Py_DECREF(result);
      return PyErr_Format(PyExc_TypeError,"pragma key is not a string");
    }
    if (!PyString_Check(remark)) {
      Py_DECREF(result);
      return PyErr_Format(PyExc_TypeError,"pragma value is not a string");
    }
    PyObject* if1 /*owned*/ = PyString_FromFormat("C$  %c %s",PyString_AS_STRING(key)[0],PyString_AS_STRING(remark));
    if (!if1) { Py_DECREF(result); return NULL; }
    int a = PyList_Append(result,if1);
    Py_DECREF(if1);
    if (a) { Py_DECREF(result); return NULL; }
  }

  if (PyErr_Occurred()) return NULL;
  return result;
}

PyObject* module_addtype_array(pIF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist) {
  long code;
  PyObject* base = NULL;
  if (!PyArg_ParseTuple(args,"lO!",&code,&pIF1_TypeType,&base)) return NULL;
  return rawtype(self,code,0,base,NULL,name);
}

PyObject* module_addtype_basic(pIF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist) {
  long code;
  long basic;
  if (!PyArg_ParseTuple(args,"ll",&code,&basic)) return NULL;
  if (basic < 0 || basic > IFWildBasic) return PyErr_Format(PyExc_ValueError,"invalid basic code %ld",basic);
  return rawtype(self,code,basic,NULL,NULL,name);
}

template<int IFXXX>
PyObject* module_addtype_chain(pIF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist, ssize_t first) {
  PyObject* last /*owned*/ = NULL;
  for(ssize_t i=PyTuple_GET_SIZE(args)-1; i>=first; --i) {
    PyObject* Tname = namelist?PyList_GetItem(namelist,i-first):NULL;
    PyErr_Clear();
    PyObject* T /*borrowed*/ = PyTuple_GET_ITEM(args,i);
    if (!PyObject_IsInstance(T,reinterpret_cast<PyObject*>(&pIF1_TypeType))) {
      Py_XDECREF(last);
      return PyErr_Format(PyExc_TypeError,"Argument %ld is not a type",(long)i+1);
    }
    PyObject* next = rawtype(self,IFXXX,0,T,last,Tname);
    Py_XDECREF(last);
    last = next;
  }

  // We return the last type or None for the empty argument set
  if (!last) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  return last;
}

template<int STRUCTURE,int CHAIN>
PyObject* module_addtype_structure(pIF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist) {
  PyObject* chain = module_addtype_chain<CHAIN>(self,args,NULL,namelist,1);
  if (!chain) return NULL;

  return rawtype(self,STRUCTURE,0,chain,NULL,name);
}


PyObject* module_addtype_function(pIF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist) {
  // I want this to be of the form: (code,tupletype,tupletype), but I might get
  // ... (code,(T,T,T),(T,T)) or (code,None,(T,T)) or even (code,(T,T,T),T)
  // at least it all looks like (code, ins, outs)
  long code;
  PyObject* ins;
  PyObject* outs;
  if (!PyArg_ParseTuple(args,"lOO",&code,&ins,&outs)) return NULL;

  // Inputs may be a tupletype, a tuple of types, a normal type, or None
  if (PyObject_IsInstance(ins,(PyObject*)&pIF1_TypeType)) {
    pIF1_TypeObject* tins = reinterpret_cast<pIF1_TypeObject*>(ins);
    if (tins->code == IFTuple) {
      Py_INCREF(ins);
    } else {
      // Build a tuple to pass this in
      PyObject* tup = PyTuple_New(1);
      PyTuple_SET_ITEM(tup,0,ins);
      Py_INCREF(ins);
      ins = module_addtype_chain<IFTuple>(self,tup,NULL,namelist,0);
      if (!ins) return NULL;
      Py_DECREF(tup);
    }
  }
  else if (PyTuple_Check(ins)) {
    // With a tuple, just construct the new IFTuple from those
    if (PyTuple_GET_SIZE(ins) == 0) {
      ins = NULL;
    } else {
      ins = module_addtype_chain<IFTuple>(self,ins,NULL,namelist,0);
      if (!ins) return NULL;
    }
  }
  else if (ins == Py_None) {
    ins = NULL;
  }
  else {
    return PyErr_Format(PyExc_TypeError,"function input cannot be %s",ins->ob_type->tp_name);
  }

  // Outputs may be a tupletype, a tuple of types, a normal type, but not None
  if (PyObject_IsInstance(outs,(PyObject*)&pIF1_TypeType)) {
    pIF1_TypeObject* touts = reinterpret_cast<pIF1_TypeObject*>(outs);
    if (touts->code == IFTuple) {
      Py_INCREF(outs);
    } else {
      // Build a tuple to pass this in
      PyObject* tup = PyTuple_New(1);
      PyTuple_SET_ITEM(tup,0,outs);
      Py_INCREF(outs);
      outs = module_addtype_chain<IFTuple>(self,tup,NULL,namelist,0);
      if (!outs) return NULL;
      Py_DECREF(tup);
    }
  }
  else if (PyTuple_Check(outs)) {
    // With a tuple, just construct the new IFTuple from those
    if (PyTuple_GET_SIZE(outs) == 0) return PyErr_Format(PyExc_ValueError,"Cannot have an empty output type");
    outs = module_addtype_chain<IFTuple>(self,outs,NULL,namelist,0);
    if (!outs) return NULL;
  }
  else {
    Py_XDECREF(ins);
    return PyErr_Format(PyExc_TypeError,"function output cannot be %s",outs->ob_type->tp_name);
  }

  PyObject* result = rawtype(self,IFFunction,0,ins,outs,name);
  Py_XDECREF(ins);
  Py_XDECREF(outs);
  return result;
}

PyObject* module_addtype(PyObject* pySelf,PyObject* args,PyObject* kwargs) {
  pIF1_ModuleObject* self = reinterpret_cast<pIF1_ModuleObject*>(pySelf);
  PyObject* name /*borrowed*/ = NULL;
  PyObject* namelist /*owned*/ = NULL;
  if (kwargs) {
    name = PyDict_GetItemString(kwargs,"name");
    PyDict_DelItemString(kwargs,"name");

    PyObject* names = PyDict_GetItemString(kwargs,"names");
    PyDict_DelItemString(kwargs,"names");
    if (names) {
      PyObject* namesiter /*owned*/ = PyObject_GetIter(names);
      if (!namesiter) return NULL;
      PyObject* item;
      namelist = PyList_New(0);
      if (!namelist) { Py_DECREF(namesiter); return NULL; }
      while((PyErr_Clear(), item=PyIter_Next(namesiter))) {
	int a = PyList_Append(namelist,item);
	Py_DECREF(item);
	if(a<0) {
	  Py_DECREF(namesiter);
	  Py_DECREF(namelist);
	  return NULL;
	}
	Py_DECREF(item);
      }
      Py_DECREF(namesiter);
    }
  }

  if (PyTuple_GET_SIZE(args) < 1) {
    return PyErr_Format(PyExc_TypeError,"the code is required");
  }
  long code = PyInt_AsLong(PyTuple_GET_ITEM(args,0));
  if ( code < 0 && PyErr_Occurred() ) return NULL;

  PyObject* result = NULL;
  switch(code) {
  case IFArray:
    result = module_addtype_array(self,args,name, namelist);
    break;
  case IFBasic: 
    result = module_addtype_basic(self,args,name, namelist);
    break;
  case IFField: 
    result = module_addtype_chain<IFField>(self,args,name, namelist,1);
    break;
  case IFFunction: 
    result = module_addtype_function(self,args,name, namelist);
    break;
  case IFMultiple: 
    result = module_addtype_array(self,args,name, namelist);
    break;
  case IFRecord: 
    result = module_addtype_structure<IFRecord,IFField>(self,args,name, namelist);
    break;
  case IFStream: 
    result = module_addtype_array(self,args,name, namelist);
    break;
  case IFTag: 
    result = module_addtype_chain<IFTag>(self,args,name, namelist,1);
    break;
  case IFTuple: 
    result = module_addtype_chain<IFTuple>(self,args,name, namelist,1);
    break;
  case IFUnion: 
    result = module_addtype_structure<IFUnion,IFTag>(self,args,name, namelist);
    break;
  default:
    result = PyErr_Format(PyExc_ValueError,"Unknown type code %ld",code);
  }

  Py_XDECREF(namelist);
  return result;
}

static PyMethodDef module_methods[] = {
  {(char*)"addtype",(PyCFunction)module_addtype,METH_VARARGS|METH_KEYWORDS,(char*)"adds a new type"},
  {NULL}  /* Sentinel */
};

static PyGetSetDef module_getset[] = {
  {(char*)"if1",module_if1,NULL,(char*)"IF1 code for entire module",NULL},
  {NULL}
};

static PyMethodDef pIF1_methods[] = {
    {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initif1(void) 
{
    PyObject* m;

    pIF1_ModuleType.tp_new = PyType_GenericNew;
    pIF1_ModuleType.tp_init = module_init;
    pIF1_ModuleType.tp_dealloc = module_dealloc;
    pIF1_ModuleType.tp_methods = module_methods;
    pIF1_ModuleType.tp_str = module_str;
    pIF1_ModuleType.tp_repr = module_str;
    pIF1_ModuleType.tp_members = module_members;
    pIF1_ModuleType.tp_getset = module_getset;
    pIF1_ModuleType.tp_getattro = PyObject_GenericGetAttr;
    pIF1_ModuleType.tp_dictoffset = offsetof(pIF1_ModuleObject,dict);
    if (PyType_Ready(&pIF1_ModuleType) < 0)
        return;

    pIF1_TypeType.tp_dealloc = type_dealloc;
    pIF1_TypeType.tp_str = type_str;
    pIF1_TypeType.tp_repr = type_str;
    pIF1_TypeType.tp_members = type_members;
    pIF1_TypeType.tp_getset = type_getset;
    pIF1_TypeType.tp_as_number = &type_number;
    type_number.nb_int = type_int;
    if (PyType_Ready(&pIF1_TypeType) < 0)
        return;

    m = Py_InitModule3("pIF1", pIF1_methods,
                       "Example module that creates an extension type.");

    Py_INCREF(&pIF1_ModuleType);
    PyModule_AddObject(m, "Module", (PyObject *)&pIF1_ModuleType);
    Py_INCREF(&pIF1_TypeType);
    PyModule_AddObject(m, "Type", (PyObject *)&pIF1_TypeType);

    PyModule_AddIntMacro(m,IFArray);
    PyModule_AddIntMacro(m,IFBasic);
    PyModule_AddIntMacro(m,IFField);
    PyModule_AddIntMacro(m,IFFunction);
    PyModule_AddIntMacro(m,IFMultiple);
    PyModule_AddIntMacro(m,IFRecord);
    PyModule_AddIntMacro(m,IFStream);
    PyModule_AddIntMacro(m,IFTag);
    PyModule_AddIntMacro(m,IFTuple);
    PyModule_AddIntMacro(m,IFUnion);
    PyModule_AddIntMacro(m,IFWild);

    PyModule_AddIntMacro(m,IFBoolean);
    PyModule_AddIntMacro(m,IFCharacter);
    PyModule_AddIntMacro(m,IFDoubleReal);
    PyModule_AddIntMacro(m,IFInteger);
    PyModule_AddIntMacro(m,IFNull);
    PyModule_AddIntMacro(m,IFReal);
    PyModule_AddIntMacro(m,IFWildBasic);

}
