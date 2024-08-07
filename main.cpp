#include "skiplist.h"
#include <iostream>
int main() {
  SkipList<int> sl;
  sl.insert(3);
  sl.insert(123);
  std::cout << sl.contains(4) << ' ' << sl.contains(123) << std::endl;
  ;
}