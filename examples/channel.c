#include <stdatomic.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <threads.h>
#include <unistd.h>
#define VCHANNEL_H
#include "../vchannel.h"

struct thread_info {
	int id;
	struct vch *ch;
};

int send(void *data) {
	thrd_sleep(&(struct timespec){.tv_nsec = rand()}, NULL); // Add some random delay
	struct thread_info *info = data;
	vch_send(info->ch, (void *)(uintptr_t)info->id);
	printf("-> %d\n", info->id);
	return 0;
}

int recv(void *data) {
	thrd_sleep(&(struct timespec){.tv_nsec = rand()}, NULL); // Add some random delay
	struct thread_info *info = data;
	int num = (uintptr_t)vch_recv(info->ch);
	printf("[%d] %d\n", info->id, num);
	return 0;
}

int main() {
	struct vch *ch = vch_new(1);

	// Synchronous usage (not recommended)
	vch_send(ch, "foo");
	puts(vch_recv(ch));
	vch_send(ch, "bar");
	puts(vch_recv(ch));

	// Threaded usage
	int n_senders = 10, n_receivers = 10;
	thrd_t threads[n_senders + n_receivers];
	struct thread_info infos[n_senders + n_receivers];

	for (int i = 0; i < n_senders; i++) {
		infos[i] = (struct thread_info){i, ch};
		thrd_create(&threads[i], send, &infos[i]);
	}

	for (int i = n_senders; i < n_senders + n_receivers; i++) {
		infos[i] = (struct thread_info){i, ch};
		thrd_create(&threads[i], recv, &infos[i]);
	}

	for (int i = 0; i < n_senders + n_receivers; i++) {
		thrd_join(threads[i], NULL);
	}

	vch_del(ch);

	return 0;
}
