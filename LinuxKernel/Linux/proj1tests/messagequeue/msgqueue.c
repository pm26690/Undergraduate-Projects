#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * queue nodes, each message queue contains messages, more on that in the structure below. uniqueId is the
 * id assigned to the queue at creation, isEncrypted is whether or not the queue was designated for encryption
 * at creation. next is a pointer to the next queue down (if such a queue exists).
*/
typedef struct queue {
	unsigned long uniqueId;
	unsigned int isEncrypted;
	// dummy header doesn't need to be printed or included in anything	
	unsigned int isHead;
	long numMsg;
	struct queue *next;
	struct message *firstMsg;
} queue;

/*
 * messages in a queue, these nodes constitute the messages in any given queue
*/
typedef struct message {
	unsigned char *data;
	long size;
	struct message *next;
} message;

// dummy header
static queue * head;

// prototype of writableCopy
long writableCopy(unsigned char * dest, unsigned char * src, long n);

// prototype of xorEncDec
long xorEncDec(unsigned char * src, long n, unsigned long key);

// prototype of isQueue
long isQueue(unsigned long id);

/*
 * adds a message queue to the list, if no queue existed before initialize the header and go from there.
 * if queues did exist, make sure the target queue does not have the same id as any other queue.
*/
long addQueue(unsigned long id, int crypt) {
	// create a dummy head if no message queue exists, the dummy head will not contain an ID or any messages
	if(head == NULL) {
		head = malloc(sizeof(queue));
		head->isHead = 1;
		head->next = NULL;
		head->firstMsg = NULL;
	}
	
	// check if a queue at the target value already exists, there should be no duplicate queues, so this
	// would return a placeholder negative value for an error (dont have specific errors yet)
	long exists = isQueue(id);
	if(exists < 0) {
		printf("CAN NOT ADD QUEUE IF SUCH A QUEUE EXISTS WITH THIS ID\n");
		return -1; // placeholder for an actual error value
	}

	// either this is the first message queue added, or other queues already exist, if other queues
	// exist make this queue the new first message queue and push the others down (order doesn't appear
	// to matter in the spec so this should be ok)
	if(head->next == NULL) {
		// first queue in the list
		printf("Adding first queue %u\n", id);
		queue *newQueue = malloc(sizeof(queue));
		head->next = newQueue;
		newQueue->uniqueId = id;
		newQueue->isEncrypted = crypt;
		newQueue->isHead = 0;
		newQueue->next = NULL;
		newQueue->firstMsg = NULL;
		newQueue->numMsg = 0;
		return 0;
	} else {
		// not first queue, strap this new one on top and push the others down
		printf("Adding non-first queue %u\n", id);
		queue *newQueue = malloc(sizeof(queue));
		queue *oldQueue = head->next;			
		head->next = newQueue;
		newQueue->next = oldQueue;
		// define other data as normal
		newQueue->uniqueId = id;
		newQueue->isEncrypted = crypt;
		newQueue->isHead = 0;
		newQueue->firstMsg = NULL;
		newQueue->numMsg = 0;
		return 0;
	}
}

/*
 * removes the target queue from the list of queues if and only if it is empty
*/
long removeQueue(unsigned long id){
	queue *nextQueue;
	queue *targetQueue;
	queue *predQueue;
	nextQueue = NULL;
	predQueue = NULL;
	targetQueue = NULL;

	if(head == NULL) {
		return -1; // there are no queues, queue does not exist
	} else {
		nextQueue = head->next;
		// check if there are any queues whatsoever other than the dummy node
		if(nextQueue != NULL) {
			// check if the targeted queue exist by linear traversal of the queue list
			while(nextQueue != NULL && targetQueue == NULL) {
				if(nextQueue->uniqueId == id) {
					printf("	trying to remove queue: %lu\n", id);
					targetQueue = nextQueue;
				}
				
				if(targetQueue == NULL) {
					printf("	predQueue: %lu\n", nextQueue->uniqueId);
					predQueue = nextQueue;
					nextQueue = nextQueue->next;
				}
			}
		} else {
			// there are no message queues at all, stop search now and return appropriate error
			return -1;
		}
	}

	if(targetQueue != NULL) {
		if(targetQueue->numMsg == 0) {
			if(predQueue != NULL) {
				// first queue was not the target queue
				if(targetQueue->next == NULL) {
					// target queue was the last queue, don't need to relink anything				
					free(targetQueue);
					predQueue->next = NULL;
				} else {
					// target queue was somewhere in between head and the last queue, must relink
					predQueue->next = targetQueue->next;
					targetQueue->next = NULL;
					free(targetQueue);
				}
			} else {
				// first queue was the target queue, perhaps it was the only queue
				if(targetQueue->next == NULL) {
					head->next = NULL;
				} else {
					head->next = targetQueue->next;
					targetQueue->next = NULL;
				}
				free(targetQueue);
			}
		} else {
			return -1; // cant get rid of non empty queue
		}
	} else {
		return -1; // queue does not exist
	}
}

