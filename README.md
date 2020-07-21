# vlib

This is a collection of single-file header libraries with minimal dependencies - some don't even require the standard library. Many libraries are compatible with C99, some require C11 or later.

## Included Headers

- `v2.h` - 2D vector and collision library. Requires libc
- `v2draw.h` - Debug drawing for v2. Requires libc, SDL2 and v2
- `varena.h` - Simple arena-based allocator
- `vchannel.h` - Multi-producer, multi-consumer, thread-safe queue
- `vdict.h` - An ordered dictionary for any type inspired by Python's `dict`
- `vdlist.h` - An efficient doubly-linked list implementation - does not use recursion. No libc dependency
- `vgl.h` - OpenGL/GLFW helper library, depends on `vmath.h`
- `vmath.h` - Math helper library
- `vnet.h` - Go-style dial/listen sockets
- `vslist.h` - An efficient singly-linked list implementation - does not use recursion. No libc dependency
- `vstring.h` - A bounded string type with O(1) length operations. No libc dependency
