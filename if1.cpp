#include "Python.h"
#include "structmember.h"
#include "IFX.h"
#include <map>
#include <vector>
#include <string>

PyObject* OPNAMES = nullptr;
PyObject* OPCODES = nullptr;

PyObject* FUNCTIONTYPE = nullptr;

PyObject* ADDTYPE = nullptr; // addtype
PyObject* ARROW = nullptr; // ->
PyObject* COLON = nullptr; // :
PyObject* COMMA = nullptr; // ,
PyObject* LPAREN = nullptr; // (
PyObject* RPAREN = nullptr; // )
PyObject* SPACE = nullptr;
PyObject* DQUOTE = nullptr; // "
PyObject* NEWLINE = nullptr;
PyObject* SPACEPERCENT = nullptr; // ' %'
PyObject* EQUAL = nullptr; // =

enum literal_parser_state_t {
  ERROR = 0,
  START,
  INTEGER,
  REAL,
  REAL_EXP_OPTSIGN,
  REAL_EXP_DIGIT,
  REAL_EXP,
  DOUBLEREAL_EXP_OPTSIGN,
  DOUBLEREAL_EXP_DIGIT,
  DOUBLEREAL_EXP,
  NIL0,
  NIL1,
  NIL2,
  TRUE0,
  TRUE1,
  TRUE2,
  TRUE3,
  FALSE0,
  FALSE1,
  FALSE2,
  FALSE3,
  FALSE4,
  CHAR_BODY,
  CHAR_ESCAPE,
  CHAR_END,
  CHAR_NULL,
  STRING_BODY,
  STRING_ESCAPE,
  STRING_END,
  NSTATES,		// Just marks the last state
  BOOLEAN_LITERAL = -1,
  CHAR_LITERAL = -2,
  DOUBLEREAL_LITERAL = -3,
  INTEGER_LITERAL = -4,
  NULL_LITERAL = -5,
  REAL_LITERAL = -6,
  STRING_LITERAL = -7
};