/*
 * check if target queue exists, and add a message to it with the given encryption key
*/
long addMessage(unsigned long id, unsigned char * msg, long n, unsigned long key) {
	queue *targetQueue;
	queue *nextQueue;

	targetQueue = NULL;
	nextQueue = NULL;

	nextQueue = head->next;
	// check if there are any queues whatsoever other than the dummy node
	if(nextQueue != NULL) {
		// check if the targeted queue exist by linear traversal of the queue list
		while(nextQueue != NULL) {
			if(nextQueue->uniqueId == id) {
				targetQueue = nextQueue;
			}
			nextQueue = nextQueue->next;
		}
	} else {
		// there are no message queues at all, stop search now and return appropriate error
		return -1;
	}
	
	// if targeted queue does not exist, return appropriate error, else add the message to it
	if(targetQueue == NULL) {
		return -1;
	} else {
		// either the targeted queue has no messages, or it does and we will make this message the
		// first message and push the others down. this keeps our FIFO queue structure intact.
		if(targetQueue->firstMsg == NULL) {
			// first message in the queue
			message *newMsg = malloc(sizeof(message));
			targetQueue->firstMsg = newMsg;
			newMsg->data = malloc(sizeof(unsigned char) * n);
			newMsg->size = n;
			// encrypt message if necessary, dont want to edit given memory
			if(targetQueue->isEncrypted != 0) {
				writableCopy(newMsg->data, msg, n);
				xorEncDec(newMsg->data, newMsg->size, key);
			} else {
				writableCopy(newMsg->data, msg, n);
			}
			// increment numMsg
			targetQueue->numMsg = targetQueue->numMsg + 1;
			return 0;
		} else {
			// not first message in queue, strap this one on top and push the others down
			message *newMsg = malloc(sizeof(message));
			message *oldMsg = targetQueue->firstMsg;
			targetQueue->firstMsg = newMsg;
			newMsg->data = malloc(sizeof(unsigned char) * n);
			newMsg->size = n;
			newMsg->next = oldMsg;
			// encrypt message if necessary, dont want to edit given memory
			if(targetQueue->isEncrypted != 0) {
				writableCopy(newMsg->data, msg, n);
				xorEncDec(newMsg->data, newMsg->size, key);
			} else {
				writableCopy(newMsg->data, msg, n);
			}
			// increment numMsg
			targetQueue->numMsg = targetQueue->numMsg + 1;
			return 0;
		}
	}
}

