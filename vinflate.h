/* vinflate.h
 *
 * DEFLATE decompressor
 * Define VENFLATE_IMPL in one translation unit
 *
 * Limitations:
 * - The output buffer must be statically sized. Because of how gzip files
 *   store uncompressed length, this means gzip files storing more than 4GiB
 *   of uncompressed data are not currently supported
 */

/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or distribute
 * this software, either in source code form or as a compiled binary, for any
 * purpose, commercial or non-commercial, and by any means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors of
 * this software dedicate any and all copyright interest in the software to the
 * public domain. We make this dedication for the benefit of the public at
 * large and to the detriment of our heirs and successors. We intend this
 * dedication to be an overt act of relinquishment in perpetuity of all present
 * and future rights to this software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */
#ifndef VENFLATE_H
#define VENFLATE_H

#include <stddef.h>
#include <stdint.h>

// Data types {{{
struct vinf_data {
	const unsigned char *inp;
	size_t inp_len;

	unsigned char *out;
	uint32_t out_crc, out_len;
};

struct vinf_gzip {
	uint16_t id;
	uint8_t cm, flg;
	uint32_t mtime;
	uint8_t xfl, os;

	const char *f_name;
	const char *f_comment;
	uint16_t f_hcrc; // This stores the computed CRC, not the one read from the file

	struct vinf_data data;
};

enum {
	VENF_GZ_TEXT = 1<<0,
	VENF_GZ_HCRC = 1<<1,
	VENF_GZ_EXTRA = 1<<2,
	VENF_GZ_NAME = 1<<3,
	VENF_GZ_COMMENT = 1<<4,
};

enum vinf_error {
	VENF_ERR_SUCCESS, // No error
	VENF_ERR_EOF,
	VENF_ERR_OVERFLOW,
	VENF_ERR_ID_MISMATCH,
	VENF_ERR_CRC_MISMATCH,
	VENF_ERR_LEN_MISMATCH,
	VENF_ERR_TREE_INVALID,
	VENF_ERR_CODE_INVALID,
	VENF_ERR_DIST_INVALID,
	VENF_ERR_TYPE_INVALID,

	VENF_NERR,
};
extern const char *vinf_error_string[VENF_NERR];
// }}}

enum vinf_error vinflate(struct vinf_data data);
enum vinf_error vinf_read_gzip(const unsigned char *data, size_t data_len, struct vinf_gzip *hdr);

// Internal stuff that might be useful externally {{{
struct vinf_bitreader {
	const unsigned char *data; // Stores a pointer to the end of the data buffer
	size_t bit; // Stores a negative bit index into the data buffer
};

void vinf_brinit(struct vinf_bitreader *br, const unsigned char *data, size_t len);
int vinf_readbit(struct vinf_bitreader *br);
int vinf_readval(struct vinf_bitreader *br, int nbits);
const unsigned char *vinf_readbyt(struct vinf_bitreader *br, int nbytes);

uint32_t vinf_crc32(uint32_t crc, unsigned char byt);
// }}}

#endif

#ifdef VENFLATE_IMPL
#undef VENFLATE_IMPL

#if __STDC_VERSION__ >= 201112L
#define _vinf_thread_local _Thread_local
#elif defined(__GNUC__)
#define _vinf_thread_local __thread
#else
#error "Unsupported implemention: vinflate requires either C11 _Thread_local or GCC __thread"
#endif

void vinf_brinit(struct vinf_bitreader *br, const unsigned char *data, size_t len) {
	br->data = data + len;
	br->bit = len * 8;
}

int vinf_readbit(struct vinf_bitreader *br) {
	if (br->bit <= 1) return -1;
	br->bit--;

	size_t ibyt = br->bit / 8 + 1;
	size_t ibit = 7 - br->bit % 8;

	unsigned char byt = br->data[-ibyt];
	return byt>>ibit & 1;
}

int vinf_readval(struct vinf_bitreader *br, int nbits) {
	int val = 0;
	for (int i = 0; i < nbits; i++) {
		int bit = vinf_readbit(br);
		if (bit < 0) return -1;
		val |= bit << i;
	}
	return val;
}

