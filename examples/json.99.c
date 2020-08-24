#include <ctype.h>
#include <stdio.h>
#include "../v.h"
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

void parse_and_print(enum vjson_type (*parse)(const char **src, size_t *len), const char **src, size_t *len, int indent) {
	for (int i = 0; i < indent; i++) {
		printf("  ");
	}

	const char *start = *src;
	switch (parse(src, len)) {
	case VJSON_ERROR:
		panic("Parse error");

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
		puts("ARRAY:");

		start = vjson_enter(start);
		size_t alen = *src - start;
		while (*start != ']') {
			parse_and_print(vjson_item, &start, &alen, indent+1);
		}

		break;

	case VJSON_OBJECT:
		puts("OBJECT:");

		start = vjson_enter(start);
		size_t olen = *src - start;
		while (*start != '}') {
			parse_and_print(vjson_key, &start, &olen, indent+1);
			parse_and_print(vjson_item, &start, &olen, indent+1);
		}

		break;
	}
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

	while (len) {
		parse_and_print(vjson_value, &source, &len, 0);
	}

	return 0;
}