/*
 * removes a message from the target queue and returns it to the dest char pointers at msg
*/
long removeMessage(unsigned long id, unsigned char *msg, long n, unsigned long key) {
	queue *targetQueue;
	queue *nextQueue;
	message *targetMessage;
	unsigned char *msgToCopy;

	targetQueue = NULL;
	nextQueue = NULL;
	targetMessage = NULL;

	nextQueue = head->next;
	// check if there are any queues whatsoever other than the dummy node
	if(nextQueue != NULL) {
		// check if the targeted queue exist by linear traversal of the queue list
		while(nextQueue != NULL) {
			if(nextQueue->uniqueId == id) {
				targetQueue = nextQueue;
				// the target queue exists, either it has no messages, or does
				if(targetQueue->firstMsg != NULL) {
					// the target queue has at least one message
					if(targetQueue->firstMsg->next == NULL) {
						// the target queue has a single message

						// dont want to edit the message directly in case of errors, so copy it
						msgToCopy = malloc(sizeof(unsigned char)*targetQueue->firstMsg->size);
						memcpy(msgToCopy, targetQueue->firstMsg->data, targetQueue->firstMsg->size);

						if(targetQueue->isEncrypted != 0) {
							xorEncDec(msgToCopy, targetQueue->firstMsg->size, key);
							// in the kernel this would be copy_to_user
							memcpy(msg, msgToCopy, n);
						} else {
							memcpy(msg, msgToCopy, n);
						}

						targetMessage = targetQueue->firstMsg; // keep for later
				
						// free dynamically allocated memory
						free(targetQueue->firstMsg->data);
						free(targetQueue->firstMsg);
						free(msgToCopy);
						targetQueue->numMsg = 0;
						targetQueue->firstMsg = NULL;
						return 0;

						/*
						// free dynamically allocated memory
						free(targetMessage->data);
						free(targetMessage);
						free(msgToCopy);
						targetQueue->numMsg = 0;
						targetQueue->firstMsg = NULL;
						return 0;
						*/
					} else {
						// the target queue has multiple messages

						// dont want to edit the message directly in case of errors, so copy it
						msgToCopy = malloc(sizeof(unsigned char)*targetQueue->firstMsg->size);
						memcpy(msgToCopy, targetQueue->firstMsg->data, targetQueue->firstMsg->size);

						if(targetQueue->isEncrypted != 0) {
							xorEncDec(msgToCopy, targetQueue->firstMsg->size, key);
							// in the kernel this would be copy_to_user
							memcpy(msg, msgToCopy, n);
						} else {
							memcpy(msg, msgToCopy, n);
						}
				
						// link first message to the next messages
						targetMessage = targetQueue->firstMsg; // keep this for later
						targetQueue->firstMsg = targetQueue->firstMsg->next;

						// free dynamically allocated memory
						free(targetMessage->data);
						free(targetMessage);
						free(msgToCopy);
						targetQueue->numMsg = targetQueue->numMsg - 1;
						return 0;
					}
				} else {
					return -1; // target queue has no messages
				}
			}
			nextQueue = nextQueue->next;
		}
		return -1;
	} else {
		// there are no message queues at all, stop search now and return appropriate error
		return -1;
	}
}

/*
 * creates a copy of the source string to the destination string dest for the given writable size n, does not
 * modify the source string src, does not need a null character.
*/
long writableCopy(unsigned char * dest, unsigned char * src, long n) {
	// assumes validation has already been done to the string, just copy.
	for(int i = 0; i < n; i++) {
		unsigned int j;
		j = (unsigned int) src[i];
		dest[i] = (unsigned char)j;
	}
	return 0;
}

/*
 * Similar to code used in addMessage, checks if the queue at target id exists. Different in that
 * the return value is 0 when there is no such queue and negative when there is.
*/
long isQueue(unsigned long id) {
	queue *nextQueue;
	queue *targetQueue;
	nextQueue = NULL;
	targetQueue = NULL;

	nextQueue = head->next;
	// check if there are any queues whatsoever other than the dummy node
	if(nextQueue != NULL) {
		// check if the targeted queue exist by linear traversal of the queue list
		while(nextQueue != NULL) {
			if(nextQueue->uniqueId == id) {
				targetQueue = nextQueue;
			}
			nextQueue = nextQueue->next;
		}
	} else {
		// there are no message queues at all, stop search now and return appropriate value
		return 0;
	}

	// if the targeted queue exists, return a negative value, if it does not return 0.
	if(targetQueue != NULL) {
		return -1;
	} else {
		return 0;
	}
}

/*
 * Does a linear traversal of the message queues and returns the total number of queues
*/
long numQueues() {
	queue * nextQueue;
	nextQueue = NULL;
	long total = 0;

	// unless this function is called right away, this should never happen
	if(head == NULL) {
		return 0; // no head, no queues
	} else if (head->next == NULL) {
		return 0; // again, no queue after head, no queues
	} else {
		nextQueue = head->next;
		while(nextQueue != NULL) {
			total = total + 1;
			nextQueue = nextQueue->next;
		}
		return total;
	}
}