// ----------------------------------------------------------------------
// Convert a dictionary into pragmas.
//
// Emit the two character pragma values in lexical order for sanity and predictability
// ----------------------------------------------------------------------
static PyObject* pragmas(PyObject* dict,PyObject* auxdict = nullptr) {
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

bool initialize_literal_parser_table(literal_parser_state_t table[]) {
  // Initialize all entries to ERROR
  for(int i=0;i<NSTATES*256;++i) table[i] = ERROR;

  // ----------------------------------------------------------------------
  // In the start state, we figure out what kind of literal this "looks like"
  // ----------------------------------------------------------------------
  table[START*256+'n'] = NIL0;
  table[START*256+'N'] = NIL0;
  table[START*256+'t'] = TRUE0;
  table[START*256+'T'] = TRUE0;
  table[START*256+'f'] = FALSE0;
  table[START*256+'F'] = FALSE0;
  table[START*256+'0'] = INTEGER;
  table[START*256+'1'] = INTEGER;
  table[START*256+'2'] = INTEGER;
  table[START*256+'3'] = INTEGER;
  table[START*256+'4'] = INTEGER;
  table[START*256+'5'] = INTEGER;
  table[START*256+'6'] = INTEGER;
  table[START*256+'7'] = INTEGER;
  table[START*256+'8'] = INTEGER;
  table[START*256+'9'] = INTEGER;
  table[START*256+'+'] = INTEGER;
  table[START*256+'-'] = INTEGER;
  table[START*256+'\''] = CHAR_BODY;
  table[START*256+'"'] = STRING_BODY;

  // ----------------------------------------------------------------------
  // Booleans are true or false
  // ----------------------------------------------------------------------
  table[TRUE0*256+'r'] = TRUE1;
  table[TRUE0*256+'R'] = TRUE1;
  table[TRUE1*256+'u'] = TRUE2;
  table[TRUE1*256+'U'] = TRUE2;
  table[TRUE2*256+'e'] = TRUE3;
  table[TRUE2*256+'E'] = TRUE3;
  table[TRUE3*256+0] = BOOLEAN_LITERAL;

  table[FALSE0*256+'a'] = FALSE1;
  table[FALSE0*256+'A'] = FALSE1;
  table[FALSE1*256+'l'] = FALSE2;
  table[FALSE1*256+'L'] = FALSE2;
  table[FALSE2*256+'s'] = FALSE3;
  table[FALSE2*256+'S'] = FALSE3;
  table[FALSE3*256+'e'] = FALSE4;
  table[FALSE3*256+'E'] = FALSE4;
  table[FALSE4*256+0] = BOOLEAN_LITERAL;

  // ----------------------------------------------------------------------
  // Characters are single quote with an escape
  // ----------------------------------------------------------------------
  for(char c=' '; c < 127; ++c) {
    if (c == '\\') {
      table[CHAR_BODY*256+c] = CHAR_ESCAPE;
    } else if (c == '\'') {
      table[CHAR_BODY*256+c] = ERROR;
    } else {
      table[CHAR_BODY*256+c] = CHAR_END;
    }
  }

  for(char c=' '; c < 127; ++c) {
    table[CHAR_ESCAPE*256+c] = CHAR_END;
  }

  table[CHAR_END*256+'\''] = CHAR_NULL;
  table[CHAR_NULL*256+0] = CHAR_LITERAL;

  // ----------------------------------------------------------------------
  // DoubleReal
  // ----------------------------------------------------------------------      
  table[DOUBLEREAL_EXP_OPTSIGN*256+0] = DOUBLEREAL_LITERAL;
  table[DOUBLEREAL_EXP_OPTSIGN*256+'+'] = DOUBLEREAL_EXP_DIGIT;
  table[DOUBLEREAL_EXP_OPTSIGN*256+'-'] = DOUBLEREAL_EXP_DIGIT;
  for(char c='0'; c <= '9'; ++c) {
    table[DOUBLEREAL_EXP_OPTSIGN*256+c] = DOUBLEREAL_EXP;
  }

  for(char c='0'; c <= '9'; ++c) {
    table[DOUBLEREAL_EXP_DIGIT*256+c] = DOUBLEREAL_EXP;
  }

  table[DOUBLEREAL_EXP*256+0] = DOUBLEREAL_LITERAL;
  for(char c='0'; c <= '9'; ++c) {
    table[DOUBLEREAL_EXP*256+c] = DOUBLEREAL_EXP;
  }

  // ----------------------------------------------------------------------
  // Integer
  // ----------------------------------------------------------------------  
  table[INTEGER*256+0] = INTEGER_LITERAL;
  for(char c='0'; c <= '9'; ++c) {
    table[INTEGER*256+c] = INTEGER;
  }
  table[INTEGER*256+'.'] = REAL;
  table[INTEGER*256+'e'] = REAL_EXP_OPTSIGN;
  table[INTEGER*256+'E'] = REAL_EXP_OPTSIGN;
  table[INTEGER*256+'d'] = DOUBLEREAL_EXP_OPTSIGN;
  table[INTEGER*256+'D'] = DOUBLEREAL_EXP_OPTSIGN;

  // ----------------------------------------------------------------------
  // Null
  // ----------------------------------------------------------------------      
  table[NIL0*256+'i'] = NIL1;
  table[NIL0*256+'I'] = NIL1;
  table[NIL1*256+'l'] = NIL2;
  table[NIL1*256+'L'] = NIL2;
  table[NIL2*256+  0] = NULL_LITERAL;

  // ----------------------------------------------------------------------
  // Real
  // ----------------------------------------------------------------------      
  table[REAL*256+0] = REAL_LITERAL;
  for(char c='0'; c <= '9'; ++c) {
    table[REAL*256+c] = REAL;
  }
  table[REAL*256+'e'] = REAL_EXP_OPTSIGN;
  table[REAL*256+'E'] = REAL_EXP_OPTSIGN;
  table[REAL*256+'d'] = DOUBLEREAL_EXP_OPTSIGN;
  table[REAL*256+'D'] = DOUBLEREAL_EXP_OPTSIGN;
      
  table[REAL_EXP_OPTSIGN*256+0] = REAL_LITERAL;
  table[REAL_EXP_OPTSIGN*256+'+'] = REAL_EXP_DIGIT;
  table[REAL_EXP_OPTSIGN*256+'-'] = REAL_EXP_DIGIT;
  for(char c='0'; c <= '9'; ++c) {
    table[REAL_EXP_OPTSIGN*256+c] = REAL_EXP;
  }

  for(char c='0'; c <= '9'; ++c) {
    table[REAL_EXP_DIGIT*256+c] = REAL_EXP;
  }

  table[REAL_EXP*256+0] = REAL_LITERAL;
  for(char c='0'; c <= '9'; ++c) {
    table[REAL_EXP*256+c] = REAL_EXP;
  }

  // ----------------------------------------------------------------------
  // String
  // ----------------------------------------------------------------------
  for(char c=' '; c < 127; ++c) {
    if (c == '\\') {
      table[STRING_BODY*256+c] = STRING_ESCAPE;
    } else if (c == '"') {
      table[STRING_BODY*256+c] = STRING_END;
    } else {
      table[STRING_BODY*256+c] = STRING_BODY;
    }
  }
  for(char c=' '; c < 127; ++c) {
    table[STRING_ESCAPE*256+c] = STRING_BODY;
  }
  table[STRING_END*256+0] = STRING_LITERAL;

  return true;
}



// ----------------------------------------------------------------------
// InPort
// ----------------------------------------------------------------------
const char* inport_doc = "TBD: input port";

static PyTypeObject IF1_InPortType;

typedef struct {
  PyObject_HEAD

  PyObject* weakdst;
  ssize_t port;

} IF1_InPortObject;

static PyNumberMethods inport_number;

// ----------------------------------------------------------------------
// Node
// ----------------------------------------------------------------------
const char* node_doc = "TBD: node";

static PyTypeObject IF1_NodeType;

struct Edge;
struct PortInfo;
typedef struct {
  PyObject_HEAD
  PyObject* weakmodule;
  PyObject* weakparent;
  long opcode;
  
  PyObject* children;

  PyObject* pragmas;

  std::map<long,Edge>* edges;
  std::map<long,PortInfo>* outputs;

  // Weak reference support
  PyObject* weak;
} IF1_NodeObject;

PyNumberMethods node_number;
PySequenceMethods node_sequence;

static PyMemberDef node_members[] = {
  {(char*)"opcode",T_LONG,offsetof(IF1_NodeObject,opcode),0,(char*)"op code: IFPlus, IFMinus, ..."},
  {(char*)"children",T_OBJECT_EX,offsetof(IF1_NodeObject,children),READONLY,(char*)"children of compounds"},
  {(char*)"pragmas",T_OBJECT_EX,offsetof(IF1_NodeObject,pragmas),READONLY,(char*)"pragma dictionary"},
  {nullptr}
};

// ----------------------------------------------------------------------
// OutPort and PortInfo
// ----------------------------------------------------------------------
const char* outport_doc = "TBD: output port";
static PyTypeObject IF1_OutPortType;

typedef struct {
  PyObject_HEAD

  PyObject* weaksrc;
  ssize_t port;

} IF1_OutPortObject;

struct PortInfo {
private:
  PyObject* weaktype;
  PyObject* dict;
public:
  PortInfo() : weaktype(nullptr), dict(PyDict_New()) {}

  PortInfo(const PortInfo& other) {
    this->operator=(other);
  }

  PortInfo& operator=(const PortInfo& other) {
    if (this != &other) {
      Py_XDECREF(weaktype);
      Py_XDECREF(dict);

      Py_XINCREF(weaktype = other.weaktype);
      Py_XINCREF(dict = other.dict);
    }
    return *this;
  }

  ~PortInfo() {
    Py_XDECREF(weaktype);
    Py_DECREF(dict);
  }

  PyObject* /*borrowed*/ pragmas() {
    return dict;
  }

  static PortInfo* portinfo_from_outport(PyObject* outport) {
    IF1_OutPortObject* self = reinterpret_cast<IF1_OutPortObject*>(outport);
    PyObject* node /*borrowed*/ = PyWeakref_GET_OBJECT(self->weaksrc);
    if (node == Py_None) {
      PyErr_Format(PyExc_RuntimeError,"iport is disconnected from node");
      return nullptr;
    }
    IF1_NodeObject* N = reinterpret_cast<IF1_NodeObject*>(node);

    std::map<long,PortInfo>::iterator p = N->outputs->find(self->port);
    if (p == N->outputs->end()) {
      PyErr_Format(PyExc_IndexError,"No such port %zd",self->port);
      return nullptr;
    }

    return &p->second;
  }

  int settype(PyObject* T) {
    Py_XDECREF(weaktype);
    weaktype = PyWeakref_NewRef(T,0);
    if (!weaktype) return -1;
    return 0;
  }

  PyObject* /*owned*/ gettype() {
    PyObject* type /*borrowed*/ = PyWeakref_GET_OBJECT(weaktype);
    if (type == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected type");
    Py_INCREF(type);
    return type;
  }
};


static PyNumberMethods outport_number;

// ----------------------------------------------------------------------
// Edge
// ----------------------------------------------------------------------
struct Edge {
  std::string value;
  PyObject* literal_type;

  PyObject* weaksrc;
  long      oport;

  PyObject* pragmas;

  Edge() : literal_type(nullptr),weaksrc(nullptr),oport(0),pragmas(PyDict_New()) {}
  Edge(const char* value, PyObject* type) : value(value), literal_type(type), weaksrc(nullptr), oport(0), pragmas(PyDict_New()) {
    Py_XINCREF(type);
  }
  Edge(PyObject* weaksrc, long oport) : literal_type(nullptr), weaksrc(weaksrc), oport(oport), pragmas(PyDict_New()) {
    Py_XINCREF(weaksrc);
  }
  ~Edge() {
    Py_XDECREF(literal_type);
    Py_XDECREF(weaksrc);
    Py_XDECREF(pragmas);
  }
  Edge(const Edge& other) {
    this->operator=(other);
  }
  Edge& operator=(const Edge& other) {
    if (&other != this) {
      Py_XDECREF(literal_type);
      Py_XDECREF(weaksrc);
      Py_XDECREF(pragmas);
      
      value = other.value;
      Py_XINCREF(literal_type = other.literal_type);

      Py_XINCREF(weaksrc = other.weaksrc);
      oport = other.oport;

      Py_XINCREF(pragmas = other.pragmas);
    }
    return *this;
  }


  static Edge* edge_from_inport(PyObject* inport) {
    IF1_InPortObject* self = reinterpret_cast<IF1_InPortObject*>(inport);
    PyObject* node /*borrowed*/ = PyWeakref_GET_OBJECT(self->weakdst);
    if (node == Py_None) {
      PyErr_Format(PyExc_RuntimeError,"iport is disconnected from node");
      return nullptr;
    }

    IF1_NodeObject* N = reinterpret_cast<IF1_NodeObject*>(node);
    std::map<long,Edge>::iterator p = N->edges->find(self->port);
    if ( p == N->edges->end() ) {
      PyErr_Format(PyExc_IndexError,"unbound input port %ld",self->port);
      return nullptr;
    }
    return &p->second;
  }

  PyObject* /*owned*/ get_literal() {
    if (value.size()) {
      return PyString_FromStringAndSize(value.c_str(),value.size());
    }
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyObject* /*borrowed*/ get_type() {
    if (oport == 0) {
      return literal_type?literal_type:Py_None;
    }

    PyObject* src /*borrowed*/ = get_src();
    if (!src) return nullptr;
    IF1_NodeObject* N = reinterpret_cast<IF1_NodeObject*>(src);

    std::map<long,PortInfo>::iterator p = N->outputs->find(oport);
    if (p == N->outputs->end()) {
      return PyErr_Format(PyExc_NotImplementedError,"edge was disconnected");
    }
    return p->second.gettype();
  }

  long get_type_label() {
    PyObject* T /*borrowed*/ = get_type();
    if (T == Py_None) return 0;

    PyObject* pylabel /*owned*/ = PyNumber_Int(T);
    if (!pylabel) { PyErr_Clear(); return 0; }
    long label = PyInt_AsLong(pylabel);
    if (label < 0) { PyErr_Clear(); return 0; }
    return label;
  }

  PyObject* /*borrowed*/ get_src() {
    if (!weaksrc) return Py_None;
    return PyWeakref_GET_OBJECT(weaksrc);
  }

  PyObject* /*owned*/ get_oport() {
    return PyInt_FromLong(oport);
  }

  long get_src_label() {
    PyObject* src = get_src();
    PyObject* pylabel /*owned*/ = PyNumber_Int(src);
    long elabel = PyInt_AsLong(pylabel);
    Py_DECREF(pylabel);
    return elabel;
  }

  PortInfo* /*borrowed*/ get_portinfo() {
    if (!oport) return nullptr;

    PyObject* src /*borrowed*/ = get_src();
    if (src == Py_None) return nullptr;

    IF1_NodeObject* N = reinterpret_cast<IF1_NodeObject*>(src);

    std::map<long,PortInfo>::iterator p = N->outputs->find(oport);
    if (p == N->outputs->end()) return nullptr;

    return &p->second;
  }

};

// ----------------------------------------------------------------------
// Graph
// ----------------------------------------------------------------------
const char* graph_doc = "TBD: Graph (isa Node)";

static PyTypeObject IF1_GraphType;

typedef struct {
  IF1_NodeObject node;

  // Optional name & type (function graphs)
  PyObject* name;
  PyObject* type;

} IF1_GraphObject;

PyNumberMethods graph_number;

static PyMemberDef graph_members[] = {
  {(char*)"name",T_OBJECT,offsetof(IF1_GraphObject,name),0,(char*)"Optional graph name"},
  {(char*)"nodes",T_OBJECT,offsetof(IF1_NodeObject,children),0,(char*)"list of graph nodes"},
  {nullptr}
};


// ----------------------------------------------------------------------
// Type
// ----------------------------------------------------------------------
const char* type_doc =
  "TODO: Add docsn for type\n"
  ;

static PyTypeObject IF1_TypeType;

typedef struct {
  PyObject_HEAD
  PyObject* weakmodule;
  unsigned long code;
  unsigned long aux;
  PyObject* weak1;
  PyObject* weak2;
  PyObject* pragmas;

  // Weak reference slot
  PyObject* weak;
} IF1_TypeObject;

static PyMemberDef type_members[] = {
  {(char*)"code",T_LONG,offsetof(IF1_TypeObject,code),READONLY,(char*)"type code: IF_Array, IF_Basic..."},
  {(char*)"pragmas",T_OBJECT,offsetof(IF1_TypeObject,pragmas),READONLY,(char*)"dictionary of pragma info"},
  {nullptr}  /* Sentinel */
};

PyNumberMethods type_number;

// ----------------------------------------------------------------------
// Module
// ----------------------------------------------------------------------
const char* module_doc =
  "TODO: Add docs for module\n"
  ;

static PyTypeObject IF1_ModuleType;

typedef struct {
  PyObject_HEAD
  PyObject* dict;
  PyObject* types;
  PyObject* pragmas;
  PyObject* functions;

  // Weak reference slot
  PyObject* weak;
} IF1_ModuleObject;


static PyMemberDef module_members[] = {
  {(char*)"__dict__",T_OBJECT_EX,offsetof(IF1_ModuleObject,dict),0,(char*)"instance dictionary"},
  {(char*)"types",T_OBJECT_EX,offsetof(IF1_ModuleObject,types),0,(char*)"type table"},
  {(char*)"pragmas",T_OBJECT_EX,offsetof(IF1_ModuleObject,pragmas),0,(char*)"pragma set"},
  {(char*)"functions",T_OBJECT_EX,offsetof(IF1_ModuleObject,functions),0,(char*)"function graphs"},
  {nullptr}  /* Sentinel */
};

/*
  ###         ###                #    
   #          #  #               #    
   #    ###   #  #   ##   ###   ###   
   #    #  #  ###   #  #  #  #   #    
   #    #  #  #     #  #  #      #    
  ###   #  #  #      ##   #       ##  
*/

void inport_dealloc(PyObject* pySelf) {
  IF1_InPortObject* self = reinterpret_cast<IF1_InPortObject*>(pySelf);

  Py_XDECREF(self->weakdst);

  Py_TYPE(pySelf)->tp_free(pySelf);
}

PyObject* inport_str(PyObject* pySelf) {
  IF1_InPortObject* self = reinterpret_cast<IF1_InPortObject*>(pySelf);

  PyObject* sport /*owned*/ = PyString_FromFormat("%ld",self->port);
  if (!sport) return nullptr;

  PyString_Concat(&sport,COLON);
  if (!sport) return nullptr;

  PyObject* dst /*borrowed*/ = PyWeakref_GET_OBJECT(self->weakdst);
  if (dst == Py_None) { Py_DECREF(sport); return PyErr_Format(PyExc_ValueError,"in port is disconnected"); }
  PyObject* str /*owned*/ = PyObject_Str(dst);
  if (!str) { Py_DECREF(sport); return nullptr; }
  
  PyString_ConcatAndDel(&sport,str);
  return sport;
}

PyObject* inport_literal(PyObject* module, PyObject* literal) {

    static literal_parser_state_t table[NSTATES*256];
    static bool table_ready = false;
    if (!table_ready) table_ready = initialize_literal_parser_table(table);

    const char* p = PyString_AS_STRING(literal);
    literal_parser_state_t state = START;
    while(state > 0) {
      state = table[state*256+*p++];
    }

    switch(state) {
    case BOOLEAN_LITERAL: return PyObject_GetAttrString(module,"boolean");
    case CHAR_LITERAL: return PyObject_GetAttrString(module,"character");
    case DOUBLEREAL_LITERAL: return PyObject_GetAttrString(module,"doublereal");
    case INTEGER_LITERAL: return PyObject_GetAttrString(module,"integer");
    case NULL_LITERAL: return PyObject_GetAttrString(module,"null");
    case REAL_LITERAL: return PyObject_GetAttrString(module,"real");
    case STRING_LITERAL: return PyObject_GetAttrString(module,"string");

    case ERROR: return PyErr_Format(PyExc_ValueError,"unparseable literal: %s at %c",PyString_AS_STRING(literal),*p);
    default:
      ;
    }
    return PyErr_Format(PyExc_NotImplementedError,"should be impossible");
}

static PyObject* inport_dst(PyObject* pySelf) {
  IF1_InPortObject* self = reinterpret_cast<IF1_InPortObject*>(pySelf);
  PyObject* dst /*borrowed*/ = PyWeakref_GET_OBJECT(self->weakdst);
  if (dst == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected port");
  return dst;
}

static PyObject* inport_int(PyObject* pySelf) {
  IF1_InPortObject* self = reinterpret_cast<IF1_InPortObject*>(pySelf);
  return PyInt_FromLong(self->port);
}

static PyObject* node_module(PyObject* pySelf);
static PyObject* inport_lshift(PyObject* pySelf, PyObject* value) {
  IF1_InPortObject* self = reinterpret_cast<IF1_InPortObject*>(pySelf);

  // Remove any old value stored at this port
  long port = self->port;
  
  IF1_NodeObject* dst /*borrowed*/ = reinterpret_cast<IF1_NodeObject*>(inport_dst(pySelf));
  if (!dst) return nullptr;

  // Sending in a None will erase this port
  if (value == Py_None) {
    dst->edges->erase(self->port);
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyObject* module /*borrowed*/ = node_module((PyObject*)dst);
  if (!module) return nullptr;

  // Do we have a good literal?  Figure out the type (either from the type PyInt PyBoolean PyFloat)
  // or by parsing the literal string
  if (PyBool_Check(value)) {
    PyObject* T /*owned*/ = PyObject_GetAttrString(module,"boolean");
    if (!T) return nullptr;
    dst->edges->operator[](port) = Edge(PyInt_AS_LONG(value)?"true":"false",T);
    return T;
  }
  else if (PyInt_Check(value)) {
    PyObject* T /*owned*/ = PyObject_GetAttrString(module,"integer");
    if (!T) return nullptr;
    PyObject* str = PyObject_Str(value);
    if (!str) { Py_DECREF(T); return nullptr; }
    dst->edges->operator[](port) = Edge(PyString_AS_STRING(str),T);
    Py_DECREF(str); 
    return T;
  }
  
  else if (PyFloat_Check(value)) {
    PyObject* T /*owned*/ = PyObject_GetAttrString(module,"doublereal");
    if (!T) return nullptr;
    PyObject* str = PyObject_Str(value);
    if (!str) { Py_DECREF(T); return nullptr; }
    dst->edges->operator[](port) = Edge(PyString_AS_STRING(str),T);
    Py_DECREF(str); 
    return T;
  }

  // Strings
  else if (PyString_Check(value)) {

    PyObject* T /*owned*/ = inport_literal(module,value);
    if (!T) return nullptr;

    dst->edges->operator[](port) = Edge(PyString_AS_STRING(value),T);
    return T;
  }

  else if (PyObject_IsInstance(value,(PyObject*)&IF1_OutPortType)) {
    IF1_OutPortObject* outport = reinterpret_cast<IF1_OutPortObject*>(value);

    // Check to make sure that the source and destination are in the same graph
    PyObject* dst_home /*owned*/ = PyNumber_Positive((PyObject*)dst);
    PyObject* src /*borrowed*/ = PyWeakref_GET_OBJECT(outport->weaksrc);
    PyObject* src_home /*owned*/ = PyNumber_Positive(src);
    bool ok = (dst_home == src_home);
    PyErr_Clear(); // Just clear any errors
    Py_XDECREF(dst_home);
    Py_XDECREF(src_home);
    if (!ok) { return PyErr_Format(PyExc_ValueError,"source and destination are not in compatible graphs"); }

    PyObject* T /*owned*/ = PyObject_GetAttrString(value,"type");
    if (!T) return nullptr;

    dst->edges->operator[](port) = Edge(outport->weaksrc,outport->port);
    return T;
  }

  return PyErr_Format(PyExc_NotImplementedError,"cannot wire an input with type %s",value->ob_type->tp_name);
}

static PyObject* inport_richcompare(PyObject* pySelf, PyObject* pyOther, int op) {
  // types must match
  if (!PyObject_IsInstance(pyOther,(PyObject*)&IF1_InPortType)) return Py_NotImplemented;

  IF1_InPortObject* self = reinterpret_cast<IF1_InPortObject*>(pySelf);
  IF1_InPortObject* other = reinterpret_cast<IF1_InPortObject*>(pyOther);

  PyObject* result = nullptr;
  switch(op) {
  case Py_LT:
  case Py_LE:
    return Py_NotImplemented;
  case Py_EQ:
    if (self == other) {
      result = Py_True;
    } else {
      result = (PyWeakref_GET_OBJECT(self->weakdst) == PyWeakref_GET_OBJECT(self->weakdst) &&
		self->port == other->port)?Py_True:Py_False;
    }
    break;
  case Py_NE:
    if (self == other) {
      result = Py_False;
    } else {
      result = (PyWeakref_GET_OBJECT(self->weakdst) == PyWeakref_GET_OBJECT(self->weakdst) &&
		self->port == other->port)?Py_False:Py_True;
    }
    break;
  case Py_GT:
  case Py_GE:
    return Py_NotImplemented;
  }
  Py_INCREF(result);
  return result;
}

static PyObject* inport_getattro(PyObject* pySelf, PyObject* attr) {
  if (PyString_Check(attr) && PyString_GET_SIZE(attr) == 2) {
    //PyObject* rhs = PyDict_GetItem(self->pragmas,attr);
    //if (rhs) { Py_INCREF(rhs); return rhs; }
    //PyErr_Clear();
    puts("get pragma");
  }
 return PyObject_GenericGetAttr(pySelf,attr);
}
static int inport_setattro(PyObject* pySelf, PyObject* attr, PyObject* rhs) {
  if (PyString_Check(attr) && PyString_GET_SIZE(attr) == 2) {
    Edge* e = Edge::edge_from_inport(pySelf);
    if (e) {
      if (rhs) {
	return PyDict_SetItem(e->pragmas,attr,rhs);
      } else {
	int status = PyDict_DelItem(e->pragmas,attr);
	if (status) {
	  PyErr_Format(PyExc_AttributeError,"no such pragma '%s'",PyString_AS_STRING(attr));
	}
	return status;
      }
    }
  }

  return PyObject_GenericSetAttr(pySelf,attr,rhs);
}

static PyObject* inport_get_node(PyObject* pySelf,void*) {
  IF1_InPortObject* self = reinterpret_cast<IF1_InPortObject*>(pySelf);
  PyObject* node /*borrowed*/ = PyWeakref_GET_OBJECT(self->weakdst);
  if (node == Py_None) return PyErr_Format(PyExc_RuntimeError,"iport is disconnected from node");
  Py_INCREF(node);
  return node;
}
static PyObject* inport_get_port(PyObject* pySelf,void*) {
  IF1_InPortObject* self = reinterpret_cast<IF1_InPortObject*>(pySelf);
  PyObject* node /*borrowed*/ = PyWeakref_GET_OBJECT(self->weakdst);
  if (node == Py_None) return PyErr_Format(PyExc_RuntimeError,"iport is disconnected from node");
  return PyInt_FromLong(self->port);
}
static PyObject* inport_get_literal(PyObject* pySelf,void*) {
  Edge* e = Edge::edge_from_inport(pySelf);
  if (!e) return nullptr;

  return e->get_literal();
}
static PyObject* inport_get_type(PyObject* pySelf,void*) {
  Edge* e = Edge::edge_from_inport(pySelf);
  if (!e) return nullptr;

  PyObject* T /*borrowed*/ = e->get_type();
  Py_XINCREF(T);
  return T;
}
static PyObject* inport_get_src(PyObject* pySelf,void*) {
  Edge* e = Edge::edge_from_inport(pySelf);
  if (!e) return nullptr;

  PyObject* N /*borrowed*/ = e->get_src();
  Py_XINCREF(N);
  return N;
}

static PyObject* inport_get_oport(PyObject* pySelf,void*) {
  Edge* e = Edge::edge_from_inport(pySelf);
  if (!e) return nullptr;

  return e->get_oport();
}

static PyObject* inport_get_pragmas(PyObject* pySelf,void*) {
  Edge* e = Edge::edge_from_inport(pySelf);
  if (!e) return nullptr;

  PyObject* result = e->pragmas;
  Py_INCREF(result);
  return result;
}

static PyGetSetDef inport_getset[] = {
  {(char*)"node",inport_get_node,nullptr,(char*)"Get the node associated with this port reference"},
  {(char*)"port",inport_get_port,nullptr,(char*)"Get the port number associated with this port reference"},
  {(char*)"literal",inport_get_literal,nullptr,(char*)"Get the literal string associated with this port reference (None for edge)"},
  {(char*)"type",inport_get_type,nullptr,(char*)"Get the type associated with this port reference"},
  {(char*)"src",inport_get_src,nullptr,(char*)"Get the output node associated with this input port reference (None for literal)"},
  {(char*)"oport",inport_get_oport,nullptr,(char*)"Get the output port number associated with this port reference (0 for literal)"},
  {(char*)"pragmas",inport_get_pragmas,nullptr,(char*)"Get the pragmas associated with this port reference"},

  {nullptr}
};

/*
  ###          #  ###                #    
  # #          #  #  #               #    
  # #         ### #  #   ##   ###   ###   
  # #   #  #   #  ###   #  #  #  #   #    
  # #   #  #   #  #     #  #  #      #    
  ###   ####   @  #      ##   #       ##  
*/

static void outport_dealloc(PyObject* pySelf) {
  IF1_OutPortObject* self = reinterpret_cast<IF1_OutPortObject*>(pySelf);

  Py_XDECREF(self->weaksrc);

  Py_TYPE(pySelf)->tp_free(pySelf);
}

static PyObject* outport_str(PyObject* pySelf) {
  IF1_OutPortObject* self = reinterpret_cast<IF1_OutPortObject*>(pySelf);

  PyObject* src /*borrowed*/ = PyWeakref_GET_OBJECT(self->weaksrc);
  if (src == Py_None) return PyErr_Format(PyExc_ValueError,"out port is disconnected");
  PyObject* str /*owned*/ = PyObject_Str(src);
  if (!str) return nullptr;
  
  PyString_Concat(&str,COLON);
  if (!str) return nullptr;

  PyObject* sport /*owned*/ = PyString_FromFormat("%ld",self->port);
  if (!sport) { Py_DECREF(str); return nullptr; }
  PyString_ConcatAndDel(&str,sport);

  return str;
}

static PyObject* outport_int(PyObject* pySelf) {
  IF1_OutPortObject* self = reinterpret_cast<IF1_OutPortObject*>(pySelf);
  return PyInt_FromLong(self->port);
}

static PyObject* outport_getattro(PyObject* pySelf, PyObject* attr) {
  if (PyString_Check(attr) && PyString_GET_SIZE(attr) == 2) {
    PortInfo* info = PortInfo::portinfo_from_outport(pySelf);
    if (info) {
      PyObject* rhs = PyDict_GetItem(info->pragmas(),attr);
      if (rhs) { Py_INCREF(rhs); return rhs; }
      PyErr_Clear();
    }
  }
  return PyObject_GenericGetAttr(pySelf,attr);
}
static int outport_setattro(PyObject* pySelf, PyObject* attr, PyObject* rhs) {
  if (PyString_Check(attr) && PyString_GET_SIZE(attr) == 2) {
    PortInfo* info = PortInfo::portinfo_from_outport(pySelf);
    if (info) {
      if (rhs) {
	return PyDict_SetItem(info->pragmas(),attr,rhs);
      } else {
	int status = PyDict_DelItem(info->pragmas(),attr);
	if (status) {
	  PyErr_Format(PyExc_AttributeError,"no such pragma '%s'",PyString_AS_STRING(attr));
	}
	return status;
      }
    }
  }
  return PyObject_GenericSetAttr(pySelf,attr,rhs);
}

static PyObject* outport_richcompare(PyObject* pySelf, PyObject* pyOther, int op) {
  // types must match
  if (!PyObject_IsInstance(pyOther,(PyObject*)&IF1_OutPortType)) return Py_NotImplemented;

  IF1_OutPortObject* self = reinterpret_cast<IF1_OutPortObject*>(pySelf);
  IF1_OutPortObject* other = reinterpret_cast<IF1_OutPortObject*>(pyOther);

  PyObject* result = nullptr;
  switch(op) {
  case Py_LT:
  case Py_LE:
    return Py_NotImplemented;
  case Py_EQ:
    if (self == other) {
      result = Py_True;
    } else {
      result = (PyWeakref_GET_OBJECT(self->weaksrc) == PyWeakref_GET_OBJECT(self->weaksrc) &&
		self->port == other->port)?Py_True:Py_False;
    }
    break;
  case Py_NE:
    if (self == other) {
      result = Py_False;
    } else {
      result = (PyWeakref_GET_OBJECT(self->weaksrc) == PyWeakref_GET_OBJECT(self->weaksrc) &&
		self->port == other->port)?Py_False:Py_True;
    }
    break;
  case Py_GT:
  case Py_GE:
    return Py_NotImplemented;
  }
  Py_INCREF(result);
  return result;
}

static PyObject* outport_get_node(PyObject* pySelf,void*) {
  IF1_OutPortObject* self = reinterpret_cast<IF1_OutPortObject*>(pySelf);
  PyObject* node /*borrowed*/ = PyWeakref_GET_OBJECT(self->weaksrc);
  if (node == Py_None) return PyErr_Format(PyExc_RuntimeError,"out port is disconnected from node");
  Py_INCREF(node);
  return node;
}
static PyObject* outport_get_port(PyObject* pySelf,void*) {
  IF1_OutPortObject* self = reinterpret_cast<IF1_OutPortObject*>(pySelf);
  PyObject* node /*borrowed*/ = PyWeakref_GET_OBJECT(self->weaksrc);
  if (node == Py_None) return PyErr_Format(PyExc_RuntimeError,"out port is disconnected from node");
  return PyInt_FromLong(self->port);
}
static PyObject* outport_get_type(PyObject* pySelf,void*) {
  PortInfo* info = PortInfo::portinfo_from_outport(pySelf);
  if (!info) return nullptr;
  return info->gettype();
}
static PyObject* outport_get_pragmas(PyObject* pySelf,void*) {
  PortInfo* info = PortInfo::portinfo_from_outport(pySelf);
  if (!info) return nullptr;
  PyObject* result = info->pragmas();
  Py_XINCREF(result);
  return result;
}

static bool edge_match(PyObject* target,long port, PyObject* N, PyObject* result) {
  IF1_NodeObject* node = reinterpret_cast<IF1_NodeObject*>(N);
  for(std::map<long,Edge>::iterator p=node->edges->begin(), end = node->edges->end(); p != end; ++p) {
    if (p->second.oport == port) {
      PyObject* src /*borrowed*/ = PyWeakref_GET_OBJECT(p->second.weaksrc);
      if (src == target) {
	PyObject* in /*owned*/ = PyObject_CallFunction(N,(char*)"l",p->first);
	if (!in) return false;
	if (PyList_Append(result,in) < 0) { Py_DECREF(in); return -1; }
	Py_DECREF(in);
      }
    }
  }
  return true;
}

static PyObject* outport_get_edges(PyObject* pySelf,void*) {
  IF1_OutPortObject* self = reinterpret_cast<IF1_OutPortObject*>(pySelf);

  PyObject* node /*borrowed*/ = PyWeakref_GET_OBJECT(self->weaksrc);
  if (node == Py_None) return PyErr_Format(PyExc_RuntimeError,"out port is disconnected from node");

  PyObject* home /*owned*/ = PyNumber_Positive(node);
  if (!home) return nullptr;

  PyObject* result /*owend*/ = PyList_New(0);
  IF1_GraphObject* G = reinterpret_cast<IF1_GraphObject*>(home);

  // Check the graph itself
  if (!edge_match(node,self->port,home,result)) {
    Py_DECREF(home);
    Py_DECREF(result);
    return nullptr;
  }

  // Check all the top level children
  for(ssize_t i=0; i < PyList_GET_SIZE(G->node.children); ++i) {
    if (!edge_match(node,self->port,PyList_GET_ITEM(G->node.children,i),result)) {
      Py_DECREF(home);
      Py_DECREF(result);
      return nullptr;
    }
  }

  Py_DECREF(home);
  return result;
}

static PyGetSetDef outport_getset[] = {
  {(char*)"node",outport_get_node,nullptr,(char*)"Get the node associated with this port reference"},
  {(char*)"port",outport_get_port,nullptr,(char*)"Get the port number associated with this port reference"},
  {(char*)"type",outport_get_type,nullptr,(char*)"Get the type associated with this port reference"},
  {(char*)"pragmas",outport_get_pragmas,nullptr,(char*)"Get the pragmas associated with this port reference"},
  {(char*)"edges",outport_get_edges,nullptr,(char*)"Get edges attached to this port reference"},
  {nullptr}
};

/*
  #  #           #        
  ## #           #        
  ## #   ##    ###   ##   
  # ##  #  #  #  #  # ##  
  # ##  #  #  #  #  ##    
  #  #   ##    ###   ##  
*/

static int node_init(PyObject* pySelf, PyObject* module, PyObject* parent, long opcode) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);

  if ( PyWeakref_Check(module) ) {
    Py_INCREF(self->weakmodule = module);
  } else {
    self->weakmodule = PyWeakref_NewRef(module,0);
    if (!self->weakmodule) return -1;
  }

  if ( !parent ) {
    self->weakparent = nullptr;
  } else if ( PyWeakref_Check(parent) ) {
    Py_INCREF(self->weakparent = parent);
  } else {
    self->weakparent = PyWeakref_NewRef(parent,0);
    if (!self->weakparent) return -1;
  }

  self->opcode = opcode;

  self->children = PyList_New(0);
  if (!self->children) return -1;

  self->pragmas = PyDict_New();
  if (!self->pragmas) return -1;

  self->edges = new std::map<long,Edge>;
  self->outputs = new std::map<long,PortInfo>;

  return 0;
}

static void node_dealloc(PyObject* pySelf) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);

  if (self->weak) PyObject_ClearWeakRefs(pySelf);

  Py_XDECREF(self->weakmodule);
  Py_XDECREF(self->weakparent);
  Py_XDECREF(self->children);
  Py_XDECREF(self->pragmas);

  delete self->edges;
  delete self->outputs;

  Py_TYPE(pySelf)->tp_free(pySelf);
}

#if 0
}

