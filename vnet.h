/*
 * vnet.h
 *
 * A nice wrapper around POSIX sockets for easier usage. Modelled on Go's net package
 *
 * It is REQUIRED to define _POSIX_C_SOURCE to 200809L or greater in order to use this library
 *
 * Author: vktec
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
#ifndef VNET_H
#define VNET_H

#include <stddef.h>

typedef int vnet_conn_t;
typedef int vnet_listener_t;

enum vnet_net {
	_vnet_STREAM = 1<<1,
	_vnet_DGRAM = 1<<2,

	_vnet_IP4 = 1<<4,
	_vnet_IP6 = 1<<6,

	VNET_TCP4 = _vnet_IP4 | _vnet_STREAM,
	VNET_TCP6 = _vnet_IP6 | _vnet_STREAM,

	VNET_UDP4 = _vnet_IP4 | _vnet_DGRAM,
	VNET_UDP6 = _vnet_IP6 | _vnet_DGRAM,

	// TODO: Unix sockets
};

vnet_conn_t vnet_dial(enum vnet_net net, const char *addr);
int vnet_read(vnet_conn_t conn, char *buf, size_t len);
int vnet_write(vnet_conn_t conn, const char *buf, size_t len);
int vnet_close(vnet_conn_t conn); // Works on vnet_listener_t too
// TODO: vnet_localaddr
// TODO: vnet_remoteaddr

vnet_listener_t vnet_listen(enum vnet_net net, const char *addr);
vnet_conn_t vnet_accept(vnet_listener_t listener);
// TODO: vnet_addr

#endif

#ifdef VNET_IMPL
#undef VNET_IMPL

#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int _vnet_domain(enum vnet_net net) {
	if (net & _vnet_IP4) return AF_INET;
	else if (net & _vnet_IP6) return AF_INET6;
	return -1;
}

union _vnet_sockaddr {
	struct sockaddr sa;
	struct sockaddr_in ip4;
	struct sockaddr_in6 ip6;
	struct sockaddr_un un;
};

static int _vnet_type(enum vnet_net net) {
	if (net & _vnet_STREAM) return SOCK_STREAM;
	else if (net & _vnet_DGRAM) return SOCK_DGRAM;
	return -1;
}

vnet_conn_t vnet_dial(enum vnet_net net, const char *addr) {
	// Create socket
	int domain = _vnet_domain(net);
	int type = _vnet_type(net);
	if (domain < 0 || type < 0) return -1;
	vnet_conn_t sock = socket(domain, type, 0);
	if (sock < 0) return -1;

	// Split node:serv
	const char *serv = strrchr(addr, ':');
	if (!serv) return -1;
	size_t node_len = serv - addr;
	serv++;
	char node[node_len+1];
	memcpy(node, addr, node_len);
	node[node_len] = 0;

	// Lookup address
	struct addrinfo hints = {
		.ai_flags = net & _vnet_IP6 ? AI_V4MAPPED : 0,
		.ai_family = domain,
		.ai_socktype = type,
		.ai_protocol = 0,
		0
	};
	struct addrinfo *res;
	if (getaddrinfo(node, serv, &hints, &res)) return -1;

	// Connect
	if (connect(sock, res->ai_addr, res->ai_addrlen)) {
		freeaddrinfo(res);
		return -1;
	}

	freeaddrinfo(res);
	return sock;
}

int vnet_read(vnet_conn_t conn, char *buf, size_t len) {
	return recv(conn, buf, len, 0);
}

int vnet_write(vnet_conn_t conn, const char *buf, size_t len) {
	return send(conn, buf, len, 0);
}

int vnet_close(vnet_conn_t conn) {
	return close(conn);
}

vnet_listener_t vnet_listen(enum vnet_net net, const char *addr) {
	// Create socket
	int domain = _vnet_domain(net);
	int type = _vnet_type(net);
	if (domain < 0 || type < 0) return -1;
	vnet_conn_t sock = socket(domain, type, 0);
	if (sock < 0) return -1;

	// Split node:serv
	const char *serv = strrchr(addr, ':');
	if (!serv) return -1;
	size_t node_len = serv - addr;
	serv++;
	char node[node_len+1];
	memcpy(node, addr, node_len);
	node[node_len] = 0;

	// Lookup address
	struct addrinfo hints = {
		.ai_flags = net & _vnet_IP6 ? AI_V4MAPPED|AI_PASSIVE : AI_PASSIVE,
		.ai_family = domain,
		.ai_socktype = type,
		.ai_protocol = 0,
		0
	};
	struct addrinfo *res;
	if (getaddrinfo(node, serv, &hints, &res)) return -1;

	// Bind
	int ret = bind(sock, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);
	if (ret) return -1;
	if (listen(sock, 8)) return -1;

	return sock;
}

vnet_conn_t vnet_accept(vnet_listener_t listener) {
	return accept(listener, NULL, NULL);
}

#endif