/*
 * Find the target queue and retrieve the number of messages in that queue
*/
long numMessages(unsigned long id) {
	queue *targetQueue;
	queue *nextQueue;
	targetQueue = NULL;
	nextQueue = NULL;

	if(head == NULL) {
		return -1; // there are no queues, queue does not exist
	} else {
		nextQueue = head->next;
		// check if there are any queues whatsoever other than the dummy node
		if(nextQueue != NULL) {
			// check if the targeted queue exist by linear traversal of the queue list
			while(nextQueue != NULL) {
				if(nextQueue->uniqueId == id) {
					targetQueue = nextQueue;
				}
				nextQueue = nextQueue->next;
			}
		} else {
			// there are no message queues at all, stop search now and return appropriate error
			return -1;
		}
	}

	if(targetQueue != NULL) {
		return targetQueue->numMsg;
	} else {
		return -1; // queue does not exist
	}
}

long lenMessage(unsigned long id) {
	queue *targetQueue;
	queue *nextQueue;
	targetQueue = NULL;
	nextQueue = NULL;

	if(head == NULL) {
		printf("lenMessage: there are no queues\n");
		return -1; // there are no queues, queue does not exist
	} else {
		nextQueue = head->next;
		// check if there are any queues whatsoever other than the dummy node
		while(nextQueue != NULL) {
			if(nextQueue->uniqueId == id) {
				targetQueue = nextQueue;
				if(targetQueue->firstMsg != NULL) {
					printf("lenMessage: target queue found and has messages\n");
					return targetQueue->firstMsg->size;
				} else {
					printf("lenMessage: target queue has no messages\n");
					return -1;
				}
			}
			nextQueue = nextQueue->next;
		}
		printf("lenMessage: target queue does not exist\n");
		return -1; // target queue does not exist
	}
}

/*
 * returns a list of up to k mailbox Ids
*/
long listQueue(unsigned long * mbxes, long k) {
	long numQueue;
	unsigned long *queueIds;
	long i;
	queue *nextQueue;

	numQueue = numQueues();
	nextQueue = NULL;
	queueIds = NULL;

	if(mbxes == NULL) {
		return -1;
	}

	if(k < 0) {
		return -1;
	}

	queueIds = (unsigned long*) malloc(sizeof(unsigned long) * numQueue);

	if(head != NULL) {
		if(head->next != NULL) {
			nextQueue = head->next;
			i = 0;
			while(nextQueue != NULL) {
				queueIds[i] = nextQueue->uniqueId;
				printf("queueIds[%ld] = %lu\n", i, nextQueue->uniqueId);
				printf("Actual queueIds[%ld] = %lu\n", i, queueIds[i]);
				i = i + 1;
				nextQueue = nextQueue->next;
			}
			// only useful for kernel
			if(k < numQueue) {
				// kernel would copy_to_user
				memcpy(mbxes, queueIds, sizeof(unsigned long) * k);
				free(queueIds);
				return k;
			} else {
				memcpy(mbxes, queueIds, sizeof(unsigned long) * numQueue);
				free(queueIds);
				return numQueue;
			}
		} else {
			return -1; // no queues
		}
	} else {
		return -1; // no queues
	}
}

/*
 * Dumps a (possibly) human readable message m from a message queue, these can be just binary data so there is
 * no actual gaurantee that this is a string of actual characters and not just unknown characters converted
 * to random symbols. Strictly for testing purposes.
*/
void dumpMessage(unsigned char * m, long n) {
	printf("	");
	unsigned long j;	
	if (n > 0) {
		// print individual "characters" seperated by commas
		if(n > 1) {
			for(int i = 0; i < n-1; i++) {
				j = (unsigned long) m[i];
				printf("%u,", j);
			}
			j = (unsigned long) m[n-1];
			printf("%u\n", j);
		} else {
			j = (unsigned long) m[0];
			printf("%u\n", j);
		}
	} else {
		// empty message, print a new line
		printf("\n");	
	}
}

/*
 * Dumps a human readable visualization of a message queue, strictly for testing purposes
*/
void dumpQueue() {
	queue *nextQueue;
	message *nextMessage;
	nextQueue = NULL;
	nextMessage = NULL;

	// check that any message queues exist
	if(head->next != NULL) {
		// dump all of them
		nextQueue = head->next;		
		while(nextQueue != NULL) {
			printf("%u:\n", nextQueue->uniqueId);
			nextMessage = nextQueue->firstMsg;
			while(nextMessage != NULL) {
				dumpMessage(nextMessage->data, nextMessage->size);
				nextMessage = nextMessage->next;
			}
			nextQueue = nextQueue->next;
		}
	} else {
		printf("No message queue exists\n");
	}
}