const unsigned char *vinf_readbyt(struct vinf_bitreader *br, int nbytes) {
	br->bit -= br->bit % 8;
	if (br->bit < 8 * nbytes) return NULL;
	const unsigned char *ret = br->data - (br->bit / 8);
	br->bit -= 8 * nbytes;
	return ret;
}

const char *vinf_error_string[VENF_NERR] = {
	"Success",
	"Unexpected end of file",
	"Uncompressed file too large",
	"GZIP ID mismatch",
	"Checksum mismatch",
	"Block LEN/NLEN mismatch",
	"Invalid Huffman tree",
	"Invalid bit sequence",
	"Distance out of range",
	"Invalid block type",
};

// b0 and b1 specify indices into the tree array for the 0 and 1 children
// A leaf node has b0 = b1 = 0 and the symbol value stored in val
struct _vinf_huff {
	uint32_t b0: 11, b1: 11, val: 10;
};
#define _vinf_huffbit(node, bit) ((bit) ? (node).b1 : (node).b0)
#define _vinf_set_huffbit(node, bit, v) ((bit) ? ((node).b1 = (v)) : ((node).b0 = (v)))

enum {
	// Number of symbols in literal or distance alphabet
	_vinf_NSYM = 286,
	// Number of symbols in fixed alphabet
	_vinf_FIXED_NSYM = 288,
	// Number of symbols in huffman-decoding alphabet
	_vinf_HUFF_NSYM = 19,
};

// A full binary tree with n leaf nodes has exactly 2n-1 total nodes
#define _vinf_NNODE(nsym) (2*(nsym) - 1)

// Fills tree (array of length 2*nsym - 1) with the huffman tree described by desc (array of length nsym)
// This algorithm is described by RFC 1951, section 3.2.2
enum {_vinf_NLENGTHS = 16};
static enum vinf_error _vinf_mkhuff(struct _vinf_huff *tree, const uint8_t *desc, unsigned nsym) {
	// Count codes per bit length
	unsigned bl_count[_vinf_NLENGTHS] = {0};
	for (unsigned i = 0; i < nsym; i++) {
		if (desc[i] >= _vinf_NLENGTHS) {
			return VENF_ERR_TREE_INVALID;
		}
		bl_count[desc[i]]++;
	}

	// Compute starting codes for each bit length
	bl_count[0] = 0;
	uint16_t next_code[_vinf_NLENGTHS] = {0};
	for (unsigned i = 1; i < _vinf_NLENGTHS; i++) {
		next_code[i] = 2 * (next_code[i-1] + bl_count[i-1]);
	}

	// Insert into tree
	tree[0] = (struct _vinf_huff){0, 0, ~0};
	unsigned nnode = 1;
	for (unsigned i = 0; i < nsym; i++) {
		unsigned bitlen = desc[i];
		// Skip 0-bit codes
		if (!bitlen) continue;

		unsigned j = 0;
		uint16_t code = next_code[bitlen]++;

		while (bitlen--) {
			_Bool bit = (code >> bitlen) & 1;

			unsigned next = _vinf_huffbit(tree[j], bit);
			if (!next) {
				next = nnode++;
				if (nnode > 2 * nsym - 1) return VENF_ERR_TREE_INVALID;
				tree[next] = (struct _vinf_huff){0, 0, ~0};
				_vinf_set_huffbit(tree[j], bit, next);
			}

			j = next;
		}

		tree[j] = (struct _vinf_huff){0, 0, i};
	}

	return 0;
}

struct _vinf_stream {
	struct vinf_bitreader r;
	unsigned char *wstart; // Pointer to start of destination buffer
	unsigned char *w; // Pointer to end
	size_t wp; // Negative index
	uint32_t crc; // CRC of written data
};

#define _vinf_readu(st, v, nbyte) do { \
		int _n = (nbyte); \
		const unsigned char *buf = vinf_readbyt(&(st)->r, _n); \
		if (!buf) return VENF_ERR_EOF; \
		(v) = 0; \
		while (_n--) { \
			(v) = (v) << 8 | buf[_n]; \
		} \
	} while (0)

