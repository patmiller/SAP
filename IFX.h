#ifndef IFX_h
#define IFX_h

#include "Python.h"
#include "structmember.h"
#include <map>
#include <vector>
#include <string>
#include <memory>

#define STATIC_STR(name,value)  \
  static PyObject* name = nullptr;			\
  if (!name) name = PyString_FromString(value);
 
class PyOwned {
  PyObject* P = nullptr;
 public:
  explicit PyOwned(PyObject* P=nullptr) : P(P) {}
  PyOwned(PyOwned const&) = delete;
  PyOwned& operator=(PyOwned const&) = delete;
  PyOwned& operator=(PyObject* N) {
    Py_XDECREF(P);
    P = N;
    return *this;
  }
  ~PyOwned() {
    Py_XDECREF(P);
  }

  bool operator!() const { return !P; }
  operator bool() const { return P != nullptr; }
  PyObject* borrow() { return P; }
  PyObject* incref() { Py_XINCREF(P); return P; }

};


template<class T>
class IF1 : public std::enable_shared_from_this<IF1<T>> {
public:
  IF1() {}
  virtual ~IF1() {}

  struct python {
    PyObject_HEAD
    std::shared_ptr<T> cxx;
  };

  static const long flags = Py_TPFLAGS_DEFAULT;

  std::shared_ptr<T> shared() {
    return std::dynamic_pointer_cast<T>(this->shared_from_this());
  }

  static std::shared_ptr<T>& shared(PyObject* o) {
    python* p = reinterpret_cast<python*>(o);
    return p->self;
  }

  PyObject* package() {
    auto p = std::dynamic_pointer_cast<T>(this->shared_from_this());

    // Make a new Python object shell and use placement new to copy in
    // the shared pointer
    PyOwned  o(PyType_GenericNew(&T::Type,NULL,NULL));
    if (!o) return nullptr;
    new (&reinterpret_cast<python*>(o.borrow())->cxx) std::shared_ptr<T>(nullptr);
    reinterpret_cast<python*>(o.borrow())->cxx = p;
    return o.incref();
  }

  static int init(PyObject* pySelf,PyObject* args,PyObject* kwargs) {
    // Set to a known state for the destructor
    auto self = reinterpret_cast<python*>(pySelf);
    new (&self->cxx) std::shared_ptr<T>(nullptr);

    try {
      self->cxx = std::make_shared<T>(self,args,kwargs);
    }

    // Threw a C++ exception
    catch (std::exception& e) {
      PyErr_SetString(PyExc_RuntimeError,e.what());
      return -1;
    }

    // Threw a Python exception
    catch (PyObject*) {
      return -1;
    }

    return 0;
  }
   
  static void dealloc(PyObject* pySelf) {
    auto self = reinterpret_cast<python*>(pySelf);
    ~std::shared_ptr<T>(&self->cxx);
  }

  static PyObject* getattro(PyObject* self,PyObject* attr) {
    return PyObject_GenericGetAttr(self,attr);
  }
  static int setattro(PyObject* self,PyObject* attr,PyObject* rhs) {
    return PyObject_GenericSetAttr(self,attr,rhs);
  }

  static bool ready(PyObject* module,char const* name) {
    PyOwned mname(PyObject_GetAttrString(module,"__name__"));
    if (!mname) return false;
    if (!PyString_Check(mname.borrow())) return false;
    std::string module_name(PyString_AS_STRING(mname.borrow()));
    std::string fully_qualified_name = module_name + '.' + name;

    T::Type.tp_name = ::strcpy(new char[fully_qualified_name.size()+1],fully_qualified_name.c_str()); // Leaks
    T::Type.tp_basicsize = sizeof(python);
    T::Type.tp_doc = T::doc;
    T::Type.tp_flags = T::flags;
    T::Type.tp_base = T::basetype();
    T::Type.tp_new = PyType_GenericNew;
    T::Type.tp_init = T::init;
    T::Type.tp_str = T::Type.tp_repr = T::str;
    if (T::methods[0].ml_name) {
      // Only set up if we have methods defined
      T::Type.tp_methods = T::methods;
    }
    if (T::getset[0].name) {
      // Only set up if we have getset defined
      T::Type.tp_getset = T::getset;
    }
    T::Type.tp_getattro = T::getattro;
    T::Type.tp_setattro = T::setattro;

    T::setup();
    T::Type.tp_as_number = &T::as_number;
    T::Type.tp_as_sequence = &T::as_sequence;
    //T::Type.tp_richcompare
    //T::Type.tp_
    //T::Type.tp_
    //T::Type.tp_

    if (PyType_Ready(&T::Type) < 0) return false;
    Py_INCREF(&T::Type);
    PyModule_AddObject(module,name,reinterpret_cast<PyObject*>(&T::Type));
    return true;
  }

  static void setup() {
  }

  static PyTypeObject* basetype() {
    return nullptr;
  }

  static PyObject* str(PyObject* self) {
    auto cxx = reinterpret_cast<python*>(self)->cxx;
    if (!cxx) return PyErr_Format(PyExc_RuntimeError,"broken ptr");
    return cxx->string(self);
  }
  virtual PyObject* string(PyObject* self) {
    char const* dot = ::rindex(self->ob_type->tp_name,'.');
    return PyString_FromFormat("<%s>",dot?(dot+1):self->ob_type->tp_name);
  }
  static T* borrow(PyObject* o) {
    auto p = reinterpret_cast<python*>(o);
    return p->cxx.get();
  }

