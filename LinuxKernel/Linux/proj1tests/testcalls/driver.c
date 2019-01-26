#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
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

/*
 * from errno.h, values of things I may have returned at any point, and why in parenthesis
*/

/*         ENAME         = value    OFFICIAL NAME (what I used it for) 
const long EPERM 		= -1; 	 OPERATION NOT PERMITTED
const long ENOENT 		= -2;	 NO SUCH FILE OR DIRECTORY (no such message queue) 
const long ENOMEM 		= -12;	 OUT OF MEMORY (allocation failed) 
const long EFAULT 		= -14;	 BAD ADDRESS (null pointer) 
const long EEXIST 		= -17;	 FILE EXISTS (queue already exists) 
const long EINVAL 		= -22;	 INVALID ARGUMENT (negative parameters) 
const long ENOTEMPTY	= -39;	 DIRECTORY NOT EMPTY (queue not empty) 
const long ENOMSG		= -42;	 NO MESSAGE OF DESIRED TYPE (empty queue) */

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
	long errcode[4];
	printf("Testing Valid create_mbox Calls\n");
	errcode[0] = create_mbox(1, 0);
	printf("%i\n", errno);
	errcode[1] = create_mbox(2, 0);
	errcode[2] = create_mbox(3, 0);
	errcode[3] = create_mbox(4, 1);

	// all codes should be good so far
	for(int i = 0; i < 4; i++) {
		printf("	code return from system call %i: %ld\n", i+1, errcode[i]);
	}
	
// test adding messages
	// define some messages to add
	unsigned char * user;
	user = malloc(sizeof(unsigned char)*6);
	unsigned char * user2;
	user2 = malloc(sizeof(unsigned char)*7);
	unsigned char * user3;
	user3 = malloc(sizeof(unsigned char)*0);
	unsigned char * user4;
	user4 = NULL; // malicious null pointer
	unsigned char * user5;
	user5 = malloc(sizeof(unsigned char)*10);
	unsigned char * user6;
	user6 = malloc(sizeof(unsigned char)*64); // bigger than anything else instantiated
	
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
	
	strncpy(user2, hexa, 7);

	strncpy(user3, hex, 6);
	// end message definitions
	
	printf("Testing Valid send_msg Calls\n");

	long errcodes[10];

	errcodes[0] = send_msg(1, user3, 6, 0);
	errcodes[1] = send_msg(1, user3, 4, 0);
	errcodes[2] = send_msg(1, user3, 12, 0);
	errcodes[3] = send_msg(2, user, 6, 0);
	errcodes[4] = send_msg(2, user, 6, 0);
	errcodes[5] = send_msg(2, user, 6, 0);
	errcodes[6] = send_msg(3, user, 6, 0);
	errcodes[7] = send_msg(3, user, 6, 0);
	errcodes[8] = send_msg(3, user, 6, 0);
	errcodes[9] = send_msg(4, user, 6, (unsigned long)(0x1BADC0DE));

	// all codes should be good so far
	for(int i = 0; i < 10; i++) {
		printf("	code return from system call %i: %ld\n", i+1, errcodes[i]);
	}

	printf("Testing Semi-Valid send_msg Call\n");

	errcode[0] = send_msg(3, user2, 6, 0); // should be passed but truncated
	errcode[1] = send_msg(3, user2, 8, 0); // passed but padded
	errcode[2] = send_msg(4, user3, 0, 0); // should be passed

	printf("	code return from system call: %ld\n", errcode[0]);
	printf("	code return from system call: %ld\n", errcode[1]);
	printf("	code return from system call: %ld\n", errcode[2]);

	printf("Testing Malicious Invalid send_msg Call\n");
	
	errcode[0] = send_msg(3, user4, 6, 0);
	errcode[1] = send_msg(3, user2, 20, 0);
	printf("	code return from system call: %ld\n", errcode[0]); // should be -1
	printf("	code return from system call: %ld\n", errcode[1]); // should be 0
	

// test getting number of queues and number of messages
	printf("Testing Valid count_mbox and count_msg Calls\n");
	printf("	number of queues: %ld\n", count_mbox()); // should be 4
	printf("	number of messages in %ld: %ld\n", 1, count_msg(1)); // should be 3
	printf("	number of messages in %ld: %ld\n", 4, count_msg(4)); // should be 2

// test getting length of the next message to be removed
	printf("Testing Valid len_msg Calls\n");
	printf("	len of next message in queue %ld: %ld\n", 1, len_msg(1)); // should be 20
	printf("	len of next message in queue %ld: %ld\n", 4, len_msg(4)); // should be 0
	printf("	len of next message in queue %ld: %ld\n", 3, len_msg(3)); // should be 12

// test emptying all queues with recv_msg
	long code = recv_msg(1, user6, 64, 0);
 	printf("Exhausting all queues\n");
	for(unsigned long i = 1; i < 5; i++) {
		printf("queue %lu\n", i);
		do {
			code = recv_msg(i, user6, 64, 0);
			if(code >= 0) {
				printf("	%ld\n", code);
			}
		} while (code >= 0);
	}

// test removing all queues with remove_mbox
	printf("Testing underlying structure with remove_mbox\n");
	remove_mbox(1);
	remove_mbox(2);
	remove_mbox(3);
	remove_mbox(4);
	printf("%ld queues left after removal\n", count_mbox()); // should be 0

// test readding and deleting again
	create_mbox(1, 0);
	create_mbox(2, 0);
	create_mbox(3, 0);
	create_mbox(4, 0);
	printf("%ld queues left after readding\n", count_mbox()); // should be 4
// test list_mbox
	printf("Testing list_mbox\n");
	unsigned long * user7;
	int copied;
	user7 = (unsigned long*) malloc(sizeof(unsigned long) * 10);
	copied = list_mbox(user7, 10);
	for(int i = 0; i < copied; i++) {
		printf("%lu	", user7[i]); // should be 1-4
	}
	printf("\n");

// remove the mboxes so driver has the same values every time (expected, anyway)
	remove_mbox(1);
	remove_mbox(2);
	remove_mbox(3);
	remove_mbox(4);
	printf("%ld queues left after removal\n", count_mbox()); // should be 0
	
	free(user);
	free(user2);
	free(user3);
	free(user5);
	free(user6);
	free(user7);

	return 0;
	
}
