#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/errno.h>

/* begin functions and structs used by system calls */

/* 
 * errcode values table
*/

/* eperm		= -1; 	 OPERATION NOT PERMITTED */
/* enoent 		= -2;	 NO SUCH FILE OR DIRECTORY (no such message queue) */
/* enomem 		= -12;	 OUT OF MEMORY (allocation failed) */
/* efault 		= -14;	 BAD ADDRESS (null pointer) */
/* eexist 		= -17;	 FILE EXISTS (queue already exists) */
/* einval 		= -22;	 INVALID ARGUMENT (negative parameters) */
/* enotempty	= -39;	 DIRECTORY NOT EMPTY (queue not empty) */
/* enomsg		= -42;	 NO MESSAGE OF DESIRED TYPE (empty queue) */


/*
 * the mailbox nodes, called queues because that's what I called them in user space.
*/
typedef struct queue {
	// unique id assigned to a given mailbox
	unsigned long uniqueId;
	// whether or not a given mailbox is encrypted
	unsigned int isEncrypted;
	// dummy header doesn't need to be included in anything	
	unsigned int isHead;
	// number of messages in a given mailbox
	unsigned long numMsg;
	// pointer to the next mailbox, if such a mailbox exists
	struct queue *next;
	// pointer to the first message in the mailbox, if such a mailbox exists
	struct message *firstMsg;
} queue;