#endif

static PyObject* node_getattro(PyObject* pySelf, PyObject* attr) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);
  if (PyString_Check(attr) && PyString_GET_SIZE(attr) == 2) {
    PyObject* rhs = PyDict_GetItem(self->pragmas,attr);
    if (rhs) { Py_INCREF(rhs); return rhs; }
    PyErr_Clear();
  }
  return PyObject_GenericGetAttr(pySelf,attr);
}
static int node_setattro(PyObject* pySelf, PyObject* attr, PyObject* rhs) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);
  if (PyString_Check(attr) && PyString_GET_SIZE(attr) == 2) {
    if (rhs) {
      return PyDict_SetItem(self->pragmas,attr,rhs);
    } else {
      int status = PyDict_DelItem(self->pragmas,attr);
      if (status) {
	PyErr_Format(PyExc_AttributeError,"no such pragma '%s'",PyString_AS_STRING(attr));
      }
      return status;
    }
  }
  return PyObject_GenericSetAttr(pySelf,attr,rhs);
}

static PyObject* node_str(PyObject* pySelf) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);

  PyObject* pyOpcode /*owned*/ = PyLong_FromLong(self->opcode);
  if (not pyOpcode) return nullptr;

  PyObject* gstr /*borrowed*/ = PyDict_GetItem(OPCODES,pyOpcode);
  Py_DECREF(pyOpcode);
  Py_XINCREF(gstr);
  if (!gstr) {
    gstr = PyString_FromFormat("<opcode %ld>",self->opcode);
  }
  return gstr;
}

