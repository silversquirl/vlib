#include <stdio.h>
#define VENFLATE_IMPL
#include "../v.h"
#include "../vinflate.h"

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s FILE.gz\n", argv[0]);
		return 1;
	}

	size_t len;
	unsigned char *gz = mapfile(argv[1], &len);
	if (!gz) panic("Opening file failed");

	struct vinf_gzip hdr;
	enum vinf_error err = vinf_read_gzip(gz, len, &hdr);
	if (err) panic("GZIP header corrupt: %s", vinf_error_string[err]);

	fprintf(stderr, "Name:\t%s\nLength:\t%d\nCRC:\t%08x\n\n", hdr.f_name, hdr.data.out_len, hdr.data.out_crc);

	hdr.data.out = malloc(hdr.data.out_len);
	err = vinflate(hdr.data);
	if (err) panic("Compressed data corrupt: %s", vinf_error_string[err]);

	fwrite(hdr.data.out, 1, hdr.data.out_len, stdout);
	fflush(stdout);
	fputs("\u00a7\n\n", stderr);

	free(hdr.data.out);

	return 0;
}