#define _vinf_write(st, buffer, buf_length) do { \
		const unsigned char *_buf = (buffer); \
		if (!_buf) return VENF_ERR_EOF; \
		size_t _len = (buf_length); \
		if ((st)->wp < _len) return VENF_ERR_OVERFLOW; \
		while (_len--) { \
			(st)->crc = vinf_crc32((st)->crc, *_buf); \
			(st)->w[-(st)->wp--] = *_buf++; \
		} \
	} while (0)

#define _vinf_read_huff_code(st, huff, v) do { \
		uint16_t _node = 0; \
		do { \
			int _bit = vinf_readbit(&(st)->r); \
			if (_bit < 0) return VENF_ERR_EOF; \
			_node = _vinf_huffbit((huff)[_node], _bit); \
		} while ((huff)[_node].b0 || (huff)[_node].b1); \
		if ((huff)[_node].val >= _vinf_NSYM) return VENF_ERR_CODE_INVALID; \
		(v) = (huff)[_node].val; \
	} while (0)

static enum vinf_error _vinf_read_alphabet(struct _vinf_stream *st, struct _vinf_huff *hc_tree, struct _vinf_huff *tree, int count) {
	uint8_t desc[_vinf_NSYM] = {0};
	for (int i = 0; i < count;) {
		int value, count;
		_vinf_read_huff_code(st, hc_tree, value);

		if (value < 16) {
			count = 1;
		} else if (value == 16) {
			count = vinf_readval(&st->r, 2);
			if (count < 0) return VENF_ERR_EOF;
			count += 3;
			value = desc[i-1];
		} else if (value == 17) {
			count = vinf_readval(&st->r, 3);
			if (count < 0) return VENF_ERR_EOF;
			count += 3;
			value = 0;
		} else if (value == 18) {
			count = vinf_readval(&st->r, 7);
			if (count < 0) return VENF_ERR_EOF;
			count += 11;
			value = 0;
		} else {
			return VENF_ERR_TREE_INVALID;
		}

		while (count--) {
			desc[i++] = value;
		}
	}

	return _vinf_mkhuff(tree, desc, _vinf_NSYM);
}

static enum vinf_error _vinf_block_huff(
	struct _vinf_stream *st,
	const struct _vinf_huff *lit_tree,
	const struct _vinf_huff *dist_tree
) {
	enum {END_OF_BLOCK = 256};
	static const uint8_t len_table[] = {
		0, 0, 0, 0,
		0, 0, 0, 0,
		1, 1, 1, 1,
		2, 2, 2, 2,
		3, 3, 3, 3,
		4, 4, 4, 4,
		5, 5, 5, 5,
		0,
	};

	static const uint8_t dist_table[] = {
		0, 0, 0, 0,
		1, 1, 2, 2,
		3, 3, 4, 4,
		5, 5, 6, 6,
		7, 7, 8, 8,
		9, 9, 10, 10,
		11, 11, 12, 12,
		13, 13,
	};

	uint16_t value;
	unsigned char byt;
	do {
		_vinf_read_huff_code(st, lit_tree, value);
		if (value < END_OF_BLOCK) {
			byt = value;
			_vinf_write(st, &byt, 1);
		} else if (value > END_OF_BLOCK) {
			value -= END_OF_BLOCK + 1;
			if (value >= sizeof len_table / sizeof *len_table) {
				return VENF_ERR_CODE_INVALID;
			}

			// Calculate backref length
			int len = vinf_readval(&st->r, len_table[value]);
			if (len < 0) return VENF_ERR_EOF;
			len += 3;
			for (uint8_t i = 0; i < value; i++) {
				len += 1 << len_table[i];
			}

			// Read distance code
			_vinf_read_huff_code(st, dist_tree, value);
			if (value >= sizeof dist_table / sizeof *dist_table) {
				return VENF_ERR_CODE_INVALID;
			}

			// Calculate backref distance
			int off = vinf_readval(&st->r, dist_table[value]);
			if (off < 0) return VENF_ERR_EOF;
			off += 1;
			for (uint8_t i = 0; i < value; i++) {
				off += 1 << dist_table[i];
			}

			unsigned char *src = st->w - st->wp - off;
			if (src < st->wstart) return VENF_ERR_DIST_INVALID;

			// Copy backref data
			_vinf_write(st, src, len);
		}
	} while (value != END_OF_BLOCK);
	return 0;
}