static PyObject* node_inport(PyObject* pySelf, PyObject* args, PyObject* kwargs) {
  PyObject* p /*owned*/ = PyType_GenericNew(&IF1_InPortType,nullptr,nullptr);
  IF1_InPortObject* port = reinterpret_cast<IF1_InPortObject*>(p);

  if (!PyArg_ParseTuple(args,"l",&port->port)) return nullptr;
  if (port->port <= 0) return PyErr_Format(PyExc_ValueError,"port must be > 0 (%ld)",port->port);

  port->weakdst = PyWeakref_NewRef(pySelf,0);
  if (!port->weakdst) { Py_DECREF(p); return nullptr; }
  return p;
}

static PyObject* node_module(PyObject* pySelf) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);
  PyObject* module /*borrowed*/ = PyWeakref_GET_OBJECT(self->weakmodule);
  if (module == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected node");
  return module;
}

static PyObject* node_int(PyObject* pySelf) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);
  long label = 0;
  if (self->weakparent) {
    PyObject* parent /*borrowed*/ = PyWeakref_GET_OBJECT(self->weakparent);
    if ( parent == Py_None ) {
      label = 0;
    } else {
      // Look up this node in the parent
      IF1_NodeObject* P = reinterpret_cast<IF1_NodeObject*>(parent);
      for(ssize_t i = 0; i < PyList_GET_SIZE(P->children); ++i) {
	if (PyList_GET_ITEM(P->children,i) == pySelf) {
	  label = i+1;
	  break;
	}
      }
    }
  }
  return PyInt_FromLong(label);
}

static PyObject* node_positive(PyObject* pySelf) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);
  if (!self->weakparent) { Py_INCREF(Py_None); return Py_None; }
  PyObject* parent /*borrowed*/ = PyWeakref_GET_OBJECT(self->weakparent);
  Py_INCREF(parent);
  return parent;
}

static PyObject* node_inedge_if1(PyObject* pySelf,PyObject** pif1) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);

  PyObject* pylabel = PyNumber_Int(pySelf);
  if (!pylabel) return nullptr;
  long ilabel = PyInt_AsLong(pylabel);
  Py_DECREF(pylabel);
  if (ilabel < 0 && PyErr_Occurred()) return nullptr;

  for(std::map<long,Edge>::iterator ii = self->edges->begin(), end = self->edges->end();
	ii != end; ++ii) {
    PyObject* eif1 = nullptr;
    long port = ii->first;
    Edge& e = ii->second;
    PyObject* auxdict /*borrowed*/ = nullptr;
    if (e.oport) {
      PortInfo* info = e.get_portinfo();
      if (info) auxdict = info->pragmas();
      eif1 = PyString_FromFormat("E %ld %ld %ld %ld %ld",
				 e.get_src_label(),e.oport,
				 ilabel,port,
				 e.get_type_label()
				 );
    } else {
      eif1 = PyString_FromFormat("L     %ld %ld %ld \"%s\"",
				 ilabel,port,
				 e.get_type_label(),
				 e.value.c_str()
				 );
    }
    if (!eif1) { Py_DECREF(*pif1); return nullptr; }

    // Add in the pragmas
    PyObject* prags /*owned*/ = pragmas(e.pragmas,auxdict);
    if (!prags) {
      Py_DECREF(*pif1);
      Py_DECREF(eif1);
      return nullptr;
    }
    PyString_ConcatAndDel(&eif1,prags);
    if (!eif1) { Py_DECREF(*pif1); return nullptr; }
    PyString_Concat(&eif1,NEWLINE);
    if (!eif1) { Py_DECREF(*pif1); return nullptr; }

    PyString_ConcatAndDel(pif1,eif1);
    if (!*pif1) return nullptr;
  }
  return *pif1;
}

static PyObject* node_get_if1(PyObject* pySelf,void*) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);
  
  PyObject* pylabel /*owned*/ = PyNumber_Int(pySelf);
  if (!pylabel) return nullptr;
  long label = PyInt_AsLong(pylabel);
  Py_DECREF(pylabel);
  if (label < 0 && PyErr_Occurred()) return nullptr;

  PyObject* if1 /*owned*/ = PyString_FromFormat("N %ld %ld",label,self->opcode);

  // Add in the pragmas
  PyObject* prags /*owned*/ = pragmas(self->pragmas);
  if (!prags) {
    Py_DECREF(if1);
    return nullptr;
  }
  PyString_ConcatAndDel(&if1,prags);
  if (!if1) return nullptr;
  PyString_Concat(&if1,NEWLINE);
  if (!if1) return nullptr;

  if (!if1) return nullptr;
  return node_inedge_if1(pySelf,&if1);
}

PyObject* node_get_inputs(PyObject* pySelf,void*) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);

  PyObject* result = PyList_New(0);
  for(std::map<long,Edge>::iterator p=self->edges->begin(), end=self->edges->end(); p != end; ++p) {
    PyObject* in /*owned*/ = PyObject_CallFunction(pySelf,(char*)"l",p->first);
    if (!in) { Py_DECREF(result); return nullptr; }
    if (PyList_Append(result,in) != 0) { Py_DECREF(result); return nullptr; }
  }
  return result;
}

PyObject* node_get_outputs(PyObject* pySelf,void*) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);

  PyObject* result = PyList_New(0);
  for(std::map<long,PortInfo>::iterator p=self->outputs->begin(), end=self->outputs->end(); p != end; ++p) {
    PyObject* out /*owned*/ = PySequence_GetItem(pySelf,p->first);
    if (!out) { Py_DECREF(result); return nullptr; }
    if (PyList_Append(result,out) != 0) { Py_DECREF(result); return nullptr; }
  }
  return result;
}

static PyGetSetDef node_getset[] = {
  {(char*)"if1",node_get_if1,nullptr,(char*)"IF1 representation (including edges)",nullptr},
  {(char*)"inputs",node_get_inputs,nullptr,(char*)"all input ports"},
  {(char*)"outputs",node_get_outputs,nullptr,(char*)"all output ports"},
  {nullptr}
};

static ssize_t node_seq_length(PyObject* pySelf) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);
  return self->outputs->size();
}

static PyObject* node_seq_getitem(PyObject* pySelf,ssize_t port) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);

  std::map<long,PortInfo>::iterator p = self->outputs->find(port);
  if (p == self->outputs->end()) {
    return PyErr_Format(PyExc_IndexError,"No such port %ld",port);
  }

  PyObject* info = PyType_GenericNew(&IF1_OutPortType,nullptr,nullptr);
  if (!info) return nullptr;
  IF1_OutPortObject* out = reinterpret_cast<IF1_OutPortObject*>(info);
  out->weaksrc = PyWeakref_NewRef(pySelf,nullptr);
  out->port = port;

  return info;
}

static int node_seq_setitem(PyObject* pySelf,ssize_t port,PyObject* type) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);

  // Are we deleting this port?
  if (!type || type == Py_None) {
    self->outputs->erase(port);
    return 0;
  }

  if (!PyObject_IsInstance(type,(PyObject*)&IF1_TypeType)) {
    PyErr_Format(PyExc_TypeError,"Can only assign an IF1 type to a port, not a %s",type->ob_type->tp_name);
    return -1;
  }

  return self->outputs->operator[](port).settype(type);
}


/*
   ##                     #     
  #  #                    #     
  #     ###    ###  ###   ###   
  # ##  #  #  #  #  #  #  #  #  
  #  #  #     # ##  #  #  #  #  
   ###  #      # #  ###   #  #  
                    #           
 */

static void graph_init(PyObject* pySelf,
		PyObject* module,
		long opcode,
		PyObject* name,
		PyObject* type
		) {
  IF1_GraphObject* self = reinterpret_cast<IF1_GraphObject*>(pySelf);
  node_init(pySelf,module,nullptr,opcode);

  Py_XINCREF( self->name = name );
  Py_XINCREF( self->type = type );
}

static void graph_dealloc(PyObject* pySelf) {
  IF1_GraphObject* self = reinterpret_cast<IF1_GraphObject*>(pySelf);

  Py_XDECREF(self->name);
  Py_XDECREF(self->type);

  node_dealloc(pySelf);
}

static PyObject* graph_int(PyObject* pySelf) {
  return PyInt_FromLong(0);
}

static PyObject* graph_positive(PyObject* pySelf) {
  Py_INCREF(pySelf);
  return pySelf;
}

static PyObject* graph_get_type(PyObject* pySelf,void*) {
  IF1_GraphObject* self = reinterpret_cast<IF1_GraphObject*>(pySelf);
  // Did we set an explicit type?
  if (self->type) {
    Py_INCREF(self->type);
    return self->type;
  }

  // If not a function graph, we return None here
  if (self->node.opcode != IFXGraph && self->node.opcode != IFLGraph) {
    Py_INCREF(Py_None);
    return Py_None;
  }

  // function inputs (graph outputs) are optional
  ssize_t ninputs = self->node.outputs->size();
  if (ninputs && self->node.outputs->rbegin()->first != ninputs) {
    return PyErr_Format(PyExc_ValueError,
			"Function has %ld inputs, but the high port is %ld",ninputs,self->node.outputs->rbegin()->first);
  }
  PyObject* inputs = PyTuple_New(ninputs);
  if (!inputs) return nullptr;
  for(std::map<long,PortInfo>::iterator p=self->node.outputs->begin(), end = self->node.outputs->end();
      p != end; ++p) {
    PyObject* T /*borrowed*/ = p->second.gettype();
    Py_INCREF(T);
    PyTuple_SET_ITEM(inputs,p->first-1,T);
  }

  // Do we have function outputs (graph inputs)?  Are they wired with no gaps?
  ssize_t noutputs = self->node.edges->size();
  if ( noutputs == 0 ) {
    Py_DECREF(inputs);
    Py_INCREF(Py_None);
    return Py_None;
  }
  if (self->node.edges->rbegin()->first != noutputs) {
    Py_DECREF(inputs);
    return PyErr_Format(PyExc_ValueError,
			"Function produces %ld values, but the high port is %ld",noutputs,self->node.edges->rbegin()->first);
  }
  PyObject* outputs = PyTuple_New(self->node.edges->size());
  if (!outputs) { Py_DECREF(inputs); return nullptr; }
  for(std::map<long,Edge>::iterator ii=self->node.edges->begin(), end=self->node.edges->end();
      ii != end; ++ii) {
    PyObject* T /*borrowed*/ = ii->second.get_type();
    if (!T) { Py_DECREF(inputs); Py_DECREF(outputs); return nullptr; }
    PyTuple_SET_ITEM(outputs,ii->first-1,T);
    Py_INCREF(T);
  }

  PyObject* module /*borrowed*/ = node_module(pySelf);
  PyObject* T /*owned*/ = PyObject_CallMethodObjArgs(module,ADDTYPE,FUNCTIONTYPE,inputs,outputs,0);
  if (!T) puts("the add type failed");
  Py_DECREF(inputs); Py_DECREF(outputs);
  if (!T) return nullptr;
  return T;
}

