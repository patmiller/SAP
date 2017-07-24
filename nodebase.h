#ifndef SAP_NODEBASE
#define SAP_NODEBASE

#include "IFX.h"

class module;
class inport;
class graph;

class nodebase {
public:
  long opcode;
  std::weak_ptr<nodebase> weakparent;
  PyOwned children;
  PyOwned pragmas;
  std::map<long,std::shared_ptr<inport>> inputs;
  std::map<long,std::shared_ptr<int>> outputs;
  
  nodebase(long opcode=-1,std::shared_ptr<nodebase> parent=nullptr);
  virtual ~nodebase() {}

  PyObject* string();

  virtual PyObject* lookup();
  virtual std::shared_ptr<graph> my_graph();

  static std::map<long,std::string> opcode_to_name;
  static std::map<std::string,long> name_to_opcode;
};

#endif
