#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "vtest.h"
#define VUTF8_IMPL
#include "../vutf8.h"

#define TEST_UTF8 "data/vutf8/utf8.txt"
#define TEST_UCS4 "data/vutf8/ucs4.txt"

const struct {uint32_t cp; const char *s;} point_table[] = {
	{'!', u8"!"},
	{'#', u8"#"},
	{'%', u8"%"},
	{'&', u8"&"},
	{'0', u8"0"},
	{'3', u8"3"},
	{'8', u8"8"},
	{'A', u8"A"},
	{'F', u8"F"},
	{'j', u8"j"},
	{'i', u8"i"},
	{'~', u8"~"},

	{0xa1, u8"¡"},
	{0xa2, u8"¢"},
	{0xa3, u8"£"},
	{0xa4, u8"¤"},
	{0xa5, u8"¥"},
	{0xab, u8"«"},
	{0xae, u8"®"},
	{0xc3, u8"Ã"},
	{0xc4, u8"Ä"},
	{0xc5, u8"Å"},
	{0xc6, u8"Æ"},
	{0xc7, u8"Ç"},
	{0xc8, u8"È"},

	{0x300a, u8"《"},
	{0x300b, u8"》"},
	{0x300c, u8"「"},
	{0x300d, u8"」"},
	{0x300e, u8"『"},
	{0x300f, u8"』"},
	{0x3010, u8"【"},
	{0x3011, u8"】"},

	{0x1d000, u8"𝀀"},
	{0x1d001, u8"𝀁"},
	{0x1d002, u8"𝀂"},
	{0x1d003, u8"𝀃"},
	{0x1d004, u8"𝀄"},
	{0x1d005, u8"𝀅"},
	{0x1d006, u8"𝀆"},
};
static char *readf(const char *fn, size_t *len) {
	int fd = open(fn, O_RDONLY);
	if (fd < 0) return NULL;

	struct stat st;
	if (fstat(fd, &st)) {
		close(fd);
		return NULL;
	}

	char *buf = malloc(st.st_size+1);
	ssize_t bufl = read(fd, buf, st.st_size);
	if (bufl < 0) {
		free(buf);
		close(fd);
		return NULL;
	}

	buf[bufl] = 0;
	if (len) *len = bufl;
	return buf;
}

static uint32_t *readf_ucs4(const char *fn, size_t *len) {
	size_t nbyte;
	char *bytes = readf(fn, &nbyte);
	if (!bytes) return NULL;

	size_t npoint = nbyte/4;
	uint32_t *points = calloc(npoint, sizeof *points);
	if (!points) {
		free(bytes);
		return NULL;
	}

	for (size_t i = 0; i < npoint; i++) {
		for (int j = 0; j < 4; j++) {
			points[i] <<= 8;
			points[i] |= bytes[4*i + j] & 0xff;
		}
	}

	free(bytes);
	if (len) *len = npoint;
	return points;
}

VTEST(test_len) {
	vassert_eq(1, vutf8_len(0));
	vassert_eq(1, vutf8_len(127));
	vassert_eq(2, vutf8_len(0x80));
	vassert_eq(2, vutf8_len(0x7ff));
	vassert_eq(3, vutf8_len(0x800));
	vassert_eq(3, vutf8_len(0xFFFF));
	vassert_eq(4, vutf8_len(0x10000));
	vassert_eq(4, vutf8_len(0x10FFFF));
	vassert_eq(0, vutf8_len(0x110000));
	vassert_eq(0, vutf8_len(0x11FFFF));

	for (int i = 0; i < sizeof point_table / sizeof *point_table; i++) {
		vassert_eq(strlen(point_table[i].s), vutf8_len(point_table[i].cp));
	}
}

VTEST(test_write_codepoint) {
	char buf[5];
	for (size_t i = 0; i < sizeof point_table / sizeof *point_table; i++) {
		memset(buf, 0, 5);
		size_t len = strlen(point_table[i].s);
		vassert_eq_p(vutf8_write(buf, point_table[i].cp), buf + len);
		vassert_eq_s(buf, point_table[i].s);
	}
}

VTEST(test_write_testfile) {
	size_t npoint;
	uint32_t *points = readf_ucs4(TEST_UCS4, &npoint);
	if (!vassert_not_null(points)) return;

	char *written_data = malloc(4*npoint), *p = written_data;
	if (!vassert_not_null(written_data)) return;
	for (size_t i = 0; i < npoint; i++) {
		p = vutf8_write(p, points[i]);
		vassert(p < written_data + 4*npoint);
	}
	*p = 0;
	free(points);

	char *test_data = readf(TEST_UTF8, NULL);
	if (!vassert_not_null(test_data)) return;

	// Using this rather than vassert_eq_s because these strings are kiiiiinda long
	vassert(!strcmp(written_data, test_data));

	free(test_data);
	free(written_data);
}

VTEST(test_next_codepoint) {
	for (size_t i = 0; i < sizeof point_table / sizeof *point_table; i++) {
		uint32_t point;
		vassertn(*vutf8_next(point_table[i].s, &point));
		vassert_msg(point_table[i].cp == point, "At index %zu: expected U+%.04"PRIX32", got U+%.04"PRIX32, i, point_table[i].cp, point);
	}
}

VTEST(test_next_testfile) {
	size_t npoint;
	char *bytes = readf(TEST_UTF8, NULL), *p = bytes;
	uint32_t *points = readf_ucs4(TEST_UCS4, &npoint);

	for (size_t i = 0; i < npoint; i++) {
		if (!vassert(*p)) break;

		uint32_t point;
		p = vutf8_next(p, &point);
		if (!vassert_msg(point == points[i], "Codepoint %zu at byte %zu incorrect: expected U+%.04"PRIX32", got U+%.04"PRIX32, i, p - bytes, points[i], point)) break;
	}

	free(points);
	free(bytes);
}

VTEST(test_prev_codepoint) {
	for (size_t i = 0; i < sizeof point_table / sizeof *point_table; i++) {
		uint32_t point;
		size_t len = strlen(point_table[i].s);
		vassert_eq_p(vutf8_prev(point_table[i].s + len, NULL, &point), point_table[i].s);
		vassert_msg(point_table[i].cp == point, "At index %zu: expected U+%.04"PRIX32", got U+%.04"PRIX32, i, point_table[i].cp, point);
	}
}

VTEST(test_prev_testfile) {
	size_t nbyte, npoint;
	char *bytes = readf(TEST_UTF8, &nbyte), *p = bytes + nbyte;
	uint32_t *points = readf_ucs4(TEST_UCS4, &npoint);

	// Test bound checking
	vassert_eq_p(vutf8_prev(bytes, bytes, NULL), bytes);

	do {
		npoint--;
		uint32_t point;
		p = vutf8_prev(p, bytes, &point);
		if (!vassert(p >= bytes)) break;
		if (!vassert_msg(point == points[npoint], "Codepoint %zu at byte %zu incorrect: expected U+%.04"PRIX32", got U+%.04"PRIX32, npoint, p - bytes, points[npoint], point)) break;
	} while (npoint);

	free(points);
	free(bytes);
}

VTESTS_BEGIN
	test_len,
	test_write_codepoint,
	test_write_testfile,
	test_next_codepoint,
	test_next_testfile,
	test_prev_codepoint,
	test_prev_testfile,
VTESTS_END