static int graph_set_type(PyObject* pySelf,PyObject* T, void*) {
  IF1_GraphObject* self = reinterpret_cast<IF1_GraphObject*>(pySelf);
  Py_XDECREF(self->type);

  // Delete type?
  if (!T || T == Py_None) {
    self->type = nullptr;
  }

  else if (!PyObject_IsInstance(T,(PyObject*)&IF1_TypeType) ) {
    PyErr_SetString(PyExc_TypeError,"graph type must be an IF1 type");
    return -1;
  }

  else {
    self->type = T;
  }
  return 0;
}


static PyObject* graph_get_if1(PyObject* pySelf,void*)  {
  IF1_GraphObject* self = reinterpret_cast<IF1_GraphObject*>(pySelf);
  long typecode = 0;
  char tag = 'G';
  switch(self->node.opcode) {
  case IFIGraph:
    tag = 'I';
    if (self->type) {
      PyObject* label /*owned*/ = PyNumber_Int(pySelf);
      if (!label) return nullptr;
      typecode = PyInt_AsLong(label);
      if (typecode < 0 && PyErr_Occurred()) return nullptr;
    }
    break;
  case IFXGraph: {
    tag = 'X';
    typecode = 11111;
    PyObject* T /*owned*/ = graph_get_type(pySelf,nullptr);
    if (!T) return nullptr;
    if (T == Py_None) {
      typecode = 0;
    } else {
      PyObject* label /*owned*/ = PyNumber_Int(T);
      if (!label) { Py_DECREF(T); return nullptr; }
      typecode = PyInt_AsLong(label);
      Py_DECREF(label);
      if (typecode < 0 && PyErr_Occurred()) { Py_DECREF(T); return nullptr; }
    }
    Py_DECREF(T);
  } break;
  case IFSGraph:
  case IFLGraph:
  case IFLPGraph:
  case IFRLGraph:
  default:
    tag = 'G';
  }
  
  PyObject* if1 /*owned*/ = PyString_FromFormat("%c %ld",tag,typecode);
  if (!if1) return nullptr;

  if (self->name) {
    PyString_Concat(&if1,SPACE);
    if (!if1) return nullptr;
    PyString_Concat(&if1,DQUOTE);
    if (!if1) return nullptr;
    PyString_Concat(&if1,self->name);
    if (!if1) return nullptr;
    PyString_Concat(&if1,DQUOTE);
    if (!if1) return nullptr;
  }

  // Add in the pragmas
  PyObject* prags /*owned*/ = pragmas(self->node.pragmas);
  if (!prags) {
    Py_DECREF(if1);
    return nullptr;
  }
  PyString_ConcatAndDel(&if1,prags);
  if (!if1) return nullptr;

  PyString_Concat(&if1,NEWLINE);
  if (!if1) return nullptr;

  node_inedge_if1(pySelf,&if1);
  if (!if1) return nullptr;

  // Now, get the if1 for each node
  for(ssize_t i=0;i<PyList_GET_SIZE(self->node.children);++i) {
    PyObject* nif1 = node_get_if1(PyList_GET_ITEM(self->node.children,i),nullptr);
    if (!nif1) {
      Py_DECREF(if1);
      return nullptr;
    }
    PyString_ConcatAndDel(&if1,nif1);
    if (!if1) return nullptr;
  }
  
  return if1;
}

static PyGetSetDef graph_getset[] = {
  {(char*)"if1",graph_get_if1,nullptr,(char*)"IF1 representation (including edges)",nullptr},
  {(char*)"type",graph_get_type,graph_set_type,(char*)"graph type (only useful for functions)",nullptr},
  {nullptr}
};

static PyObject* graph_node(PyObject* pySelf, PyObject* pyOp) {
  IF1_GraphObject* self = reinterpret_cast<IF1_GraphObject*>(pySelf);

  long opcode = PyInt_AsLong(pyOp);
  if (opcode < 0) return PyErr_Format(PyExc_TypeError,"opcode must be convertable to a non-negative integer");

  PyObject* N /*owned*/ = PyType_GenericNew(&IF1_NodeType,nullptr,nullptr);
  if (node_init(N,self->node.weakmodule,pySelf,opcode) < 0) { Py_DECREF(N); return nullptr; }

  if (PyList_Append(self->node.children,N) < 0) {
    Py_DECREF(N);
    return nullptr;
  }

  return N;
}

static PyMethodDef graph_methods[] = {
  {(char*)"addnode",graph_node,METH_O,(char*)"node(opcode) -> node   Add a new node to the node list"},
  {nullptr}
};

/*
  ###                     
   #                      
   #    #  #  ###    ##   
   #    #  #  #  #  # ##  
   #     # #  #  #  ##    
   #      #   ###    ## 
   #     #    #
 */


static void type_dealloc(PyObject* pySelf) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  if (self->weak) PyObject_ClearWeakRefs(pySelf);

  Py_XDECREF(self->weakmodule);
  Py_XDECREF(self->weak1);
  Py_XDECREF(self->weak2);
  Py_XDECREF(self->pragmas);
  Py_TYPE(pySelf)->tp_free(pySelf);
}

static long type_label(PyObject* pySelf) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  PyObject* w = PyWeakref_GET_OBJECT(self->weakmodule);
  if (w == Py_None) { PyErr_SetString(PyExc_RuntimeError,"disconnected type"); return -1; }
  IF1_ModuleObject* m = reinterpret_cast<IF1_ModuleObject*>(w);
  for(long label=0; label < PyList_GET_SIZE(m->types); ++label) {
    if (PyList_GET_ITEM(m->types,label) == pySelf) return label+1;
  }
  PyErr_SetString(PyExc_RuntimeError,"disconnected type");
  return -1;
}

static PyObject* type_int(PyObject* self) {
  long label = type_label(self);
  if (label <= 0) {
    PyErr_SetString(PyExc_TypeError,"disconnected type");
    return nullptr;
  }
  return PyInt_FromLong(label);
}

static PyObject* type_getattro(PyObject* pySelf,PyObject* attr) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  if (PyString_Check(attr) && PyString_GET_SIZE(attr) == 2) {
    PyObject* rhs = PyDict_GetItem(self->pragmas,attr);
    if (rhs) { Py_INCREF(rhs); return rhs; }
    PyErr_Clear();
  }
  return PyObject_GenericGetAttr(pySelf,attr);
}

static int type_setattro(PyObject* pySelf,PyObject* attr,PyObject* rhs) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  if (PyString_Check(attr) && PyString_GET_SIZE(attr) == 2) {
    if (rhs) {
      return PyDict_SetItem(self->pragmas,attr,rhs);
    } else {
      int status = PyDict_DelItem(self->pragmas,attr);
      if (status) {
	PyErr_Format(PyExc_AttributeError,"no such pragma '%s'",PyString_AS_STRING(attr));
      }
      return status;
    }
  }
  return PyObject_GenericSetAttr(pySelf,attr,rhs);
}