static enum vinf_error _vinf_block_uncompressed(struct _vinf_stream *st) {
	uint16_t len, nlen;
	_vinf_readu(st, len, 2);
	_vinf_readu(st, nlen, 2);
	nlen = ~nlen;
	if (len != nlen) return VENF_ERR_LEN_MISMATCH;

	_vinf_write(st, vinf_readbyt(&st->r, len), len);
	return 0;
}

static enum vinf_error _vinf_block_fixed(struct _vinf_stream *st) {
	// Specified in RFC 1951, section 3.2.6
	static const uint8_t desc[_vinf_FIXED_NSYM] = {
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8,
	};

	static _vinf_thread_local struct _vinf_huff tree[_vinf_NNODE(_vinf_FIXED_NSYM)] = {0};
	static _vinf_thread_local _Bool inited = 0;
	if (!inited) {
		inited = 1;
		enum vinf_error err = _vinf_mkhuff(tree, desc, _vinf_FIXED_NSYM);
		if (err) return err;
	}

	return _vinf_block_huff(st, tree, tree);
}

static enum vinf_error _vinf_block_dynamic(struct _vinf_stream *st) {
	int hlit = vinf_readval(&st->r, 5);
	if (hlit < 0) return VENF_ERR_EOF;
	hlit += 257;

	int hdist = vinf_readval(&st->r, 5);
	if (hdist < 0) return VENF_ERR_EOF;
	hdist += 1;

	int hclen = vinf_readval(&st->r, 4);
	if (hclen < 0) return VENF_ERR_EOF;
	hclen += 4;
	if (hclen >= _vinf_HUFF_NSYM) return VENF_ERR_TREE_INVALID;

	static const uint8_t hc_order[_vinf_HUFF_NSYM] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
	uint8_t hc_desc[_vinf_HUFF_NSYM] = {0};
	for (int i = 0; i < hclen; i++) {
		int v = vinf_readval(&st->r, 3);
		if (v < 0) return VENF_ERR_EOF;
		hc_desc[hc_order[i]] = v;
	}

	struct _vinf_huff hc_tree[_vinf_NNODE(_vinf_HUFF_NSYM)];
	enum vinf_error err = _vinf_mkhuff(hc_tree, hc_desc, _vinf_HUFF_NSYM);
	if (err) return err;

	struct _vinf_huff lit_tree[_vinf_NNODE(_vinf_NSYM)];
	err = _vinf_read_alphabet(st, hc_tree, lit_tree, hlit);
	if (err) return err;

	struct _vinf_huff dist_tree[_vinf_NNODE(_vinf_NSYM)];
	err = _vinf_read_alphabet(st, hc_tree, dist_tree, hdist);
	if (err) return err;

	return _vinf_block_huff(st, lit_tree, dist_tree);
}

static enum vinf_error _vinf_block_reserved(struct _vinf_stream *st) {
	return VENF_ERR_TYPE_INVALID;
}

enum vinf_error vinflate(struct vinf_data data) {
	// This type is cursed, so here's an explanation, courtesy of cdecl.org:
	//
	//   declare block_types as array of pointer to function (pointer to struct _vinf_stream) returning enum vinf_error
	//
	// That didn't really help, did it
	static enum vinf_error (*block_types[])(struct _vinf_stream *st) = {
		_vinf_block_uncompressed,
		_vinf_block_fixed,
		_vinf_block_dynamic,
		_vinf_block_reserved,
	};

	struct _vinf_stream st = {
		{0},
		data.out,
		data.out + data.out_len,
		data.out_len,
		0,
	};
	vinf_brinit(&st.r, data.inp, data.inp_len);

	int b_final, b_type;
	enum vinf_error err;
	do {
		b_final = vinf_readbit(&st.r);
		if (b_final < 0) return VENF_ERR_EOF;

		b_type = vinf_readval(&st.r, 2);
		if (b_type < 0) return VENF_ERR_EOF;

		err = block_types[b_type](&st);
		if (err) return err;
	} while (!b_final);

