#include <ctype.h>
#include <stdio.h>
#define VJSON_IMPL
#include "../vjson.h"

void print_str(const char *s) {
	putchar('"');
	while (*s) {
		if (*s == '\\' || *s == '"') {
			printf("\\%c", *s);
		} else if (!isprint(*s)) {
			printf("\\x%02x", (unsigned char)*s);
		} else {
			putchar(*s);
		}
		++s;
	}
	puts("\"");
}

int main() {
	const char *source =
		"0 -0	123 -123	123.5 -123.5	0.5 -0.5\n"
		"true false	null\n"
		"\"Hello, world!\" \"\\\" \\\\ \\/ \\b \\f \\n \\r \\t \\u00a7\"\n"
		"[] [0, 1, 2] [[0], [0], [1], [\"foo\"], false]\n"
		"{} {\"hello\": \"world\"} {\".\": [1, 2, 3, {}, {\"foo\": \"bar\"}]}\n"
		;

	size_t len = strlen(source);
	const char *end = source + len;

	while (len) {
		const char *start = source;
		switch (vjson_value(&source, &len)) {
		case VJSON_ERROR:
			fprintf(stderr, "Parse error\n");
			return 1;

		case VJSON_EOF:
			puts("EOF");
			break;

		case VJSON_NUMBER:
			printf("NUM:  %Lg\n", vjson_get_number(start));
			break;

		case VJSON_BOOL:
			printf("BOOL: %s\n", vjson_get_bool(start) ? "true" : "false");
			break;

		case VJSON_NULL:
			puts("NULL: null");
			break;

		case VJSON_STRING:;
			printf("STR:  ");
			char *s = vjson_get_string(start);
			print_str(s);
			free(s);
			break;

		case VJSON_ARRAY:
			puts("ARR");
			break;
		case VJSON_OBJECT:
			puts("OBJ");
			break;
		}

		if (len != end - source) printf("Length discrepancy: %zu != %zu\n", len, end - source);
	}

	return 0;
}
