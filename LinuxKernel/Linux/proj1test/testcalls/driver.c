#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/kernel.h>
#include <sys/syscall.h>

/* (syscall codes)
335  common	create_mbox_421	__x64_sys_create_mbox_421
336  common	remove_mbox_421	__x64_sys_remove_mbox_421
337  common	count_mbox_421		__x64_sys_count_mbox_421
338  common	list_mbox_421		__x64_sys_list_mbox_421
339  common	send_msg_421		__x64_sys_send_msg_421
340  common	recv_msg_421		__x64_sys_recv_msg_421
341	common	peek_msg_421		__x64_sys_peek_msg_421
342  common	count_msg_421		__x64_sys_count_msg_421
343  common	len_msg_421		__x64_sys_len_msg_421
*/

// I have no idea what the __NR_ means but the example given for project 0 has it
#define __NR_create_mbox_421	335
#define __NR_remove_mbox_421	336
#define __NR_count_mbox_421	337
#define __NR_list_mbox_421	338
#define __NR_send_msg_421	339
#define __NR_recv_msg_421	340
#define __NR_peek_msg_421	341
#define __NR_count_msg_421	342
#define __NR_len_msg_421		343

long create_mbox(unsigned long id, int enable_crypt) {
	return syscall(__NR_create_mbox_421, id, enable_crypt);
}

long remove_mbox(unsigned long id) {
	return syscall(__NR_remove_mbox_421, id);
}

long count_mbox(void) {
	return syscall(__NR_count_mbox_421);
}

long list_mbox(unsigned long * mbxes, long k) {
	return syscall(__NR_list_mbox_421, mbxes, k);
}

long send_msg(unsigned long id, unsigned char * msg, long n, unsigned long key) {
	return syscall(__NR_send_msg_421, id, msg, n, key);
}

long recv_msg(unsigned long id, unsigned char * msg, long n, unsigned long key) {
	return syscall(__NR_recv_msg_421, id, msg, n, key);
}

long peek_msg(unsigned long id, unsigned char * msg, long n, unsigned long key) {
	return syscall(__NR_peek_msg_421, id,  msg, n, key);
}

long count_msg(unsigned long id) {
	return syscall(__NR_count_msg_421, id);
}

long len_msg(unsigned long id) {
	return syscall(__NR_len_msg_421, id);
}

/*
 * Implements basic tests of each system call into one file, driver.c
*/
int main(int argc, char *argv[]) {
// test adding mailboxes
	int errcode[4];
	printf("Testing Valid create_mbox Calls\n");
	errcode[0] = create_mbox(1, 0);
	errcode[1] = create_mbox(2, 0);
	errcode[2] = create_mbox(3, 0);
	errcode[3] = create_mbox(4, 1);

	// all codes should be good so far
	for(int i = 0; i < 4; i++) {
		printf("	code return from system call %i: %i\n", i, (int) errcode[i]);
	}
	
// test adding messages
	// define some messages to add
	char * user;
	user = malloc(sizeof(char)*6);
	char * user2;
	user2 = malloc(sizeof(char)*7);
	char * user3;
	user3 = malloc(sizeof(char)*0);
	
	unsigned char hex[6];
	hex[0] = (unsigned int)(0xDE);
	hex[1] = (unsigned int)(0xAD);
	hex[2] = (unsigned int)(0xBE);
	hex[3] = (unsigned int)(0xEF);
	hex[4] = (unsigned int)(0x12);
	hex[5] = (unsigned int)(0x34);

	strncpy(user, hex, 6);

	unsigned char hexa[7];
	hexa[0] = (unsigned int)(0xDE);
	hexa[1] = (unsigned int)(0xAD);
	hexa[2] = (unsigned int)(0xBE);
	hexa[3] = (unsigned int)(0xEF);
	hexa[4] = (unsigned int)(0x12);
	hexa[5] = (unsigned int)(0x34);
	hexa[6] = (unsigned int)(0x56);
	
	strncpy(user, hexa, 7);
	// end message definitions
	
	printf("Testing Valid send_msg Calls\n");

	int errcodes[10];

	errcodes[0] = send_msg(1, user, 6, 0);
	errcodes[1] = send_msg(1, user, 6, 0);
	errcodes[2] = send_msg(1, user, 6, 0);
	errcodes[3] = send_msg(2, user, 6, 0);
	errcodes[4] = send_msg(2, user, 6, 0);
	errcodes[5] = send_msg(2, user, 6, 0);
	errcodes[6] = send_msg(3, user, 6, 0);
	errcodes[7] = send_msg(3, user, 6, 0);
	errcodes[8] = send_msg(3, user, 6, 0);
	errcodes[9] = send_msg(4, user, 6, (unsigned long)(0x1BADC0DE));

	// all codes should be good so far
	for(int i = 0; i < 10; i++) {
		if(errcodes[i] < 0) {
			printf("	code return from system call %i: %ld\n", i, errcodes[i]);
		}
	}

// test getting number of queues and number of messages
	printf("Testing Valid count_mbox and count_msg Calls\n");
	printf("	number of queues: %ld\n", count_mbox()); // should be 4
	printf("	number of messages in %ld: %ld\n", 1, count_msg(1)); // should be 3
	printf("	number of messages in %ld: %ld\n", 4, count_msg(4)); // should be 1


	return 0;	
}