	if (st.wp != 0) {
		return VENF_ERR_EOF;
	}

	if (st.crc != data.out_crc) {
		return VENF_ERR_CRC_MISMATCH;
	}

	return 0;
}

enum vinf_error vinf_read_gzip(const unsigned char *data, size_t data_len, struct vinf_gzip *hdr) {
	*hdr = (struct vinf_gzip){0};

	uint32_t crc = 0;

#define _vinf_advance(step, n) do { \
		size_t _n = (n); \
		if (data_len < _n) return VENF_ERR_EOF; \
		for (int _i = 0; _i < _n; _i++) { \
			step; \
			crc = vinf_crc32(crc, *data); \
			data++; \
			data_len--; \
		} \
	} while (0)

#define _vinf_read(v, n) do { (v) = 0; _vinf_advance((v) |= *data << (8*_i), n); } while (0)

	_vinf_read(hdr->id, 2);
	if (hdr->id != 0x8b1f) return VENF_ERR_ID_MISMATCH;

	_vinf_read(hdr->cm, 1);
	_vinf_read(hdr->flg, 1);
	_vinf_read(hdr->mtime, 4);
	_vinf_read(hdr->xfl, 1);
	_vinf_read(hdr->os, 1);

	if (hdr->flg & VENF_GZ_EXTRA) {
		uint16_t xlen;
		_vinf_read(xlen, 2);
		_vinf_advance(, xlen);
	}

	if (hdr->flg & VENF_GZ_NAME) {
		hdr->f_name = (const char *)data;
		unsigned char ch = 1;
		while (ch) _vinf_read(ch, 1);
	}

	if (hdr->flg & VENF_GZ_COMMENT) {
		hdr->f_comment = (const char *)data;
		unsigned char ch = 1;
		while (ch) _vinf_read(ch, 1);
	}

	if (hdr->flg & VENF_GZ_HCRC) {
		hdr->f_hcrc = crc & 0xffff;
		uint16_t file_crc16;
		_vinf_read(file_crc16, 2);
		if (hdr->f_hcrc != file_crc16) return VENF_ERR_CRC_MISMATCH;
	}

#undef _vinf_read
#undef _vinf_advance

	// Remove 8 bytes at end for gzip footer
	if (data_len < 8) return VENF_ERR_EOF;
	data_len -= 8;

	// Store pointer to compressed data
	hdr->data.inp = data;
	hdr->data.inp_len = data_len;

	// Read gzip footer
	data += data_len;
	for (int i = 0; i < 4; i++) hdr->data.out_crc |= *data++ << (8*i);
	for (int i = 0; i < 4; i++) hdr->data.out_len |= *data++ << (8*i);

	return 0;
}

_Bool vinf_verify_gzip(const unsigned char *data, size_t data_len, uint32_t crc, size_t len) {
	if (data_len < 8) return 0;

	uint32_t file_crc = 0, file_len = 0;
	for (int i = 0; i < 4; i++) file_crc |= *data++ << (8*i);
	for (int i = 0; i < 4; i++) file_len |= *data++ << (8*i);

	return file_crc == crc && file_len == (len & ~(uint32_t)0);
}

uint32_t vinf_crc32(uint32_t crc, uint8_t byt) {
	static _vinf_thread_local uint32_t table[256];
	static _vinf_thread_local _Bool table_inited = 0;

	if (!table_inited) {
		for (int i = 0; i < 256; i++) {
			uint32_t c = i;
			for (int j = 0; j < 8; j++) {
				if (c & 1) {
					c /= 2;
					c ^= 0xEDB88320;
				} else {
					c /= 2;
				}
			}
			table[i] = c;
		}
		table_inited = 1;
	}

	crc = ~crc;
	uint8_t idx = crc ^ byt;
	crc >>= 8;
	crc ^= table[idx];
	return ~crc;
}

uint32_t vinf_crc32_buf(unsigned char *buf, size_t len) {
	uint32_t crc = 0;
	while (len--) crc = vinf_crc32(crc, *buf++);
	return crc;
}

#endif