  static PyObject* pragma_string(PyObject* dict,PyObject* auxdict=nullptr) {
    static PyObject* SPACEPERCENT = nullptr;
    if (!SPACEPERCENT) SPACEPERCENT = PyString_FromString(" %");
    static PyObject* EQUAL = nullptr;
    if (!EQUAL) EQUAL = PyString_FromString("=");

    // Get keys from the main dict.   We will sort these later
    PyObject* sorted = PyList_New(0);
    PyObject* key;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict, &pos, &key, nullptr)) {
      // Only string keys of size 2 are pragmas
      if (!PyString_Check(key) || PyString_GET_SIZE(key) != 2) continue;
      if (PyList_Append(sorted,key) != 0) { Py_DECREF(sorted); return nullptr; }
    }

    if (auxdict) {
      pos = 0;
      while (PyDict_Next(auxdict, &pos, &key, nullptr)) {
	// Only string keys of size 2 are pragmas
	if (!PyString_Check(key) || PyString_GET_SIZE(key) != 2) continue;
	bool match = false;
	for(ssize_t i=0; i<PyList_GET_SIZE(sorted); ++i) {
	  int cmp = -1;
	  if (PyObject_Cmp(key,PyList_GET_ITEM(sorted,i),&cmp) == 0 && cmp == 0) {
	    match = true;
	    break;
	  }
	}
	if (!match) {
	  if (PyList_Append(sorted,key) != 0) { Py_DECREF(sorted); return nullptr; }
	}
      }
    }
    if (PyList_Sort(sorted) != 0) { Py_DECREF(sorted); return nullptr; }

    PyObject* result = PyString_FromString("");
    for(ssize_t i=0; i < PyList_GET_SIZE(sorted); ++i) {
      PyObject* key /*borrowed*/ = PyList_GET_ITEM(sorted,i);
      PyObject* pragma /*borrowed*/ = PyDict_GetItem(dict,key);
      if (!pragma && auxdict) {
	pragma = PyDict_GetItem(auxdict,key);
      }
      if (!pragma) continue;

      PyString_Concat(&result,SPACEPERCENT);
      if (!result) return nullptr;
      PyString_Concat(&result,key);
      if (!result) return nullptr;
      PyString_Concat(&result,EQUAL);
      if (!result) return nullptr;
      PyObject* rhs /*borrowed*/ = PyObject_Str(pragma);
      if (!rhs) { Py_DECREF(result); return nullptr; }
      PyString_ConcatAndDel(&result,rhs);
      if (!result) return nullptr;
    }
    return result;
  }
};

enum IFTypeCode {
  IF_Array = 0,
  IF_Basic = 1,
  IF_Field = 2,
  IF_Function = 3,
  IF_Multiple = 4,
  IF_Record = 5,
  IF_Stream = 6,
  IF_Tag = 7,
  IF_Tuple = 8,
  IF_Union = 9,
  IF_Wild = 10
};

enum IFBasics {
  IF_Boolean = 0,
  IF_Character = 1,
  IF_DoubleReal = 2,
  IF_Integer = 3,
  IF_Null = 4,
  IF_Real = 5,
  IF_WildBasic = 6
};

enum IF1Opcodes {
  IFAAddH = 100,
  IFAAddL = 101,
  IFAAdjust = 102,
  IFABuild = 103,
  IFACatenate = 104,
  IFAElement = 105,
  IFAFill = 106,
  IFAGather = 107,
  IFAIsEmpty = 108,
  IFALimH = 109,
  IFALimL = 110,
  IFARemH = 111,
  IFARemL = 112,
  IFAReplace = 113,
  IFAScatter = 114,
  IFASetL = 115,
  IFASize = 116,
  IFAbs = 117,
  IFBindArguments = 118,
  IFBool = 119,
  IFCall = 120,
  IFChar = 121,
  IFDiv = 122,
  IFDouble = 123,
  IFEqual = 124,
  IFExp = 125,
  IFFirstValue = 126,
  IFFinalValue = 127,
  IFFloor = 128,
  IFInt = 129,
  IFIsError = 130,
  IFLess = 131,
  IFLessEqual = 132,
  IFMax = 133,
  IFMin = 134,
  IFMinus = 135,
  IFMod = 136,
  IFNeg = 137,
  IFNoOp = 138,
  IFNot = 139,
  IFNotEqual = 140,
  IFPlus = 141,
  IFRangeGenerate = 142,
  IFRBuild = 143,
  IFRElements = 144,
  IFRReplace = 145,
  IFRedLeft = 146,
  IFRedRight = 147,
  IFRedTree = 148,
  IFReduce = 149,
  IFRestValues = 150,
  IFSingle = 151,
  IFTimes = 152,
  IFTrunc = 153,
  IFPrefixSize = 154,
  IFError = 155,
  IFReplaceMulti = 156,
  IFConvert = 157,
  IFCallForeign = 158,
  IFAElementN = 159,
  IFAElementP = 160,
  IFAElementM = 161,

  IFSGraph = 1000,
  IFLGraph = 1001,
  IFIGraph = 1002,
  IFXGraph = 1003,
  IFLPGraph = 1004,
  IFRLGraph = 1005
};

#endif