/*
 * the message nodes, the messages in a given mailbox
*/
typedef struct message {
	// pointer to the data contained in a message, if such data exists
	unsigned char *data;
	// size, in bytes, of the data contained in a message (actual size in bytes, not the size given by user)
	long size;
	// pointer to the next message behind this one, if such a message exists
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

long addQueue(unsigned long id, int crypt) {
	long exists;
	queue *newQueue;
	queue *oldQueue;

	newQueue = NULL;
	oldQueue = NULL;

	// create a dummy head if no message queue exists, the dummy head will not contain an ID or any messages
	if(head == NULL) {
		head = kmalloc(sizeof(queue), GFP_KERNEL);
		head->isHead = 1;
		head->next = NULL;
		head->firstMsg = NULL;
	}
	
	// check if a queue at the target value already exists, there should be no duplicate queues, so this
	// would return a placeholder negative value for an error (dont have specific errors yet)
	exists = isQueue(id);
	if(exists < 0) {
		// errno = EEXIST
		return -EEXIST; // queue already exists, eexist
	}

	// either this is the first message queue added, or other queues already exist, if other queues
	// exist make this queue the new first message queue and push the others down (order doesn't appear
	// to matter in the spec so this should be ok)
	if(head->next == NULL) {
		// first queue in the list
		newQueue = kmalloc(sizeof(queue), GFP_KERNEL);
		head->next = newQueue;
		newQueue->uniqueId = id;
		newQueue->isEncrypted = crypt;
		newQueue->isHead = 0;
		newQueue->next = NULL;
		newQueue->firstMsg = NULL;
		return 0;
	} else {
		// not first queue, strap this new one on top and push the others down
		newQueue = kmalloc(sizeof(queue), GFP_KERNEL);
		oldQueue = head->next;			
		head->next = newQueue;
		newQueue->next = oldQueue;
		// define other data as normal
		newQueue->uniqueId = id;
		newQueue->isEncrypted = crypt;
		newQueue->isHead = 0;
		newQueue->firstMsg = NULL;
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
		return -ENOENT; // there are no queues, queue does not exist
	} else {
		nextQueue = head->next;
		// check if there are any queues whatsoever other than the dummy node
		if(nextQueue != NULL) {
			// check if the targeted queue exist by linear traversal of the queue list
			while(nextQueue != NULL && targetQueue == NULL) {
				if(nextQueue->uniqueId == id) {
					targetQueue = nextQueue;
				}
				
				if(targetQueue == NULL) {
					predQueue = nextQueue;
					nextQueue = nextQueue->next;
				}
			}
		} else {
			// there are no message queues at all, stop search now and return appropriate error
			return -ENOENT;
		}
	}

	if(targetQueue != NULL) {
		if(targetQueue->numMsg == 0) {
			if(predQueue != NULL) {
				// first queue was not the target queue
				if(targetQueue->next == NULL) {
					// target queue was the last queue, don't need to relink anything				
					kfree(targetQueue);
					predQueue->next = NULL;
					return 0;
				} else {
					// target queue was somewhere in between head and the last queue, must relink
					predQueue->next = targetQueue->next;
					targetQueue->next = NULL;
					kfree(targetQueue);
					return 0;
				}
			} else {
				// first queue was the target queue, perhaps it was the only queue
				if(targetQueue->next == NULL) {
					head->next = NULL;
				} else {
					head->next = targetQueue->next;
					targetQueue->next = NULL;
				}
				kfree(targetQueue);
				return 0;
			}
		} else {
			return -ENOTEMPTY; // cant get rid of non empty queue
		}
	} else {
		return -ENOENT; // queue does not exist
	}
}

/*
 * check if target queue exists, and add a message to it with the given encryption key
*/
long addMessage(unsigned long id, unsigned char * msg, long n, unsigned long key) {
	queue *targetQueue;
	queue *nextQueue;
	message *newMsg;
	message *oldMsg;

	targetQueue = NULL;
	nextQueue = NULL;
	newMsg = NULL;
	oldMsg = NULL;

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
		// errno = ENOENT
		return -ENOENT; // enoent
	}
	
	// if targeted queue does not exist, return appropriate error, else add the message to it
	if(targetQueue == NULL) {
		//errno = ENOENT
		return -ENOENT; // enoent
	} else {
		// either the targeted queue has no messages, or it does and we will make this message the new first
		// message and push the others down. this keeps our FIFO queue structure intact.
		if(targetQueue->firstMsg == NULL) {
			// first message in the queue
			newMsg = kmalloc(sizeof(message), GFP_KERNEL);
			targetQueue->firstMsg = newMsg;
			newMsg->data = kmalloc(sizeof(unsigned char)*n, GFP_KERNEL);
			newMsg->size = n;
			// encrypt message if necessary, dont want to edit given memory
			if(targetQueue->isEncrypted == 1) {
				memcpy(newMsg->data, msg, n);
				xorEncDec(newMsg->data, newMsg->size, key);
			} else {
				memcpy(newMsg->data, msg, n);
			}
			targetQueue->numMsg = 1;
			return 0;
		} else {
			// not first message in queue, strap this one on top and push the others down
			newMsg = kmalloc(sizeof(message), GFP_KERNEL);
			oldMsg = targetQueue->firstMsg;
			targetQueue->firstMsg = newMsg;
			newMsg->data = kmalloc(sizeof(unsigned char)*n, GFP_KERNEL);
			newMsg->size = n;
			newMsg->next = oldMsg;
			// encrypt message if necessary, dont want to edit given memory
			if(targetQueue->isEncrypted == 1) {
				memcpy(newMsg->data, msg, n);
				xorEncDec(newMsg->data, newMsg->size, key);
			} else {
				memcpy(newMsg->data, msg, n);
			}
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

	if(head == NULL) {
		return -ENOENT; // no head, no queues
	}

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
						msgToCopy = kmalloc(sizeof(unsigned char)*targetQueue->firstMsg->size, GFP_KERNEL);
						memcpy(msgToCopy, targetQueue->firstMsg->data, targetQueue->firstMsg->size);

						if(targetQueue->isEncrypted != 0) {
							xorEncDec(msgToCopy, targetQueue->firstMsg->size, key);
							memcpy(msg, msgToCopy, n);
						} else {
							memcpy(msg, msgToCopy, n);
						}
		
						targetMessage = targetQueue->firstMsg;				
					
						// free dynamically allocated memory
						kfree(targetMessage->data);
						kfree(targetMessage);
						kfree(msgToCopy);
						targetQueue->numMsg = 0;
						targetQueue->firstMsg = NULL;
						return 0;
			
					} else {
						// the target queue has multiple messages

						// dont want to edit the message directly in case of errors, so copy it
						msgToCopy = kmalloc(sizeof(unsigned char)*targetQueue->firstMsg->size, GFP_KERNEL);
						memcpy(msgToCopy, targetQueue->firstMsg->data, targetQueue->firstMsg->size);
		
						if(targetQueue->isEncrypted != 0) {
							xorEncDec(msgToCopy, targetQueue->firstMsg->size, key);
							memcpy(msg, msgToCopy, n);
						} else {
							memcpy(msg, msgToCopy, n);
						}
			
						// link first message to the next messages
						targetMessage = targetQueue->firstMsg; // keep this for later
						targetQueue->firstMsg = targetQueue->firstMsg->next;

						// free dynamically allocated memory
						kfree(targetMessage->data);
						kfree(targetMessage);
						kfree(msgToCopy);
						targetQueue->numMsg = targetQueue->numMsg - 1;
						return 0;
					}
				} else {
					return -ENOMSG; // target queue has no messages
				}
			}
			nextQueue = nextQueue->next;
		}
		return -ENOENT; // while loop found no target queue
	} else {
		// there are no message queues at all, stop search now and return appropriate error
		return -ENOENT;
	}
}

