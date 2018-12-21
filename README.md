# vlib

A collection of simple C datastructures that I find myself reusing a lot.
They're written as self-contained single-header files that can be dropped into any
project, including ones that don't use the standard library.

Custom allocation functions can be provided, and any required libc functions have
an alternate implementaion supplied in the header, as well as the possibility to be
user-defined through macros.

## Included Headers

- `vdlist.h` - An efficient doubly-linked list implementation - does not use recursion
- `vslist.h` - An efficient singly-linked list implementation - does not use recursion
- `vstring.h` - A bounded string type with O(1) length operations

