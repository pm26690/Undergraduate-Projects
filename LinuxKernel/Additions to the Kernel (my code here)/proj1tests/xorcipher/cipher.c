#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

/*
 * encr/decr the message msg of size n using the unsigned long key, the encr/decr message is placed into
 * the same msg which was passed to this function. Return a 0 on success and an appropriate error value
 * on failure.
 * (not well tested, not fully operational)
*/
long xorEncDec(unsigned char * msg, long n, unsigned long key) {
	// encrypt the message 32 bits at a time, or 4 characters at a time
	if (n % 4 == 0) {
		// compare 4 bytes at a time, big endian
		for(int i = 0; i < n; i=i+4) {			
			char frst = msg[i]; 
			char scnd = msg[i+1];
			char thrd = msg[i+2];
			char frth = msg[i+3];
			
			// convert four bytes at a time to an unsigned long int, big endian
			unsigned long a = frth | (thrd<<8) | (scnd<<16) | (frst<<24);
			unsigned long c;
			c = a ^ key;

			msg[i]   = (c & 0xff000000) >> 24;
			msg[i+1] = (c & 0x00ff0000) >> 16;
			msg[i+2] = (c & 0x0000ff00) >> 8;
			msg[i+3] = (c & 0x000000ff);
		}
	} else {
	// if the number of bytes is not divisible by 4 (number of bits not divisible by 32) then pad
	// zero bytes onto the end of the key for the last n%4 bytes
		int m = n - (n % 4);
		// process first n - (n%4) bytes the same as before, big endian
		for(int i = 0; i < (n - (n%4)); i=i+4) {
			int frst = msg[i]; 
			int scnd = msg[i+1];
			int thrd = msg[i+2];
			int frth = msg[i+3];
			
			unsigned long a = frth | (thrd<<8) | (scnd<<16) | (frst<<24);
			unsigned long c;

			c = a ^ key;

			msg[i]   = (c & 0xff000000) >> 24;
			msg[i+1] = (c & 0x00ff0000) >> 16;
			msg[i+2] = (c & 0x0000ff00) >> 8;
			msg[i+3] = (c & 0x000000ff);
		}
		
		// the last n%4 bytes are encrypted individually, big endian
		m = n%4;
		int i = n-m;
		if(m == 3) {
			int frst = msg[i]; 
			int scnd = msg[i+1];
			int thrd = msg[i+2];
			int frth = 0;

			unsigned long a = frth | (thrd<<8) | (scnd<<16) | (frst<<24);
			unsigned long c;

			c = a ^ key;

			msg[i]   = (c & 0xff000000) >> 24;
			msg[i+1] = (c & 0x00ff0000) >> 16;
			msg[i+2] = (c & 0x0000ff00) >> 8;

		} else if (m == 2) {
			int frst = msg[i]; 
			int scnd = msg[i+1];
			int thrd = 0;
			int frth = 0;

			unsigned long a = frth | (thrd<<8) | (scnd<<16) | (frst<<24);
			unsigned long c;

			c = a ^ key;

			msg[i]   = (c & 0xff000000) >> 24;
			msg[i+1] = (c & 0x00ff0000) >> 16;

		} else if (m == 1) {
			int frst = msg[i]; 
			int scnd = 0;
			int thrd = 0;
			int frth = 0;

			unsigned long a = frth | (thrd<<8) | (scnd<<16) | (frst<<24);
			unsigned long c;

			c = a ^ key;

			msg[i]   = (c & 0xff000000) >> 24;
		}
	}
	
	return 0;
}
/*
 * extremely simplified but inefficient byte wise xor cipher, does the same thing as xorEncDec but owes its
 * inefficiency to the fact that it does not compare the data 32 bits at a time.
 * (well tested and operational)
*/
long bytewiseEncDec(unsigned char * msg, long n, unsigned long key) {
	int keyByte[4];
	keyByte[0] = (key & 0xff000000) >> 24;
	keyByte[1] = (key & 0x00ff0000) >> 16;
	keyByte[2] = (key & 0x0000ff00) >> 8;
	keyByte[3] = (key & 0x000000ff);

	for(int i = 0; i < n; i++) {
		msg[i] ^= keyByte[i%4];
	}

	return 0;
}	

int main(int argc, char * argv[]) {
	unsigned char foo[11] = "CASSIDYCRO\0";
	unsigned char * bar;
	bar = malloc(strlen(foo) + 1);
	bar = strcpy(bar, foo);
	
	unsigned long k = 987758345;

	printf("%s\n", bar);
	bytewiseEncDec(bar, 10, k);
	printf("%s\n", bar);
	bytewiseEncDec(bar, 10, k);
	printf("%s\n", bar);

	unsigned char hex[7];
	hex[0] = (int)(0xDE);
	hex[1] = (int)(0xAD);
	hex[2] = (int)(0xBE);
	hex[3] = (int)(0xEF);
	hex[4] = (int)(0x12);
	hex[5] = (int)(0x34);
	hex[6] = '\0';
	
	unsigned char * adec;
	adec = malloc(strlen(hex) + 1);
	adec = strcpy(adec, hex);

	unsigned long k2 = (unsigned long)(0x1BADC0DE);

	printf("%s\n", adec);

	for(int i = 0; i < 6; i++) {
		int j;
		j = (int) adec[i];
		printf("%i,",j);
	}

	printf("\n");

	bytewiseEncDec(adec, 6, k2);
	printf("%s\n", adec);

	for(int i = 0; i < 6; i++) {
		int j;
		j = (int) adec[i];
		printf("%i,",j);
	}

	printf("\n");

	bytewiseEncDec(adec, 6, k2);
	printf("%s\n", adec);

	for(int i = 0; i < 6; i++) {
		int j;
		j = (int) adec[i];
		printf("%i,",j);
	}

	printf("\n");

	free(bar);
	free(adec);
}