/*
 * Frees all allocated memory by the creation of the message queue, used to test for dangling pointers
 * (if all of my pointers are passed correctly, the program should have no memory leaks).
*/
void freeQueue() {
	// check that any message queues exist
	if(head->next != NULL) {
		printf("Freeing the queue (given the code is implemented correctly)\n");
		queue *temp;
		// temp is the next queue marked for deletion
		temp = head->next;
		while(temp != NULL) {
			printf("Freeing messages in queue: %u\n", temp->uniqueId);
			if(temp->next != NULL) {
				// if temp isnt the last queue, link the head to its next queue
				head->next = temp->next;
			} else {
				// no more queues after this one
				head->next = NULL;
			}
			message *tempMsg;
			// free the messages in the queue, if there are any
			while(temp->firstMsg != NULL) {
				// if its not the last message in the queue
				if(temp->firstMsg->next != NULL) {
					// used for linking firstMsg
					tempMsg = temp->firstMsg->next;
				} else {
					// no more messages after this one
					tempMsg = NULL;
				}
				printf("	Freeing message of size: %u\n", temp->firstMsg->size);
				// free first message in the queue
				free(temp->firstMsg->data);
				free(temp->firstMsg);

				// link the other messages onto firstMsg, links null if there are no more
				temp->firstMsg = tempMsg;
			}
			// temps message pointers are gone, can safely free it
			printf("Messages gone, freeing queue: %u\n", temp->uniqueId);
			free(temp);
			// link the other queues onto temp, links null if there are no more
			temp = head->next;
		}
		// all messages are freed, all queues are freed, safe to free head now too (head has no messages).
		free(head);
	} else {
		printf("No message queue exists, no memory was allocated\n");
	}
}

/*
 * Encrypts message src with xor logic using the given key. Inefficient byte-wise xor encryption as opposed to
 * efficient 32-bit wise xor encryption. Directly edits the src data.
*/
long xorEncDec(unsigned char * src, long n, unsigned long key) {
	// assumes validation has already been done to the string, just copy
	unsigned int keyByte[4];
	keyByte[0] = (key & 0xff000000) >> 24;
	keyByte[1] = (key & 0x00ff0000) >> 16;
	keyByte[2] = (key & 0x0000ff00) >> 8;
	keyByte[3] = (key & 0x000000ff);

	for(int i = 0; i < n; i++) {
		src[i] ^= keyByte[i%4];
	}

	return 0;
}	