/*
 * does everything that remove message does without removing the message
*/
long peekMessage(unsigned long id, unsigned char *msg, long n, unsigned long key) {
	
	queue *targetQueue;
	queue *nextQueue;
	message *targetMessage;
	unsigned char *msgToCopy;

	targetQueue = NULL;
	nextQueue = NULL;
	targetMessage = NULL;

	if(head == NULL) {
		return -ENOENT; // no head, no queues
	}

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
						msgToCopy = kmalloc(sizeof(unsigned char)*targetQueue->firstMsg->size, GFP_KERNEL);
						memcpy(msgToCopy, targetQueue->firstMsg->data, targetQueue->firstMsg->size);

						if(targetQueue->isEncrypted != 0) {
							xorEncDec(msgToCopy, targetQueue->firstMsg->size, key);
							memcpy(msg, msgToCopy, n);
						} else {
							memcpy(msg, msgToCopy, n);
						}

						targetMessage = targetQueue->firstMsg; // keep for later
			
						// free dynamically allocated memory
						kfree(msgToCopy);
						return 0;
			
					} else {
						// the target queue has multiple messages

						// dont want to edit the message directly in case of errors, so copy it
						msgToCopy = kmalloc(sizeof(unsigned char)*targetQueue->firstMsg->size, GFP_KERNEL);
						memcpy(msgToCopy, targetQueue->firstMsg->data, targetQueue->firstMsg->size);
		
						if(targetQueue->isEncrypted != 0) {
							xorEncDec(msgToCopy, targetQueue->firstMsg->size, key);
							memcpy(msg, msgToCopy, n);
						} else {
							memcpy(msg, msgToCopy, n);
						}

						// free dynamically allocated memory
						kfree(msgToCopy);
						return 0;
					}
				} else {
					return -ENOMSG; // target queue has no messages
				}
			}
			nextQueue = nextQueue->next;
		}
		return -ENOENT; // while loop found no target queue
	} else {
		// there are no message queues at all, stop search now and return appropriate error
		return -ENOENT;
	}
}

/*
 * creates a copy of the source string to the destination string dest for the given writable size n, does not
 * modify the source string src, does not need a null character. (maybe doesn't work, I'm using memcpy now).
*/
long writableCopy(unsigned char * dest, unsigned char * src, long n) {
	int i;

	// assumes validation has already been done to the string, just copy.
	for(i = 0; i < n; i++) {
		unsigned int j;
		j = (unsigned int) src[i];
		dest[i] = j;
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
				return -1; // targetQueue already exists
			}
			nextQueue = nextQueue->next;
		}
		return 0; // targetQueue not found
	} else {
		// there are no message queues at all, stop search now and return appropriate value
		return 0;
	}
}

/*
 * Does a linear traversal of the message queues and returns the total number of queues
*/
long numQueues(void) {
	queue * nextQueue;
	long total;
	nextQueue = NULL;
	total = 0;

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
		return -ENOENT; // there are no queues, queue does not exist
	} else {
		nextQueue = head->next;
		// check if there are any queues whatsoever other than the dummy node
		while(nextQueue != NULL) {
			if(nextQueue->uniqueId == id) {
				targetQueue = nextQueue;
				return targetQueue->numMsg;
			}
			nextQueue = nextQueue->next;
		}
		return -ENOENT; // target queue does not exist
	}
}

