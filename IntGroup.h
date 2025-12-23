#ifndef INTGROUPH
#define INTGROUPH

#include <vector>

#include "Int.h"

class IntGroup {
 public:
  IntGroup(int size);
  ~IntGroup();
  void Set(Int *pts);
  void ModInv();

 private:
  Int *ints;
  Int *subp;
  int size;
};

#endif  // INTGROUPCPUH
