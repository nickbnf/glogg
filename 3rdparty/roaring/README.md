# CRoaringUnityBuild
Dumps of CRoaring unity builds (for convenience)

This code is automatically generated from https://github.com/RoaringBitmap/CRoaring

## Building

```bash
 echo -e "#include <roaring.hh>\n int main(){Roaring x;}" > test.cpp && cc -c roaring.c -I. -std=c11 && c++ -o test test.cpp roaring.o -I. -std=c++11
```

You need to compile and link `roaring.c` with your project: this is not a header-only build.

## Usage (C)

```C
#include <stdio.h>
#include "roaring.c"
int main() {
  roaring_bitmap_t *r1 = roaring_bitmap_create();
  for (uint32_t i = 100; i < 1000; i++) roaring_bitmap_add(r1, i);
  printf("cardinality = %d\n", (int) roaring_bitmap_get_cardinality(r1));
  roaring_bitmap_free(r1);
  return 0;
}
```
## Usage (C++)


```C++
#include <iostream>
#include "roaring.hh"
#include "roaring.c"
int main() {
  Roaring r1;
  for (uint32_t i = 100; i < 1000; i++) {
    r1.add(i);
  }
  std::cout << "cardinality = " << r1.cardinality() << std::endl;
  return 0;
}
```