/*
 * find the length of the next message returned by the target queue
*/
long lenMessage(unsigned long id) {
	queue *targetQueue;
	queue *nextQueue;
	targetQueue = NULL;
	nextQueue = NULL;

	if(head == NULL) {
		return -ENOENT; // there are no queues, queue does not exist
	} else {
		nextQueue = head->next;
		// check if there are any queues whatsoever other than the dummy node
		while(nextQueue != NULL) {
			if(nextQueue->uniqueId == id) {
				targetQueue = nextQueue;
				if(targetQueue->firstMsg != NULL) {
					return targetQueue->firstMsg->size;
				} else {
					return -ENOMSG;
				}
			}
			nextQueue = nextQueue->next;
		}
		return -ENOENT; // target queue does not exist
	}
}

/*
 * returns a list of up to k mailbox Ids
*/
long listQueue(unsigned long * mbxes, long k) {
	unsigned long *queueIds;
	queue *nextQueue;
	long i;
	nextQueue = NULL;
	queueIds = NULL;

	queueIds = (unsigned long*) kmalloc(sizeof(unsigned long) * k, GFP_KERNEL);

	if(head != NULL) {
		if(head->next != NULL) {
			nextQueue = head->next;
			i = 0;
			while(nextQueue != NULL) {
				queueIds[i] = nextQueue->uniqueId;
				i = i + 1;
				nextQueue = nextQueue->next;
			}
			memcpy(mbxes, queueIds, sizeof(unsigned long) * k);
			kfree(queueIds);
			return 0;
		} else {
			return -ENOENT; // no queues
		}
	} else {
		return -ENOENT; // no queues
	}
}

/*
 * Encrypts message src with xor logic using the given key. Inefficient byte-wise xor encryption as opposed to
 * efficient 32-bit wise xor encryption. Directly edits the src data.
*/
long xorEncDec(unsigned char * src, long n, unsigned long key) {
	int i;

	// assumes validation has already been done to the string, just copy
	unsigned int keyByte[4];
	keyByte[0] = (key & 0xff000000) >> 24;
	keyByte[1] = (key & 0x00ff0000) >> 16;
	keyByte[2] = (key & 0x0000ff00) >> 8;
	keyByte[3] = (key & 0x000000ff);

	for(i = 0; i < n; i++) {
		src[i] ^= keyByte[i%4];
	}

	return 0;
}

/* end functions and structs used by system calls */

/* begin syscall definitions */

/*
 * creates a new empty mailbox with ID id, if it does not already exist, and returns 0. The queue should
 * be flagged for encryption if the enable_crypt option is set to anything other than 0. If enable_crypt
 * is set to zero, then the key parameter in any functions including it should be ignored.
*/
SYSCALL_DEFINE2(create_mbox_421, unsigned long, id, int, enable_crypt) {
	int encrypt;
	long code;

	if(enable_crypt != 0) {
		encrypt = 1;
	}

	code = addQueue(id, encrypt);

	return code;
}

/*
 * removes mailbox with ID id, if it empty, and returns 0. If the mailbox is not empty, this system call
 * should return an appropriate error and not remove the mailbox
*/
SYSCALL_DEFINE1(remove_mbox_421, unsigned long, id) {
	// functionality in progress
	long numMsgs;
	numMsgs = numMessages(id);	
		
	if(numMsgs > 0) {
		return -ENOTEMPTY;
	}

	if(numMsgs < 0) {
		return -ENOENT;
	}

	return removeQueue(id);
	
}

/*
 * returns the number of existing mailboxes.
*/
SYSCALL_DEFINE0(count_mbox_421) {
	return numQueues();
}

