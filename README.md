# vlib

This is a collection of single-file header libraries with minimal dependencies - some don't even require the standard library. All libraries are compatible with C99 or later.

## Included Headers

- `v2.h` - 2D vector and collision library. Requires libc
- `v2draw.h` - Debug drawing for v2. Requires libc, SDL2 and v2
- `vdlist.h` - An efficient doubly-linked list implementation - does not use recursion. No libc dependency
- `vslist.h` - An efficient singly-linked list implementation - does not use recursion. No libc dependency
- `vstring.h` - A bounded string type with O(1) length operations. No libc dependency