int main(int argc, char argv[]) {
	unsigned char * user;
	user = malloc(sizeof(unsigned char) * 6);
	
	unsigned char * user2;
	user2 = malloc(sizeof(unsigned char) * 7);

	unsigned char * user3;
	user3 = malloc(sizeof(unsigned char) * 0);

	unsigned char hex[6];
	hex[0] = (unsigned int)(0xDE);
	hex[1] = (unsigned int)(0xAD);
	hex[2] = (unsigned int)(0xBE);
	hex[3] = (unsigned int)(0xEF);
	hex[4] = (unsigned int)(0x12);
	hex[5] = (unsigned int)(0x34);

	unsigned char hexa[7];
	hexa[0] = (unsigned int)(0xDE);
	hexa[1] = (unsigned int)(0xAD);
	hexa[2] = (unsigned int)(0xBE);
	hexa[3] = (unsigned int)(0xEF);
	hexa[4] = (unsigned int)(0x12);
	hexa[5] = (unsigned int)(0x34);
	hexa[6] = (unsigned int)(0x56);

	unsigned char emp[0];

	strncpy(user, hex, 6);
	strncpy(user2, hexa, 7);
	strncpy(user3, emp, 0);

	printf("CREATING QUEUES ADDING MESSAGES TO THEM\n");

	addQueue(1, 0);
	addMessage(1, user, 6, 0);
	
	addQueue(2, 1);
	addMessage(2, user, 6, (unsigned long)(0x1BADC0DE));
	addMessage(2, user2, 7, 0);
	addMessage(2, user2, 7, 0);
	addMessage(2, user2, 7, 0);
	addMessage(2, user2, 7, 0);
	addMessage(1, user2, 7, 0);

	addQueue(3, 0);
	addMessage(3, user3, 0, 0);

	addQueue(3, 1);
	addQueue(2, 0);
	addQueue(1, 0);

	printf("CHECKING SIZE OF NEXT MESSAGES\n");

	printf("length of next message in %ld: %ld\n", 1, lenMessage(1));
	printf("length of next message in %ld: %ld\n", 2, lenMessage(2));

	printf("DUMPING QUEUE\n");

	dumpQueue();

	unsigned char *user4;
	unsigned char *user5;
	user4 = malloc(sizeof(char)*6);
	user5 = malloc(sizeof(char)*7);

	printf("REMOVING MESSAGES FROM EXISTING QUEUES\n");

	removeMessage(2, user5, 7, (unsigned long)(0x1BADC0DE));
	removeMessage(2, user4, 6, (unsigned long)(0x1BADC0DE));
	removeMessage(1, user4, 6, (unsigned long)(0x1BADC0DE));
	removeMessage(1, user4, 6, (unsigned long)(0x1BADC0DE));
	removeMessage(3, user3, 0, 0);

	unsigned long j;
	printf("PRINTING REMOVED MESSAGES\n");
	printf("	");
	for(int i = 0; i < 7; i++) {
		j = (unsigned long) user5[i];
		printf("%u", j);
		if(i != 6) {
			printf(",");
		}
	}
	printf("\n");
	printf("	");
	for(int i = 0; i < 6; i++) {
		j = (unsigned long) user4[i];
		printf("%u", j);
		if(i != 5) {
			printf(",");
		}
	}
	printf("\n");

	printf("DUMPING QUEUE\n");

	dumpQueue();

	printf("READDING REMOVED MESSAGES\n");

	addMessage(2, user, 6, (unsigned long)(0x1BADC0DE));
	addMessage(2, user, 6, (unsigned long)(0x1BADC0DE));
	addMessage(1, user, 6, (unsigned long)(0x1BADC0DE));

	printf("REMOVING AN EMPTY QUEUE\n");
	removeQueue(3);

	printf("TRYING TO REMOVE A NONEMPTY QUEUE\n");
	removeQueue(2);
	removeQueue(1);

	printf("CHECKING SIZE OF NEXT MESSAGES\n");

	printf("length of next message in %ld: %ld\n", 1, lenMessage(1));
	printf("length of next message in %ld: %ld\n", 2, lenMessage(2));
	
	printf("DUMPING QUEUE\n");

	dumpQueue();

	printf("REMOVING ALL MESSAGES, DELETING ALL QUEUES\n");

	removeMessage(2, user3, 0, 0);
	removeMessage(2, user3, 0, 0);
	removeMessage(2, user3, 0, 0);
	removeMessage(2, user3, 0, 0);
	removeMessage(2, user3, 0, 0);
	removeMessage(1, user3, 0, 0);
	removeMessage(1, user3, 0, 0);
	
	printf("REMOVING MESSAGES THAT DONT EXIST\n");
	removeMessage(2, user3, 0, 0);
	removeMessage(2, user3, 0, 0);
	removeMessage(2, user3, 0, 0);
	removeMessage(2, user3, 0, 0);
	removeMessage(2, user3, 0, 0);
	removeMessage(1, user3, 0, 0);
	removeMessage(1, user3, 0, 0);

	printf("length of next message in %ld: %ld\n", 1, lenMessage(1));
	printf("length of next message in %ld: %ld\n", 2, lenMessage(2));

	removeQueue(2);
	removeQueue(1);

	printf("DUMPING QUEUE\n");
	
	dumpQueue();

	printf("READDING QUEUES AND MESSAGES\n");

	addQueue(1, 0);
	addQueue(2, 0);
	addQueue(3, 0);

	addMessage(1, user3, 0, 0);
	addMessage(1, user3, 0, 0);
	addMessage(2, user3, 0, 0);

	printf("TESTING listQueue WITH SIZE 5\n");

	long numCopied;
	unsigned long * messQueues;
	messQueues = (unsigned long*) malloc(sizeof(unsigned long) * 5);
	numCopied = listQueue(messQueues, 5);
	printf("%ld\n", numCopied);
	for(long i = 0; i < numCopied; i++) {
		printf("Message queue: %ld\n", messQueues[i]);
	}

	printf("DUMPING QUEUE\n");
	
	dumpQueue();

	freeQueue();
	free(user);
	free(user2);
	free(user3);
	free(user4);
	free(user5);
	free(messQueues);

	return 0;
}