/* 
 * returns a list of up to k mailbox IDs in the user-space variable mbxes. It returns the number of IDs
 * written successfully to mbxes on success and an appropriate error code on failure.
*/
SYSCALL_DEFINE2(list_mbox_421, unsigned long __user *, mbxes, long, k) {
	// functionality in progress
	unsigned long * queueIds;	
	long numQueue;
	long copy;
	numQueue = numQueues();
	
	if(k < 0) {
		return -EINVAL; // invalid k value
	}

	if(numQueue == 0) {
		return -ENOENT; // no queues to copy
	}
	
	if(mbxes == NULL) {
		return -EFAULT; // null pointer
	}

	if(numQueue > k) {
		queueIds = (unsigned long*) kmalloc(sizeof(unsigned long) * k, GFP_KERNEL);
		listQueue(queueIds, k);
		copy = copy_to_user(mbxes, queueIds, sizeof(unsigned long) * k);
		kfree(queueIds);
		return k;
	} else {
		queueIds = (unsigned long*) kmalloc(sizeof(unsigned long) * numQueue, GFP_KERNEL);
		listQueue(queueIds, numQueue);
		copy = copy_to_user(mbxes, queueIds, sizeof(unsigned long) * numQueue);
		kfree(queueIds);
		return numQueue;
	}

	return 0;
}

/*
 * encryptes the message msg (if appropriate), adding it to the already existing mailbox identified.
 * Returns the number of bytes stored (which should be equal to the message length n) on success, and
 * an appropriate error code on failure. Messages with negative lengths shall be rejected as invalid
 * and cause an appropriate error to be returned, however messages with a length of zero shall be
 * accepted as valid
*/
SYSCALL_DEFINE4(send_msg_421, unsigned long, id, unsigned char __user *, msg, long, n, unsigned long, key) {
	long code;
	long copyCode;
	unsigned char *kcopy;

	// check that n is a valid length
	if(n >= 0) {
		if(msg == NULL && n != 0) {
			// errno = EFAULT
			return -EFAULT; // user provided null pointer
		}
		// copy to pass to addMessage
		kcopy = kmalloc(sizeof(unsigned char)*n, GFP_KERNEL);	
		copyCode = copy_from_user(kcopy, msg, n);
		if((copyCode == n && copyCode != 0) || copyCode < 0) {
			kfree(kcopy);
			// errno = EFAULT
			return -EFAULT;
		} else {
			code = addMessage(id, kcopy, n, key);
			kfree(kcopy);
			if(code < 0) {
				return code; // mailbox did not exist
			} else {
				return copyCode; // return number of bytes copied or 0 if there were no size issues
			}
		}
	} else {
		// errno = EINVAL
		return -EINVAL; // invalid size of message
	}
}

/*
 * copies up to n characters from the next message in the mailbox id to the user-space buffer msg,
 * decrypting with the specified key (if appropriate), and removes the entire message from the mailbox
 * (even if only part of the message is copied out). Returns the number of bytes successfully copied
 * (which should be the minimum of the length of the message that is stored and n) on success or an
 * appropriate error code on failure
*/
SYSCALL_DEFINE4(recv_msg_421, unsigned long, id, unsigned char __user *, msg, long, n, unsigned long, key) {
	// project specification says this should return min(n, message_length) but what if
	// the user provides a pointer that isn't big enough? I just return how much was copied.

	// mostly functional but rather inefficient, the findQueue() style calls happene several times
	// ending in me traversing the list about four times

	long code;
	long msgSize;
	long numMsgs;
	unsigned char *kcopy;

	kcopy = NULL;
	code = 0;
	msgSize = 0;
	numMsgs = 0;

	// invalid input
	if(n < 0) {
		return -EINVAL;
	}

	// null pointer w/o 0 length n
	if(msg == NULL && n != 0) {
		return -EFAULT;
	}
	
	// queue must have messages
	numMsgs = numMessages(id);
	if(numMsgs == 0) {
		return -ENOMSG;
	}

	// queue must exist
	if(numMsgs < 0) {
		return -ENOENT;
	}

	// find lenMessage so we can do min(n,msgSize), also checkes if the message is in the queue
	msgSize = lenMessage(id);
	if(msgSize < 0) {
		return msgSize; // only for ENOENT
	} else {
		if(n == 0) {
			// size is irrelevant just remove the message, dont need it for anything anyway
			// kind of inefficient, contains similar code to lenMessage
			kcopy = kmalloc(sizeof(unsigned char)*n, GFP_KERNEL);
			removeMessage(id, kcopy, n, key);
			kfree(kcopy);
			return 0;
		} else {
			// copy message to size min(n,msgSize) where msgSize is the size of the next message
			// in mailbox at id
			if(msgSize > n) {
				kcopy = kmalloc(sizeof(unsigned char)*n, GFP_KERNEL);
				removeMessage(id, kcopy, n, key);
				code = copy_to_user(msg, kcopy, n);
				kfree(kcopy);
				return n; // normal usage, return min(n,msgSize)
				
			} else {
				kcopy = kmalloc(sizeof(unsigned char)*msgSize, GFP_KERNEL);
				removeMessage(id, kcopy, msgSize, key);
				code = copy_to_user(msg, kcopy, msgSize);
				kfree(kcopy);
				return msgSize; // normal usage, return min(n,msgSize)
			}
		}
	}
}

