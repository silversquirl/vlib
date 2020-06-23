#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#define VNET_IMPL
#include "../vnet.h"

int server(void) {
	vnet_listener_t l = vnet_listen(VNET_TCP6, "localhost:6969");
	if (l < 0) {
		perror("vnet_listen");
		return 1;
	}

	vnet_conn_t conn = vnet_accept(l);
	if (conn < 0) {
		perror("vnet_accept");
		return 1;
	}

	const char msg[] = "Hello, world!";
	vnet_write(conn, msg, sizeof msg - 1);
	vnet_close(conn);
	vnet_close(l);

	return 0;
}

int client(void) {
	vnet_conn_t conn = vnet_dial(VNET_TCP6, "localhost:6969");
	if (conn < 0) {
		perror("vnet_dial");
		return 1;
	}

	char buf[256];
	int count;
	while ((count = vnet_read(conn, buf, sizeof buf)) > 0) {
		printf("%*s\n", count, buf);
	}

	vnet_close(conn);
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc < 2) return 1;
	if (!strcmp(argv[1], "server")) {
		return server();
	} else if (!strcmp(argv[1], "client")) {
		return client();
	}
}
