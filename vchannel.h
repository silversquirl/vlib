/*
 * vchannel.h
 *
 * A simple multi-producer multi-consumer concurrent queue implementation modeled after Go's channels. 
 *
 * Author: vktec
 *
 * Some functions will call abort() in the case of errors. You may alter this behaviour by defining VCH_PANIC to a function of your choosing
 *
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
#ifndef VCHANNEL_H
#define VCHANNEL_H

struct vch;
struct vch *vch_new(size_t buffer);
void vch_del(struct vch *ch);
void vch_send(struct vch *ch, void *item);
void *vch_recv(struct vch *ch);

#endif

#ifdef VCHANNEL_IMPL
#undef VCHANNEL_IMPL

#include <stdatomic.h>
#include <stddef.h>
#include <threads.h>

#ifndef VCH_PANIC
#define VCH_PANIC abort
#endif

// Thread error handling
#define _vch_te(expr) do { if ((expr) != thrd_success) goto err; } while (0)
// Thread error "handling"
#define _vch_tp(expr) do { if ((expr) != thrd_success) VCH_PANIC(); } while (0)

struct vch {
	cnd_t full, empty;
	mtx_t lock;
	size_t start, end, len;
	void *buf[];
};

struct vch *vch_new(size_t buffer) {
	struct vch *ch = malloc(offsetof(struct vch, buf) + (buffer * sizeof (void *)));

	_vch_te(cnd_init(&ch->full));
	_vch_te(cnd_init(&ch->empty));
	_vch_te(mtx_init(&ch->lock, mtx_plain));
	ch->start = ch->end = 0;
	ch->len = buffer;

	return ch;

err:
	free(ch);
	return NULL;
}

void vch_del(struct vch *ch) {
	cnd_destroy(&ch->full);
	cnd_destroy(&ch->empty);
	mtx_destroy(&ch->lock);
	free(ch);
}

void vch_send(struct vch *ch, void *item) {
	_vch_tp(mtx_lock(&ch->lock));

	if (ch->end >= ch->len) {
		while (ch->start == 0) {
			_vch_tp(cnd_wait(&ch->full, &ch->lock));
		}
		ch->end = 0;
		if (ch->start >= ch->len) ch->start = 0;
	}

	ch->buf[ch->end] = item;
	ch->end++;

	_vch_tp(cnd_signal(&ch->empty));
	_vch_tp(mtx_unlock(&ch->lock));

err:
	return;
}

void *vch_recv(struct vch *ch) {
	_vch_tp(mtx_lock(&ch->lock));

	while (ch->start == ch->end) {
		_vch_tp(cnd_wait(&ch->empty, &ch->lock));
	}
	if (ch->start >= ch->end) ch->start = 0;

	void *v = ch->buf[ch->start];
	ch->start++;

	_vch_tp(cnd_signal(&ch->full));
	_vch_tp(mtx_unlock(&ch->lock));
	return v;
}

#endif