/*
 * performs the same operation as recv_msg_421() without removing the message from the mailbox
*/

SYSCALL_DEFINE4(peek_msg_421, unsigned long, id, unsigned char __user *, msg, long, n, unsigned long, key) {
	// project specification says this should return min(n, message_length) but what if
	// the user provides a pointer that isn't big enough? I just return how much was copied.

	// mostly functional but rather inefficient, the findQueue() style calls happene several times
	// ending in me traversing the list about four times

	long code;
	long msgSize;
	long numMsgs;
	unsigned char *kcopy;

	kcopy = NULL;
	code = 0;
	msgSize = 0;

	// invalid input
	if(n < 0) {
		return -EINVAL;
	}

	// null pointer w/o 0 length n
	if(msg == NULL && n != 0) {
		return -EFAULT;
	}
	
	// queue must have messages
	numMsgs = numMessages(id);
	if(numMsgs == 0) {
		return -ENOMSG;
	}

	// queue must exist
	if(numMsgs < 0) {
		return -ENOENT;
	}

	// find lenMessage so we can do min(n,msgSize), also checks if the message is in the queue
	msgSize = lenMessage(id);
	if(msgSize < 0) {
		return msgSize;
	} else {
		if(n == 0) {
			// size is irrelevant just remove the message, dont need it for anything anyway
			// kind of inefficient, contains similar code to lenMessage
			kcopy = kmalloc(sizeof(unsigned char)*n, GFP_KERNEL);
			removeMessage(id, kcopy, n, key);
			kfree(kcopy);
			return 0;
		} else {
			// copy message to size min(n,msgSize) where msgSize is the size of the next message
			// in mailbox at id
			if(msgSize > n) {
				kcopy = kmalloc(sizeof(unsigned char)*n, GFP_KERNEL);
				removeMessage(id, kcopy, n, key);
				code = copy_to_user(msg, kcopy, n);
				kfree(kcopy);
				return n; // normal usage, return min(n,msgSize)
				
			} else {
				kcopy = kmalloc(sizeof(unsigned char)*msgSize, GFP_KERNEL);
				removeMessage(id, kcopy, msgSize, key);
				code = copy_to_user(msg, kcopy, msgSize);
				kfree(kcopy);
				return msgSize; // normal usage, return min(n,msgSize)
			}
		}
	}
}

/*
 * returns the number of messages in the mailbox id on success or an appropriate error code on failure
*/
SYSCALL_DEFINE1(count_msg_421, unsigned long, id) {
	// functional as far as I have tested
	long code;
	code = numMessages(id);
	return code;
}

/* 
 * returns the length of the next message that would be returned by calling sys_rcv_msg_421() with the
 * same id value (that is the number of bytes in the next message in the mailbox). If there are no
 * messages in the mailbox, this should return an appropriate error value.
*/
SYSCALL_DEFINE1(len_msg_421, unsigned long, id) {
	// functional as far as I have tested, acts funny on queus that have had all messages removed
	long code;
	code = lenMessage(id);
	return code;
}

/* end syscall definitions */
