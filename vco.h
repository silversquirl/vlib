/* vco.h
 *
 * Coroutines for C. Requires amd64 architecture and some GNU extensions
 *
 * Macros:
 *  - VCO_IMPL - define in one translation unit to define the implementation
 *  - VCO_STACK_SIZE - specifies the stack size for each coroutine, in KiB [default: 64]
 */

/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */
#ifndef VCO_H
#define VCO_H

#include <stddef.h>

// Stack size in KiB (multiples of 1024 bytes)
#ifndef VCO_STACK_SIZE
#define VCO_STACK_SIZE (64)
#endif

struct vco;
typedef int (*vco_fn_typ)(int arg, void *data);

// Create a new coroutine which will call fn, passing in data, when first switched to
struct vco *vco_new(vco_fn_typ fn, void *data);

// Delete a coroutine
void vco_del(struct vco *f);

// Push the current coroutine to the call stack, then switch to f
// Calling a finished coroutine is not permitted
int vco_call(struct vco *f, int arg);

// Pop a coroutine from the call stack, then switch to it
// Yielding from a root coroutine is not permitted
int vco_yield(int arg);

// Switch to f
// Switching from a root coroutine is not permitted
// Switching to a finished coroutine is not permitted
int vco_switch(struct vco *f, int arg);

#endif

#ifdef VCO_IMPL

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#ifndef __amd64__
#error "Unsupported platform. vco.h requires amd64 architecture"
#endif

#ifndef __GNUC__
#error "Unsupported platform. vco.h requires GNU extensions"
#endif

struct vco {
	void **stack;
};
enum {VCO_STACK_COUNT = VCO_STACK_SIZE * 1024 / sizeof *(struct vco){0}.stack};

#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#define _vco_thrlocal _Thread_local
#elif defined(__GNUC__)
#define _vco_thrlocal __thread
#else
#error "Unsupported platform. vco.h requires C11 threads or GCC __thread"
#endif

// Current coroutine
_vco_thrlocal struct vco *_vco_me = NULL;
// Caller coroutine
_vco_thrlocal struct vco _vco_caller = {NULL};

struct vco *vco_me(void) {
	return _vco_me;
}

__attribute__((__naked__, __noreturn__)) static void _vco_entry(void) {
	__asm__ (
		// Call entry function
		"mov %rax, %rdi\n\t"
		"mov %r12, %rsi\n\t"
		"call *%rbx\n\t"

		// Yield with return value
		"mov %rax, %rdi\n\t"
		"call vco_yield\n\t"

		// A finished coroutine has been resumed, panic
		"call abort"
	);
}

struct vco *vco_new(vco_fn_typ fn, void *data) {
	void **stack = malloc(VCO_STACK_COUNT * sizeof *stack);
	if (!stack) return NULL;

	// Move to start of stack (grows downwards)
	stack += VCO_STACK_COUNT;

	struct vco *f = (struct vco *)(--stack);

	// Initialize stack
	*--stack = (void *)_vco_entry; // rip
	*--stack = (void *)fn; // rbx
	*--stack = NULL; // rbp
	*--stack = data; // r12
	stack -= 3; // r13, r14, r15
	*--stack = f; // "me"

	f->stack = stack;
	return f;
}

void vco_del(struct vco *f) {
	// Get stack top
	void **stack = (void **)f + 1;
	// Move to start of allocated buffer
	stack -= VCO_STACK_COUNT;
	// Free memory
	free(stack);
}

// For calling from asm, because thread local storage is annoying
__attribute__((__used__)) static struct vco *_vco_getme(void) {
	return _vco_me;
}
__attribute__((__used__)) static void _vco_setme(struct vco *me) {
	_vco_me = me;
}

static __attribute__((__naked__)) int _vco_switch(struct vco *c, struct vco *f, int arg) {
	__asm__ (
		// Push registers
		"push %rbx\n\t"
		"push %rbp\n\t"
		"push %r12\n\t"
		"push %r13\n\t"
		"push %r14\n\t"
		"push %r15\n\t"

		// Push me
		"call _vco_getme\n\t"
		"push %rax\n\t"

		// Switch stacks
		"mov %rsp, (%rdi)\n\t"
		"mov (%rsi), %rsp\n\t"

		// Pop me
		"pop %rdi\n\t"
		"call _vco_setme\n\t"

		// Pop registers
		"pop %r15\n\t"
		"pop %r14\n\t"
		"pop %r13\n\t"
		"pop %r12\n\t"
		"pop %rbp\n\t"
		"pop %rbx\n\t"

		// Return
		"mov %rdx, %rax\n\t"
		"ret"
	);
}

int vco_call(struct vco *f, int arg) {
	struct vco caller = _vco_caller;
	int ret = _vco_switch(&_vco_caller, f, arg);
	_vco_caller = caller;
	return ret;
}

int vco_yield(int arg) {
	return _vco_switch(_vco_me, &_vco_caller, arg);
}

int vco_switch(struct vco *f, int arg) {
	return _vco_switch(_vco_me, f, arg);
}

#endif
