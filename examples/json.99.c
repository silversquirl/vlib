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

void parse_and_print(enum vjson_type (*parse)(const char **src, const char *end), const char **src, const char *end, int indent) {
	for (int i = 0; i < indent; i++) {
		printf("  ");
	}

	const char *start = *src;
	switch (parse(src, end)) {
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
		printf("ARRAY: %zu items\n", vjson_get_size(start));

		start = vjson_enter(start);
		while (*start != ']') {
			parse_and_print(vjson_item, &start, *src, indent+1);
		}

		break;

	case VJSON_OBJECT:
		printf("OBJECT: %zu items\n", vjson_get_size(start));

		start = vjson_enter(start);
		while (*start != '}') {
			parse_and_print(vjson_key, &start, *src, indent+1);
			parse_and_print(vjson_item, &start, *src, indent+1);
		}

		break;
	}
}

int main() {
	const char *source =
		"0 -0	123 -123	123.5 -123.5	0.5 -0.5\n"
		"true false	null\n"
		"\"Hello, world!\" \"\\\" \\\\ \\/ \\b \\f \\n \\r \\t \\u00a7\"\n"
		"[] [0, 1, 2] [[0], [0, 2], [1], [\"foo\"], false]\n"
		"{} {\"hello\": \"world\"} {\".\": [1, 2, 3, {}, {\"foo\": \"bar\"}]} [{\"1\": 1, \"2\": 2}]\n"
		;

	const char *end = source + strlen(source);

	while (source < end) {
		parse_and_print(vjson_value, &source, end, 0);
	}

	return 0;
}