static PyObject* type_str(PyObject* pySelf) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);

  static const char* flavor[] = {"array","basic","field","function","multiple","record","stream","tag","tuple","union"};

  // If we named the type, just use the name (unless it is a tuple, field, or tag)
  PyObject* name /*borrowed*/= PyDict_GetItemString(self->pragmas,"na");
  if (name && self->code != IF_Tuple && self->code != IF_Field && self->code != IF_Tag) {
    Py_INCREF(name); 
    return name;
  }

  switch(self->code) {
  case IF_Record:
  case IF_Union: {
    PyObject* basetype /*borrowed*/ = PyWeakref_GET_OBJECT(self->weak1);
    if (basetype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected structure");
    PyObject* base /*owned*/ = PyObject_Str(basetype);
    if (!base) return nullptr;
    PyObject* result = PyString_FromFormat("%s%s",flavor[self->code],PyString_AS_STRING(base));
    Py_DECREF(base);
    return result;
  }

  case IF_Array:
  case IF_Multiple:
  case IF_Stream: {
    PyObject* basetype /*borrowed*/ = PyWeakref_GET_OBJECT(self->weak1);
    if (basetype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected base type");
    PyObject* base /*owned*/ = PyObject_Str(basetype);
    if (!base) return nullptr;
    PyObject* result = PyString_FromFormat("%s[%s]",flavor[self->code],PyString_AS_STRING(base));
    Py_DECREF(base);
    return result;
  }
  case IF_Basic:
    return PyString_FromFormat("Basic(%ld)",self->aux);
  case IF_Field:
  case IF_Tag:
  case IF_Tuple: {
    PyObject* result /*owned*/ = PyString_FromString("");
    if (!result) return nullptr;
    PyString_Concat(&result,LPAREN);
    if (!result) return nullptr;

    for(;self;) {
      PyObject* name /*borrowed*/= PyDict_GetItemString(self->pragmas,"na");
      if (name) {
        PyObject* fname = PyObject_Str(name);
        if (!fname) { Py_DECREF(result); return nullptr; }
        PyString_ConcatAndDel(&result,fname);
        if (!result) return nullptr;
        PyString_Concat(&result,COLON);
        if (!result) return nullptr;
      }

      PyObject* elementtype /*borrowed*/ = PyWeakref_GET_OBJECT(self->weak1);
      if (elementtype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected tuple element type");

      PyObject* element /*owned*/ = PyObject_Str(elementtype);
      if (!element) return nullptr;

      PyString_ConcatAndDel(&result,element);
      if (!result) return nullptr;

      PyObject* next = self->weak2?PyWeakref_GET_OBJECT(self->weak2):nullptr;
      if (next == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected tuple element link");
      self = reinterpret_cast<IF1_TypeObject*>(next);
      if (self) PyString_Concat(&result,COMMA);
      if (!result) return nullptr;
    }
    PyString_Concat(&result,RPAREN);
    return result;
  }
  case IF_Function: {
    PyObject* intype = (self->weak1)?PyWeakref_GET_OBJECT(self->weak1):nullptr;
    if (intype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected function input");

    PyObject* outtype = PyWeakref_GET_OBJECT(self->weak2);
    if (outtype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected function output");

    PyObject* ins /*owned*/ = (intype)?PyObject_Str(intype):PyString_FromString("");
    if (!ins) return nullptr;
    PyObject* outs /*owned*/ = PyObject_Str(outtype);
    if (!outs) { Py_DECREF(ins); return nullptr; }

    PyString_Concat(&ins,ARROW);
    PyString_ConcatAndDel(&ins,outs);
    return ins;
  }
  case IF_Wild: {
    return PyString_FromString("wild");
  }
    
  }
  return PyErr_Format(PyExc_NotImplementedError,"Code %ld",self->code);
}

static PyObject* type_get_if1(PyObject* pySelf,void*) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  long label = type_label(pySelf);
  if (label < 0) return nullptr;

  PyObject* result /*owned*/ = nullptr;
  switch(self->code) {
    // Wild is a special case
  case IF_Wild:
    result = PyString_FromFormat("T %ld %ld",label,self->code);
    if (!result) return nullptr;
    break;

    // Basic uses the aux, not the weak fields
  case IF_Basic:
    result = PyString_FromFormat("T %ld %ld %ld",label,self->code,self->aux);
    if (!result) return nullptr;
    break;

    // One parameter case
  case IF_Array:
  case IF_Multiple:
  case IF_Record:
  case IF_Stream:
  case IF_Union: {
    PyObject* one = PyWeakref_GET_OBJECT(self->weak1);
    if (one == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected type");
    long one_label = type_label(one);
    if ( one_label < 0 ) return nullptr;
    result = PyString_FromFormat("T %ld %ld %ld",label,self->code,one_label);
    if (!result) return nullptr;
    break;
  }

    // Two parameter case for the rest
  default: {
    long one_label = 0;
    if (self->weak1) {
      PyObject* one = PyWeakref_GET_OBJECT(self->weak1);
      if (one == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected type");
      one_label = type_label(one);
      if ( one_label < 0 ) return nullptr;
    }

    long two_label = 0;
    if (self->weak2) {
      PyObject* two = PyWeakref_GET_OBJECT(self->weak2);
      if (two == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected type");
      two_label = type_label(two);
      if ( two_label < 0 ) return nullptr;
    }

    result = PyString_FromFormat("T %ld %ld %ld %ld",label,self->code,one_label,two_label);
    if (!result) return nullptr;
  }
  }

  PyObject* prags = pragmas(self->pragmas);
  if (!prags) { Py_DECREF(result); return nullptr; }
  PyString_ConcatAndDel(&result,prags);

  return result;

}

static PyObject* type_get_aux(PyObject* pySelf,void*) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  if (self->code == IF_Basic) return PyInt_FromLong(self->aux);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* type_get_parameter1(PyObject* pySelf,void*) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
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

static PyObject* type_get_parameter2(PyObject* pySelf,void*) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
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

static PyObject* type_get_label(PyObject* self,void*) {
  return type_int(self);
}

static PyObject* type_get_name(PyObject* pySelf,void*) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  PyObject* name /*borrowed*/ = PyDict_GetItemString(self->pragmas,"na");
  if (!name) name = Py_None;
  Py_INCREF(name);
  return name;
}

static int type_set_name(PyObject* pySelf,PyObject* rhs,void*) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  if (rhs == nullptr || rhs == Py_None) {
    PyDict_DelItemString(self->pragmas,"na");
    PyErr_Clear();
    return 0;
  }
  return PyDict_SetItemString(self->pragmas,"na",rhs);
}


static PyGetSetDef type_getset[] = {
  {(char*)"name",type_get_name,type_set_name,(char*)"name (maps to the %na pragma value)",nullptr},
  {(char*)"aux",type_get_aux,nullptr,(char*)"aux code (basic only)",nullptr},
  {(char*)"parameter1",type_get_parameter1,nullptr,(char*)"parameter1 type (if any)",nullptr},
  {(char*)"parameter2",type_get_parameter2,nullptr,(char*)"parameter2 type (if any)",nullptr},
  {(char*)"if1",type_get_if1,nullptr,(char*)"if1 code",nullptr},
  {(char*)"label",type_get_label,nullptr,(char*)"type label",nullptr},
  {nullptr}
};

/*
                    #                       
   #   #            #           ##          
   ## ##            #            #          
   # # #   ##     ###   #  #     #     ##   
   # # #  #  #   #  #   #  #     #    # ##  
   #   #  #  #   #  #   #  #     #    ##    
   #   #   ##     ###    ###    ###    ###  
*/


static IF1_TypeObject* wild_type(PyObject* module) {
  PyObject* result /*owned*/ = PyType_GenericNew(&IF1_TypeType,nullptr,nullptr);
  if (!result) return nullptr;
  IF1_TypeObject* T = reinterpret_cast<IF1_TypeObject*>(result);
  T->weakmodule = nullptr;
  T->weak1 = nullptr;
  T->weak2 = nullptr;
  T->pragmas = PyDict_New();
  if (!T->pragmas) { Py_DECREF(T); return nullptr; }

  T->weakmodule /*borrowed*/ = PyWeakref_NewRef(module,nullptr);
  if (!T->weakmodule) { Py_DECREF(T); return nullptr;}

  T->code = IF_Wild;

  return T;
}

static PyObject* rawtype(PyObject* module,
                         unsigned long code,
                         unsigned long aux,
                         PyObject* strong1,
                         PyObject* strong2,
                         PyObject* name,
                         const char* cname) {

  PyObject* typelist /*borrowed*/ = reinterpret_cast<IF1_ModuleObject*>(module)->types;

  // We may already have this type (possibly with a different name). In IF1 those are equivalent,
  // so return that
  for(ssize_t i=0;i<PyList_GET_SIZE(typelist);++i) {
    PyObject* p /*borrowed*/ = PyList_GET_ITEM(typelist,i);
    IF1_TypeObject* t = reinterpret_cast<IF1_TypeObject*>(p);
    if (t->code != code) continue;
    if (t->aux != aux) continue;
    PyObject* strong = (t->weak1)?PyWeakref_GET_OBJECT(t->weak1):nullptr;
    if (strong1 != strong) continue;
    strong = (t->weak2)?PyWeakref_GET_OBJECT(t->weak2):nullptr;
    if (strong2 != strong) continue;
    Py_INCREF(p);
    return p;
  }

  IF1_TypeObject* T = wild_type(module);
  if (!T) return nullptr;
  PyObject* result = reinterpret_cast<PyObject*>(T);
  T->code = code;
  T->aux = aux;
  T->weak1 = strong1?PyWeakref_NewRef(strong1,nullptr):nullptr;
  if (strong1 && !T->weak1) { Py_DECREF(T); return nullptr;}
  T->weak2 = strong2?PyWeakref_NewRef(strong2,nullptr):nullptr;
  if (strong2 && !T->weak2) { Py_DECREF(T); return nullptr;}
  if (name) {
    PyDict_SetItemString(T->pragmas,"na",name);
  } else if (cname) {
    name = PyString_FromString(cname);
    if (!name) { Py_DECREF(T); return nullptr;}
    PyDict_SetItemString(T->pragmas,"na",name);
    Py_DECREF(name);
  }

  if (PyList_Append(typelist,result) != 0) { Py_DECREF(result); return nullptr;}

  return result;
}

static PyObject* rawtype(IF1_ModuleObject* module,
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
                 nullptr);
}

static void parse_if1_line(PyObject* line,std::vector<long>& longs,std::vector<std::string>& strings) {
  PyObject* linesplit /*owned*/ = PyObject_CallMethod(line,(char*)"split",(char*)"");
  if (!linesplit) return;
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
}

static void apply_pragmas(PyObject* pragmas,std::vector<std::string>& strings) {
  for(std::vector<std::string>::iterator p=strings.begin(),end=strings.end(); p != end; ++p) {
    std::string& token = *p;
    // Does it look like a pragma? %xx=...?
    if (token.size() < 5 || token[0] != '%' || token[3] != '=') continue;
    PyObject* key /*owned*/ = PyString_FromStringAndSize(token.c_str()+1,2);
    if (!key) continue;
    PyObject* pragma /*owned*/ = PyString_FromString(token.c_str()+4);
    if (!pragma) { Py_DECREF(key); continue; }

    PyObject* ipragma /*owned*/ = PyNumber_Int(pragma);

    if (ipragma) {
      PyDict_SetItem(pragmas,key,ipragma);
      Py_DECREF(ipragma);
    } else {
      PyDict_SetItem(pragmas,key,pragma);
    }
    Py_DECREF(key);
    Py_DECREF(pragma);
  }
}


static int module_read_types(IF1_ModuleObject* self, PyObject* split) {
  // Read through the type lines (start with T) and find the largest type number
  // Pick off the lines we care about at the same time
  ssize_t ntypes = 0;
  std::vector<PyObject*> Tlines;  // All pointers are borrowed
  for(ssize_t i=0;i<PyList_GET_SIZE(split);++i) {
    PyObject* line /*borrowed*/ = PyList_GET_ITEM(split,i);
    std::vector<long> longs;
    std::vector<std::string> strings;
    parse_if1_line(line,longs,strings);
    if (longs.size() > 1 && strings.size() > 0 && strings[0] == "T") {
      Tlines.push_back(line);
      if (longs[0] > ntypes)  ntypes = longs[0];
    }
  }

  // Make an array full of wild types that we will fill in
  PyObject* types = PyList_New(ntypes);
  for(ssize_t i=0;i<ntypes;++i) {
    IF1_TypeObject* TT /*owned*/ = wild_type((PyObject*)self);
    PyObject* T = reinterpret_cast<PyObject*>(TT);
    if (!T) { Py_DECREF(types); return -1; }
    PyList_SET_ITEM(types,i,T);
  }
  
  // Fill in the actual info
  for(std::vector<PyObject*>::iterator p=Tlines.begin(), end=Tlines.end(); p != end; ++p) {
    PyObject* line /*borrowed*/ = *p;
    std::vector<long> longs;
    std::vector<std::string> strings;
    parse_if1_line(line,longs,strings);

    IF1_TypeObject* T /*borrowed*/ = reinterpret_cast<IF1_TypeObject*>(PyList_GET_ITEM(types,longs[0]-1));

    switch(longs[1]) {
    case IF_Wild:
      if (longs.size() != 2) { Py_DECREF(types); PyErr_Format(PyExc_ValueError,"type %ld is malformed",longs[0]); return -1; }
      break;

    case IF_Array:
    case IF_Multiple:
    case IF_Record:
    case IF_Stream:
    case IF_Union:
      if (longs.size() != 3 ) { Py_DECREF(types); PyErr_Format(PyExc_ValueError,"type %ld is malformed",longs[0]); return -1; }
      if ( longs[2] <= 0 || longs[2] > ntypes ) {
	Py_DECREF(types);
	PyErr_Format(PyExc_ValueError,"type %ld references out-of-bounds type %ld",longs[0],longs[2]);
	return -1;
      }

      T->code = longs[1];
      T->weak1 = PyWeakref_NewRef(PyList_GET_ITEM(types,longs[2]-1),nullptr);
      T->weak2 = nullptr;
      break;

    case IF_Basic: 
      if (longs.size() != 3) { Py_DECREF(types); PyErr_Format(PyExc_ValueError,"type %ld is malformed",longs[0]); return -1; }
      T->code = IF_Basic;
      if (longs[2] < IF_Boolean || longs[2] > IF_WildBasic) {
	Py_DECREF(types);
	PyErr_Format(PyExc_ValueError,"basic type %ld is malformed (tag %ld)",longs[0],longs[2]);
	return -1;
      }
      T->aux = longs[2];
      T->weak1 = nullptr;
      T->weak2 = nullptr;
      break;

    case IF_Field:
    case IF_Function:
    case IF_Tag:
    case IF_Tuple: {
      if (longs.size() != 4) { Py_DECREF(types); PyErr_Format(PyExc_ValueError,"type %ld is malformed",longs[0]); return -1; }

      if ( longs[2] < 0 || longs[2] > ntypes ) {
	Py_DECREF(types);
	PyErr_Format(PyExc_ValueError,"type %ld references out-of-bounds type %ld",longs[0],longs[2]);
	return -1;
      }

      if ( longs[3] < 0 || longs[3] > ntypes ) {
	Py_DECREF(types);
	PyErr_Format(PyExc_ValueError,"type %ld references out-of-bounds type %ld",longs[0],longs[3]);
	return -1;
      }

      // Functions can have a null set of inputs (parameter1), but cannot have null outputs (parameter2)
      // The other chain types can have a null link (parameter2), but not a null field (parameter1)
      long critical = (longs[1] == IF_Function)?(3):(2);
      if (longs[critical] == 0) {
	Py_DECREF(types);
	PyErr_Format(PyExc_ValueError,"type %ld references invalid null link",longs[0]);
	return -1;
      }

      T->code = longs[1];
      T->weak1 = longs[2]?PyWeakref_NewRef(PyList_GET_ITEM(types,longs[2]-1),nullptr):nullptr;
      T->weak2 = longs[3]?PyWeakref_NewRef(PyList_GET_ITEM(types,longs[3]-1),nullptr):nullptr;
    } break;

    default:
      PyErr_Format(PyExc_ValueError,"Unknown type format code %ld for type %ld",longs[1],longs[0]);
      Py_DECREF(types);
      return -1;
    }

    // Apply any pragmas
    apply_pragmas(T->pragmas,strings);

  }
  self->types = types;
  return 0;
}

static int module_init(PyObject* pySelf, PyObject* args, PyObject* kwargs) {
  PyObject* source = nullptr;
  static char* keywords[] = {(char*)"source",nullptr};
  if (!PyArg_ParseTupleAndKeywords(args,kwargs,"|O!",keywords,&PyString_Type,&source)) return -1;

  IF1_ModuleObject* self = reinterpret_cast<IF1_ModuleObject*>(pySelf);
  self->types = nullptr;
  self->pragmas = nullptr;

  self->dict = PyDict_New();
  if (!self->dict) return -1;

  self->types = PyList_New(0);
  if (!self->types) return -1;

  self->pragmas = PyDict_New();
  if (!self->pragmas) return -1;

  self->functions = PyList_New(0);
  if (!self->functions) return -1;

  if (source) {
    PyObject* split /*owned*/ = PyObject_CallMethod(source,(char*)"split",(char*)"s","\n");
    if (!split) return -1;
    if (module_read_types(self,split) != 0) { Py_DECREF(split); return -1; }
  } else {
    PyObject* boolean /*owned*/ = rawtype(pySelf,IF_Basic,IF_Boolean,nullptr,nullptr,nullptr,"boolean");
    if (!boolean) return -1;
    Py_DECREF(boolean);

    PyObject* character /*owned*/ = rawtype(pySelf,IF_Basic,IF_Character,nullptr,nullptr,nullptr,"character");
    if (!character) return -1;
    Py_DECREF(character);

    PyObject* doublereal /*owned*/ = rawtype(pySelf,IF_Basic,IF_DoubleReal,nullptr,nullptr,nullptr,"doublereal");
    if (!doublereal) return -1;
    Py_DECREF(doublereal);

    PyObject* integer /*owned*/ = rawtype(pySelf,IF_Basic,IF_Integer,nullptr,nullptr,nullptr,"integer");
    if (!integer) return -1;
    Py_DECREF(integer);

    PyObject* null /*owned*/ = rawtype(pySelf,IF_Basic,IF_Null,nullptr,nullptr,nullptr,"null");
    if (!null) return -1;
    Py_DECREF(null);

    PyObject* real /*owned*/ = rawtype(pySelf,IF_Basic,IF_Real,nullptr,nullptr,nullptr,"real");
    if (!real) return -1;
    Py_DECREF(real);

    PyObject* wildbasic /*owned*/ = rawtype(pySelf,IF_Basic,IF_WildBasic,nullptr,nullptr,nullptr,"wildbasic");
    if (!wildbasic) return -1;
    Py_DECREF(wildbasic);

    PyObject* wild /*owned*/ = rawtype(pySelf,IF_Wild,0,nullptr,nullptr,nullptr,"wild");
    if (!wild) return -1;
    Py_DECREF(wild);

    PyObject* string /*owned*/ = rawtype(pySelf,IF_Array,0,character,nullptr,nullptr,"string");
    if (!string) return -1;
    Py_DECREF(string);
  }

  for(int i=0;i<PyList_GET_SIZE(self->types);++i) {
    PyObject* p /*borrowed*/ = PyList_GET_ITEM(self->types,i);
    IF1_TypeObject* t /*borrowed*/ = reinterpret_cast<IF1_TypeObject*>(p);
    PyObject* name /*borrowed*/ = PyDict_GetItemString(t->pragmas,"na");
    if (name) PyDict_SetItem(self->dict,name,p);
  }
  return 0;
}

static void module_dealloc(PyObject* pySelf) {
  IF1_ModuleObject* self = reinterpret_cast<IF1_ModuleObject*>(pySelf);
  if (self->weak) PyObject_ClearWeakRefs(pySelf);

  Py_XDECREF(self->dict);
  Py_XDECREF(self->types);
  Py_XDECREF(self->pragmas);
  Py_XDECREF(self->functions);
  Py_TYPE(pySelf)->tp_free(pySelf);
}

static PyObject* module_str(PyObject* pySelf) {
  return PyString_FromString("<module>");
}

static PyObject* module_get_if1(PyObject* pySelf,void*) {
  IF1_ModuleObject* self = reinterpret_cast<IF1_ModuleObject*>(pySelf);

  // Do the functions first so that any new types get generated
  PyObject* functions = PyString_FromString("");
  if (!functions) return nullptr;
  for(ssize_t i=0; i<PyList_GET_SIZE(self->functions); ++i) {
    PyObject* if1 = PyObject_GetAttrString(PyList_GET_ITEM(self->functions,i),"if1");
    if (!if1) return nullptr;
    PyString_ConcatAndDel(&functions,if1);
    if (!functions) return nullptr;
  }

  // Add if1 for all the types
  PyObject* result = PyString_FromString("");
  if (!result) return nullptr;
  for(ssize_t i=0;i<PyList_GET_SIZE(self->types);++i) {
    PyObject* if1 /*owned*/ = PyObject_GetAttrString(PyList_GET_ITEM(self->types,i),"if1");
    PyString_ConcatAndDel(&result,if1);
    if (!result) { Py_DECREF(functions); return nullptr; }
    PyString_Concat(&result,NEWLINE);
    if (!result) { Py_DECREF(functions); return nullptr; }
  }

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
    if (!if1) { Py_DECREF(result); return nullptr; }
    PyString_ConcatAndDel(&result,if1);
    if (!result) { Py_DECREF(functions); return nullptr; }
    PyString_Concat(&result,NEWLINE);
    if (!result) { Py_DECREF(functions); return nullptr; }
  }
  if (PyErr_Occurred()) { Py_DECREF(result); Py_DECREF(functions); return nullptr; }

  // Only now do we append the functions
  PyString_Concat(&result,functions);

  return result;
}

static PyObject* module_addtype_array(IF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist) {
  long code;
  PyObject* base = nullptr;
  if (!PyArg_ParseTuple(args,"lO!",&code,&IF1_TypeType,&base)) return nullptr;
  return rawtype(self,code,0,base,nullptr,name);
}

static PyObject* module_addtype_basic(IF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist) {
  long code;
  long basic;
  if (!PyArg_ParseTuple(args,"ll",&code,&basic)) return nullptr;
  if (basic < 0 || basic > IF_WildBasic) return PyErr_Format(PyExc_ValueError,"invalid basic code %ld",basic);
  return rawtype(self,code,basic,nullptr,nullptr,name);
}

template<int IFXXX>
PyObject* module_addtype_chain(IF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist, ssize_t first) {
  PyObject* last /*owned*/ = nullptr;
  for(ssize_t i=PyTuple_GET_SIZE(args)-1; i>=first; --i) {
    PyObject* Tname = namelist?PyList_GetItem(namelist,i-first):nullptr;
    PyErr_Clear();
    PyObject* T /*borrowed*/ = PyTuple_GET_ITEM(args,i);
    if (!PyObject_IsInstance(T,reinterpret_cast<PyObject*>(&IF1_TypeType))) {
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
PyObject* module_addtype_structure(IF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist) {
  PyObject* chain = module_addtype_chain<CHAIN>(self,args,nullptr,namelist,1);
  if (!chain) return nullptr;

  return rawtype(self,STRUCTURE,0,chain,nullptr,name);
}


static PyObject* module_addtype_function(IF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist) {
  // I want this to be of the form: (code,tupletype,tupletype), but I might get
  // ... (code,(T,T,T),(T,T)) or (code,None,(T,T)) or even (code,(T,T,T),T)
  // at least it all looks like (code, ins, outs)
  long code;
  PyObject* ins;
  PyObject* outs;
  if (!PyArg_ParseTuple(args,"lOO",&code,&ins,&outs)) return nullptr;

  // Inputs may be a tupletype, a tuple of types, a normal type, or None
  if (PyObject_IsInstance(ins,(PyObject*)&IF1_TypeType)) {
    IF1_TypeObject* tins = reinterpret_cast<IF1_TypeObject*>(ins);
    if (tins->code == IF_Tuple) {
      Py_INCREF(ins);
    } else {
      // Build a tuple to pass this in
      PyObject* tup = PyTuple_New(1);
      PyTuple_SET_ITEM(tup,0,ins);
      Py_INCREF(ins);
      ins = module_addtype_chain<IF_Tuple>(self,tup,nullptr,namelist,0);
      if (!ins) return nullptr;
      Py_DECREF(tup);
    }
  }
  else if (PyTuple_Check(ins)) {
    // With a tuple, just construct the new IF_Tuple from those
    if (PyTuple_GET_SIZE(ins) == 0) {
      ins = nullptr;
    } else {
      ins = module_addtype_chain<IF_Tuple>(self,ins,nullptr,namelist,0);
      if (!ins) return nullptr;
    }
  }
  else if (ins == Py_None) {
    ins = nullptr;
  }
  else {
    return PyErr_Format(PyExc_TypeError,"function input cannot be %s",ins->ob_type->tp_name);
  }

  // Outputs may be a tupletype, a tuple of types, a normal type, but not None
  if (PyObject_IsInstance(outs,(PyObject*)&IF1_TypeType)) {
    IF1_TypeObject* touts = reinterpret_cast<IF1_TypeObject*>(outs);
    if (touts->code == IF_Tuple) {
      Py_INCREF(outs);
    } else {
      // Build a tuple to pass this in
      PyObject* tup = PyTuple_New(1);
      PyTuple_SET_ITEM(tup,0,outs);
      Py_INCREF(outs);
      outs = module_addtype_chain<IF_Tuple>(self,tup,nullptr,namelist,0);
      if (!outs) return nullptr;
      Py_DECREF(tup);
    }
  }
  else if (PyTuple_Check(outs)) {
    // With a tuple, just construct the new IF_Tuple from those
    if (PyTuple_GET_SIZE(outs) == 0) return PyErr_Format(PyExc_ValueError,"Cannot have an empty output type");
    outs = module_addtype_chain<IF_Tuple>(self,outs,nullptr,namelist,0);
    if (!outs) return nullptr;
  }
  else {
    Py_XDECREF(ins);
    return PyErr_Format(PyExc_TypeError,"function output cannot be %s",outs->ob_type->tp_name);
  }

  PyObject* result = rawtype(self,IF_Function,0,ins,outs,name);
  Py_XDECREF(ins);
  Py_XDECREF(outs);
  return result;
}

static PyObject* module_addtype(PyObject* pySelf,PyObject* args,PyObject* kwargs) {
  IF1_ModuleObject* self = reinterpret_cast<IF1_ModuleObject*>(pySelf);
  PyObject* name /*borrowed*/ = nullptr;
  PyObject* namelist /*owned*/ = nullptr;
  if (kwargs) {
    name = PyDict_GetItemString(kwargs,"name");
    PyDict_DelItemString(kwargs,"name");

    PyObject* names = PyDict_GetItemString(kwargs,"names");
    PyDict_DelItemString(kwargs,"names");
    if (names) {
      PyObject* namesiter /*owned*/ = PyObject_GetIter(names);
      if (!namesiter) return nullptr;
      PyObject* item;
      namelist = PyList_New(0);
      if (!namelist) { Py_DECREF(namesiter); return nullptr; }
      while((PyErr_Clear(), item=PyIter_Next(namesiter))) {
        int a = PyList_Append(namelist,item);
        Py_DECREF(item);
        if(a<0) {
          Py_DECREF(namesiter);
          Py_DECREF(namelist);
          return nullptr;
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
  if ( code < 0 && PyErr_Occurred() ) return nullptr;

  PyObject* result = nullptr;
  switch(code) {
  case IF_Array:
    result = module_addtype_array(self,args,name, namelist);
    break;
  case IF_Basic: 
    result = module_addtype_basic(self,args,name, namelist);
    break;
  case IF_Field: 
    result = module_addtype_chain<IF_Field>(self,args,name, namelist,1);
    break;
  case IF_Function: 
    result = module_addtype_function(self,args,name, namelist);
    break;
  case IF_Multiple: 
    result = module_addtype_array(self,args,name, namelist);
    break;
  case IF_Record: 
    result = module_addtype_structure<IF_Record,IF_Field>(self,args,name, namelist);
    break;
  case IF_Stream: 
    result = module_addtype_array(self,args,name, namelist);
    break;
  case IF_Tag: 
    result = module_addtype_chain<IF_Tag>(self,args,name, namelist,1);
    break;
  case IF_Tuple: 
    result = module_addtype_chain<IF_Tuple>(self,args,name, namelist,1);
    break;
  case IF_Union: 
    result = module_addtype_structure<IF_Union,IF_Tag>(self,args,name, namelist);
    break;
  default:
    result = PyErr_Format(PyExc_ValueError,"Unknown type code %ld",code);
  }

  Py_XDECREF(namelist);
  return result;
}

static PyObject* module_addfunction(PyObject* pySelf,PyObject* args) {
  IF1_ModuleObject* self = reinterpret_cast<IF1_ModuleObject*>(pySelf);

  PyObject* name;
  long opcode = IFXGraph;
  PyObject* type = nullptr;
  if (!PyArg_ParseTuple(args,"O!|lO!",&PyString_Type,&name,&opcode,&IF1_TypeType,&type)) return nullptr;

  PyObject* N /*owned*/ = PyType_GenericNew(&IF1_GraphType,nullptr,nullptr);
  if (!N) return nullptr;
  graph_init(N, pySelf, opcode, name, type);
  PyList_Append(self->functions,N);

  return N;
}

static PyMethodDef module_methods[] = {
  {(char*)"addtype",(PyCFunction)module_addtype,METH_VARARGS|METH_KEYWORDS,(char*)"adds a new type"},
  {(char*)"addfunction",module_addfunction,METH_VARARGS,(char*)"create a new function graph"},
  {nullptr}  /* Sentinel */
};

static PyGetSetDef module_getset[] = {
  {(char*)"if1",module_get_if1,nullptr,(char*)"IF1 code for entire module",nullptr},
  {nullptr}
};

/*
   ###    ####    #    
    #     #      ##    
    #     ###     #    
    #     #       #    
    #     #       #    
   ###    #      ###  
 */

static PyMethodDef IF1_methods[] = {
    {nullptr}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initif1(void) 
{
  FUNCTIONTYPE = PyInt_FromLong(IF_Function);
  if (!FUNCTIONTYPE) return;

  ADDTYPE = PyString_InternFromString("addtype");
  if (!ADDTYPE) return;

  ARROW = PyString_InternFromString("->");
  if (!ARROW) return;

  COLON = PyString_InternFromString(":");
  if (!COLON) return;

  COMMA = PyString_InternFromString(",");
  if (!COMMA) return;

  LPAREN = PyString_InternFromString("(");
  if (!LPAREN) return;

  RPAREN = PyString_InternFromString(")");
  if (!RPAREN) return;

  SPACE = PyString_InternFromString(" ");
  if (!SPACE) return;

  DQUOTE = PyString_InternFromString("\"");
  if (!DQUOTE) return;

  NEWLINE = PyString_InternFromString("\n");
  if (!NEWLINE) return;

  SPACEPERCENT = PyString_InternFromString(" %");
  if (!SPACEPERCENT) return;

  EQUAL = PyString_InternFromString("=");
  if (!EQUAL) return;

  // ----------------------------------------------------------------------
  // Setup
  // ----------------------------------------------------------------------
  PyObject* m = Py_InitModule3("sap.if1", IF1_methods,
                     "Example module that creates an extension type.");
  if (!m) return;

  // ----------------------------------------------------------------------
  // InPort
  // ----------------------------------------------------------------------
  IF1_InPortType.tp_name = "if1.InPort";
  IF1_InPortType.tp_basicsize = sizeof(IF1_InPortObject);
  IF1_InPortType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES;
  IF1_InPortType.tp_doc = inport_doc;
  IF1_InPortType.tp_dealloc = inport_dealloc;
  IF1_InPortType.tp_str = inport_str;
  IF1_InPortType.tp_repr = inport_str;
  IF1_InPortType.tp_richcompare = inport_richcompare;
  IF1_InPortType.tp_getattro = inport_getattro;
  IF1_InPortType.tp_setattro = inport_setattro;
  IF1_InPortType.tp_getset = inport_getset;
  IF1_InPortType.tp_as_number = &inport_number;
  inport_number.nb_lshift = inport_lshift;
  inport_number.nb_int = inport_int;
  if (PyType_Ready(&IF1_InPortType) < 0) return;

  // ----------------------------------------------------------------------
  // OutPort
  // ----------------------------------------------------------------------
  IF1_OutPortType.tp_name = "if1.OutPort";
  IF1_OutPortType.tp_basicsize = sizeof(IF1_OutPortObject);
  IF1_OutPortType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES;
  IF1_OutPortType.tp_doc = outport_doc;
  IF1_OutPortType.tp_dealloc = outport_dealloc;
  IF1_OutPortType.tp_str = outport_str;
  IF1_OutPortType.tp_repr = outport_str;
  IF1_OutPortType.tp_getattro = outport_getattro;
  IF1_OutPortType.tp_setattro = outport_setattro;
  IF1_OutPortType.tp_richcompare = outport_richcompare;
  IF1_OutPortType.tp_getset = outport_getset;
  IF1_OutPortType.tp_as_number = &outport_number;
  outport_number.nb_int = outport_int;
  if (PyType_Ready(&IF1_OutPortType) < 0) return;

  // ----------------------------------------------------------------------
  // Node
  // ----------------------------------------------------------------------
  IF1_NodeType.tp_name = "if1.Node";
  IF1_NodeType.tp_basicsize = sizeof(IF1_NodeObject);
  IF1_NodeType.tp_flags = Py_TPFLAGS_DEFAULT;
  IF1_NodeType.tp_doc = node_doc;
  IF1_NodeType.tp_dealloc = node_dealloc;
  IF1_NodeType.tp_getattro = node_getattro;
  IF1_NodeType.tp_setattro = node_setattro;
  IF1_NodeType.tp_members = node_members;
  IF1_NodeType.tp_getset = node_getset;
  IF1_NodeType.tp_call = node_inport;
  IF1_NodeType.tp_str = node_str;
  IF1_NodeType.tp_repr = node_str;
  IF1_NodeType.tp_weaklistoffset = offsetof(IF1_NodeObject,weak);
  IF1_NodeType.tp_as_sequence = &node_sequence;
  node_sequence.sq_length = node_seq_length;
  node_sequence.sq_item = node_seq_getitem;
  node_sequence.sq_ass_item = node_seq_setitem;

  IF1_NodeType.tp_as_number = &node_number;
  node_number.nb_int = node_int;
  node_number.nb_positive = node_positive;
  if (PyType_Ready(&IF1_NodeType) < 0) return;

  // ----------------------------------------------------------------------
  // Graph
  // ----------------------------------------------------------------------
  IF1_GraphType.tp_name = "if1.Graph";
  IF1_GraphType.tp_basicsize = sizeof(IF1_GraphObject);
  IF1_GraphType.tp_base = &IF1_NodeType;
  IF1_GraphType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  IF1_GraphType.tp_doc = graph_doc;
  IF1_GraphType.tp_dealloc = graph_dealloc;
  IF1_GraphType.tp_members = graph_members;
  IF1_GraphType.tp_methods = graph_methods;
  IF1_GraphType.tp_getset = graph_getset;
  IF1_GraphType.tp_as_number = &graph_number;
  graph_number.nb_int = graph_int;
  graph_number.nb_positive = graph_positive;
  if (PyType_Ready(&IF1_GraphType) < 0) return;

  // ----------------------------------------------------------------------
  // Type
  // ----------------------------------------------------------------------
  IF1_TypeType.tp_name = "if1.Type";
  IF1_TypeType.tp_basicsize = sizeof(IF1_TypeObject);
  IF1_TypeType.tp_flags = Py_TPFLAGS_DEFAULT;
  IF1_TypeType.tp_doc = type_doc;
  IF1_TypeType.tp_weaklistoffset = offsetof(IF1_TypeObject,weak);
  IF1_TypeType.tp_dealloc = type_dealloc;
  IF1_TypeType.tp_str = type_str;
  IF1_TypeType.tp_repr = type_str;
  IF1_TypeType.tp_members = type_members;
  IF1_TypeType.tp_getset = type_getset;
  IF1_TypeType.tp_getattro = type_getattro;
  IF1_TypeType.tp_setattro = type_setattro;
  IF1_TypeType.tp_as_number = &type_number;
  type_number.nb_int = type_int;
  if (PyType_Ready(&IF1_TypeType) < 0) return;
  Py_INCREF(&IF1_TypeType);
  PyModule_AddObject(m, "Type", (PyObject *)&IF1_TypeType);

  // ----------------------------------------------------------------------
  // Module
  // ----------------------------------------------------------------------
  IF1_ModuleType.tp_name = "if1.Module";
  IF1_ModuleType.tp_basicsize = sizeof(IF1_ModuleObject);
  IF1_ModuleType.tp_flags = Py_TPFLAGS_DEFAULT;
  IF1_ModuleType.tp_doc = module_doc;
  IF1_ModuleType.tp_weaklistoffset = offsetof(IF1_ModuleObject,weak);
  IF1_ModuleType.tp_new = PyType_GenericNew;
  IF1_ModuleType.tp_init = module_init;
  IF1_ModuleType.tp_dealloc = module_dealloc;
  IF1_ModuleType.tp_methods = module_methods;
  IF1_ModuleType.tp_str = module_str;
  IF1_ModuleType.tp_repr = module_str;
  IF1_ModuleType.tp_members = module_members;
  IF1_ModuleType.tp_getset = module_getset;
  IF1_ModuleType.tp_getattro = PyObject_GenericGetAttr;
  IF1_ModuleType.tp_dictoffset = offsetof(IF1_ModuleObject,dict);
  if (PyType_Ready(&IF1_ModuleType) < 0) return;
  Py_INCREF(&IF1_ModuleType);
  PyModule_AddObject(m, "Module", (PyObject *)&IF1_ModuleType);

  // ----------------------------------------------------------------------
  // Export macros
  // ----------------------------------------------------------------------
  PyModule_AddIntMacro(m,IF_Array);
  PyModule_AddIntMacro(m,IF_Basic);
  PyModule_AddIntMacro(m,IF_Field);
  PyModule_AddIntMacro(m,IF_Function);
  PyModule_AddIntMacro(m,IF_Multiple);
  PyModule_AddIntMacro(m,IF_Record);
  PyModule_AddIntMacro(m,IF_Stream);
  PyModule_AddIntMacro(m,IF_Tag);
  PyModule_AddIntMacro(m,IF_Tuple);
  PyModule_AddIntMacro(m,IF_Union);
  PyModule_AddIntMacro(m,IF_Wild);
  PyModule_AddIntMacro(m,IF_Boolean);
  PyModule_AddIntMacro(m,IF_Character);
  PyModule_AddIntMacro(m,IF_DoubleReal);
  PyModule_AddIntMacro(m,IF_Integer);
  PyModule_AddIntMacro(m,IF_Null);
  PyModule_AddIntMacro(m,IF_Real);
  PyModule_AddIntMacro(m,IF_WildBasic);
  PyModule_AddIntMacro(m,IFAAddH);
  PyModule_AddIntMacro(m,IFAAddL);
  PyModule_AddIntMacro(m,IFAAdjust);
  PyModule_AddIntMacro(m,IFABuild);
  PyModule_AddIntMacro(m,IFACatenate);
  PyModule_AddIntMacro(m,IFAElement);
  PyModule_AddIntMacro(m,IFAFill);
  PyModule_AddIntMacro(m,IFAGather);
  PyModule_AddIntMacro(m,IFAIsEmpty);
  PyModule_AddIntMacro(m,IFALimH);
  PyModule_AddIntMacro(m,IFALimL);
  PyModule_AddIntMacro(m,IFARemH);
  PyModule_AddIntMacro(m,IFARemL);
  PyModule_AddIntMacro(m,IFAReplace);
  PyModule_AddIntMacro(m,IFAScatter);
  PyModule_AddIntMacro(m,IFASetL);
  PyModule_AddIntMacro(m,IFASize);
  PyModule_AddIntMacro(m,IFAbs);
  PyModule_AddIntMacro(m,IFBindArguments);
  PyModule_AddIntMacro(m,IFBool);
  PyModule_AddIntMacro(m,IFCall);
  PyModule_AddIntMacro(m,IFChar);
  PyModule_AddIntMacro(m,IFDiv);
  PyModule_AddIntMacro(m,IFDouble);
  PyModule_AddIntMacro(m,IFEqual);
  PyModule_AddIntMacro(m,IFExp);
  PyModule_AddIntMacro(m,IFFirstValue);
  PyModule_AddIntMacro(m,IFFinalValue);
  PyModule_AddIntMacro(m,IFFloor);
  PyModule_AddIntMacro(m,IFInt);
  PyModule_AddIntMacro(m,IFIsError);
  PyModule_AddIntMacro(m,IFLess);
  PyModule_AddIntMacro(m,IFLessEqual);
  PyModule_AddIntMacro(m,IFMax);
  PyModule_AddIntMacro(m,IFMin);
  PyModule_AddIntMacro(m,IFMinus);
  PyModule_AddIntMacro(m,IFMod);
  PyModule_AddIntMacro(m,IFNeg);
  PyModule_AddIntMacro(m,IFNoOp);
  PyModule_AddIntMacro(m,IFNot);
  PyModule_AddIntMacro(m,IFNotEqual);
  PyModule_AddIntMacro(m,IFPlus);
  PyModule_AddIntMacro(m,IFRangeGenerate);
  PyModule_AddIntMacro(m,IFRBuild);
  PyModule_AddIntMacro(m,IFRElements);
  PyModule_AddIntMacro(m,IFRReplace);
  PyModule_AddIntMacro(m,IFRedLeft);
  PyModule_AddIntMacro(m,IFRedRight);
  PyModule_AddIntMacro(m,IFRedTree);
  PyModule_AddIntMacro(m,IFReduce);
  PyModule_AddIntMacro(m,IFRestValues);
  PyModule_AddIntMacro(m,IFSingle);
  PyModule_AddIntMacro(m,IFTimes);
  PyModule_AddIntMacro(m,IFTrunc);
  PyModule_AddIntMacro(m,IFPrefixSize);
  PyModule_AddIntMacro(m,IFError);
  PyModule_AddIntMacro(m,IFReplaceMulti);
  PyModule_AddIntMacro(m,IFConvert);
  PyModule_AddIntMacro(m,IFCallForeign);
  PyModule_AddIntMacro(m,IFAElementN);
  PyModule_AddIntMacro(m,IFAElementP);
  PyModule_AddIntMacro(m,IFAElementM);
  PyModule_AddIntMacro(m,IFSGraph);
  PyModule_AddIntMacro(m,IFLGraph);
  PyModule_AddIntMacro(m,IFIGraph);
  PyModule_AddIntMacro(m,IFXGraph);
  PyModule_AddIntMacro(m,IFLPGraph);
  PyModule_AddIntMacro(m,IFRLGraph);


  // We want opcode->name and name->opcode maps
  OPNAMES = PyDict_New();
  PyModule_AddObject(m,"OPNAMES",OPNAMES);
  OPCODES = PyDict_New();
  PyModule_AddObject(m,"OPCODES",OPCODES);
  PyObject* key;
  PyObject* value;
  Py_ssize_t pos = 0;
  PyObject* dict /*borrowed*/ = PyModule_GetDict(m);
  if (!dict) return;
  while(PyDict_Next(dict,&pos,&key,&value)) {
    if (PyString_Check(key) &&
        PyString_AS_STRING(key)[0] == 'I' && PyString_AS_STRING(key)[1] == 'F' &&
        PyString_AS_STRING(key)[2] != '_'
        ) {
      PyDict_SetItem(OPNAMES,key,value);
      PyDict_SetItem(OPCODES,value,key);
    }
  }

  

}
//(insert "\n" (shell-command-to-string "awk '/ IF/{printf \"PyModule_AddIntMacro(m,%s);\\n\", $1}' IFX.h"))
