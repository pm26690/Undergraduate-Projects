/*
 * A VMUFS has the following format:
 *
 * Blocks 000-199: user data, all 0 bytes
 * Blocks 200-240: normally unused data, all 0 bytes
 * Blocks 241-253: single level directory, all 0 bytes
 * Block      254: File allocaiton table:
 *                     > 0xfffc designates free block
 *                     > 0xfffa designates end of chain marker
 *                     > each used block should contain either an end of chain marker
 *                       or a pointer to the next block in the chain
 *                     > the directory is set up in the file allocation table as if it
 *                       were a file, so the entry for block 253 of the entire file system
 *                       will point to 252, 252 will point to 251, and so on until 242 which
 *                       will point to 241 which is the end of the chain. 
 *                     > The blocks for the FAT and root are set as end of chain markers.
 *                     > 240-0 are free blocks.
 * Block      255: Root:
 *                     > mostly zero bytes except for binary encoded decimal timestamp and
 *                     > some other fixed values.
*/

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
/*
 * Begin definition of helper functions
*/

/*
 * Takes an integer between 0-60 and outputs the proper binary coded hex value that looks
 * like that decimal, I use switch cases because it's probably the fastest way and I am
 * too lazy to think of some other way to do it. Goes to 60 instead of 59 because tm_sec
 * includes something called leap seconds which can have value 60.
*/

int BCD(int dec) {

	switch(dec) {
		case 0:
			return 0x00;
		case 1:
			return 0x01;
		case 2:
			return 0x02;
		case 3:
			return 0x03;
		case 4:
			return 0x04;
		case 5:
			return 0x05;
		case 6:
			return 0x06;
		case 7:
			return 0x07;
		case 8:
			return 0x08;
		case 9:
			return 0x09;
		case 10:
			return 0x10;
		case 11:
			return 0x11;
		case 12:
			return 0x12;
		case 13:
			return 0x13;
		case 14:
			return 0x14;
		case 15:
			return 0x15;
		case 16:
			return 0x16;
		case 17:
			return 0x17;
		case 18:
			return 0x18;
		case 19:
			return 0x19;
		case 20:
			return 0x20;
		case 21:
			return 0x21;
		case 22:
			return 0x22;
		case 23:
			return 0x23;
		case 24:
			return 0x24;
		case 25:
			return 0x25;
		case 26:
			return 0x26;
		case 27:
			return 0x27;
		case 28:
			return 0x28;
		case 29:
			return 0x29;
		case 30:
			return 0x30;
		case 31:
			return 0x31;
		case 32:
			return 0x32;
		case 33:
			return 0x33;
		case 34:
			return 0x34;
		case 35:
			return 0x35;
		case 36:
			return 0x36;
		case 37:
			return 0x37;
		case 38:
			return 0x38;
		case 39:
			return 0x39;
		case 40:
			return 0x40;
		case 41:
			return 0x41;
		case 42:
			return 0x42;
		case 43:
			return 0x43;
		case 44:
			return 0x44;
		case 45:
			return 0x45;
		case 46:
			return 0x46;
		case 47:
			return 0x47;
		case 48:
			return 0x48;
		case 49:
			return 0x49;
		case 50:
			return 0x50;
		case 51:
			return 0x51;
		case 52:
			return 0x52;
		case 53:
			return 0x53;
		case 54:
			return 0x54;
		case 55:
			return 0x55;
		case 56:
			return 0x56;
		case 57:
			return 0x57;
		case 58:
			return 0x58;
		case 59:
			return 0x59;
		case 60:
			return 0x60;
	}
	
	return -1; // no possible representation of given value
}

/*
 * Create the file, if successful return 0, if not return an appropriate negative value
*/
int createFile(char * filename) {
	// create the file with the given filename if it does not already exist
	FILE * file = fopen(filename, "r");
	if (file != NULL) {
		fclose(file);
		return -1; // invalid arguments
	} else {
		file = fopen(filename, "w+");
		fclose(file);
		return 0; // no errors
	}
}

/*
 * Write the user data blocks to the file, if successful return 0, if not return an
 * appropriate negative value and delete the file.
*/
int userData(char * filename) {
	// open the file for writing
	FILE * file = fopen(filename, "w");
	int errcode = 0;
	
	// write 200 blocks of zero bytes, unsigned char is a non negative byte of data
	unsigned char d = 0x00;
	for (int i = 0; i < 200*512; i++) {
		errcode = fputc(d, file);
		
		// fputc returns the byte that it wrote on success, if the two arent equal
		// then something bad happened and we should delete the file and return an
		// appropriate error.
		if(errcode != d) {
			printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
			fclose(file);
			remove(filename);
			return -2; // could not write to file
		}
	}
	
	// close the file
	fclose(file);
	return 0; // no errors
}

/*
 * Append the unused data blocks to the file, if successful return 0, if not return an
 * appropriate negative value and delete the file.
*/
int unusedData(char * filename) {
	// open the file for appending
	FILE * file = fopen(filename, "a");
	int errcode = 0;
	
	// write 41 blocks of zero bytes, unsigned char is a non negative byte of data
	unsigned char d = 0x00;
	for (int i = 0; i < 41*512; i++) {
		errcode = fputc(d, file);
		
		// fputc returns the byte that it wrote on success, if the two arent equal
		// then something bad happened and we should delete the file and return an
		// appropriate error.
		if(errcode != d) {
			printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
			fclose(file);
			remove(filename);
			return -2; // could not write to file
		}
	}
	
	// close the file
	fclose(file);
	return 0; // no errors
}

/*
 * Append the directory blocks to the file, if successful return 0, if not return an
 * appropriate negative value and delete the file.
*/
int directory(char * filename) {
	// open the file for appending
	FILE * file = fopen(filename, "a");
	int errcode = 0;
	
	// write 13 blocks of zero bytes, unsigned char is a non negative byte of data
	unsigned char d = 0x00;
	for (int i = 0; i < 13*512; i++) {
		errcode = fputc(d, file);
		
		// fputc returns the byte that it wrote on success, if the two arent equal
		// then something bad happened and we should delete the file and return an
		// appropriate error.
		if(errcode != d) {
			printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
			fclose(file);
			remove(filename);
			return -2; // could not write to file
		}
	}
	
	// close the file
	fclose(file);
	return 0; // no errors
}

/*
 * Append the FAT block to the file, if successful return 0, if not return an
 * appropriate negative value and delete the file.
*/
int FAT(char * filename) {
	// open the file for appending
	FILE * file = fopen(filename, "a");
	int errcode = 0;

	// first 241 blocks are free, put 0xfffc
	unsigned char d = 0xff;
	unsigned char e = 0xfc;
	for (int i = 0; i < 241; i++) {
		errcode = fputc(e, file);
		
		// fputc returns the byte that it wrote on success, if the two arent equal
		// then something bad happened and we should delete the file and return an
		// appropriate error.
		if (errcode != e) {
			printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
			fclose(file);
			remove(filename);
			return -2; // could not write to file
		}

		errcode = fputc(d, file);

		if (errcode != d) {
			printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
			fclose(file);
			remove(filename);
			return -2; // could not write to file
		}
	}

	// directory blocks occupy 241-253
	// FAT and root are at 254 and 255
	unsigned char f = 0xff;
	unsigned char g = 0xfa;
	unsigned char h = 0x00;
	for (unsigned int i = 241; i < 256; i++) {
		// block 241 (assuming indexing starts at zero) is an end of chain marker
		if (i == 241) {
			errcode = putc(g, file);
			if (errcode != g) {
				printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
				fclose(file);
				remove(filename);
				return -2; // could not write to file
			}

			errcode = fputc(f, file);
			if (errcode != f) {
				printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
				fclose(file);
				remove(filename);
				return -2; // could not write to file
			}
		// blocks 241-253 (assuming index starts at zero) are their index number (assuming
		// indexing starts at 1) - 1
		} else if (i > 241 && i < 254){
			errcode = putc(i-1, file);
			if (errcode != i-1) {
				printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
				fclose(file);
				remove(filename);
				return -2; // could not write to file
			}

			errcode = putc(h, file);
			if (errcode != h) {
				printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
				fclose(file);
				remove(filename);
				return -2; // could not write to file
			}
		// blocks 254, 255 are end of chain markers
		} else {
			errcode = putc(g, file);
			if (errcode != g) {
				printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
				fclose(file);
				remove(filename);
				return -2; // could not write to file
			}

			errcode = fputc(f, file);
			if (errcode != f) {
				printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
				fclose(file);
				remove(filename);
				return -2; // could not write to file
			}
		}
	}
	// close the file
	fclose(file);
	return 0; // no errors
}

/*
 * Append the root block to the file, if successful return 0, if not return an appropriate
 * negative value and delete the file.
*/
int root(char * filename) {
	// open the file for appending
	FILE * file = fopen(filename, "a");
	int errcode = 0;

	// root signature, bytes 0-15 (indexing from 0)
	unsigned char d = 0x55;
	for (unsigned int i = 0; i < 16; i++) {
		errcode = fputc(d, file);
		
		// fputc returns the byte that it wrote on success, if the two arent equal
		// then something bad happened and we should delete the file and return an
		// appropriate error.
		if (errcode != d) {
			printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
			fclose(file);
			remove(filename);
			return -2; // could not write to file
		}
	}	
	
	// color related bytes 16-19 are zero bytes in my config
	// unused bytes 20-47 are also obviously zero
	unsigned char e = 0x00;
	for (unsigned int i = 16; i < 48; i++) {
		errcode = fputc(e, file);
		if (errcode != e) {
			printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
			fclose(file);
			remove(filename);
			return -2; // could not write to file
		}
	}

	// get the BCD for the timestamp

	// get current time, not quite at files creation but it will be off by at most a few seconds
	time_t curTime = time(NULL);	
	struct tm *curTm = localtime(&curTime);

	int hour  = BCD(curTm->tm_hour);
	int mins  = BCD(curTm->tm_min);
	int secs  = BCD(curTm->tm_sec);

	int wday  = curTm->tm_wday; // 0 = sunday so this needs to be shifted
	// monday should be 0 instead of 1
	// tuesday should be 1 instead of 2
	// ... etc
	if (wday == 0) {
		wday = BCD(6);
	} else {
		wday = BCD(wday - 1);
	}

	int mon   = BCD(curTm->tm_mon + 1); // +1 because this value is from 0 to 11
	int mday  = BCD(curTm->tm_mday);

	int year  = BCD(curTm->tm_year - 100); // this gets us the decimal value on the right
	int cen   = BCD(20); // I presuppose that the year is 20xx, sadly this software will
                          // only be good for the next 81 years.

	// add the bytes 48-55 to the file, one at a time, in order given in spec
	errcode = fputc(cen, file);
	if (errcode != cen) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	errcode = fputc(year, file);
	if (errcode != year) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	errcode = fputc(mon, file);
	if (errcode != mon) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	errcode = fputc(mday, file);
	if (errcode != mday) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	errcode = fputc(hour, file);
	if (errcode != hour) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	errcode = fputc(mins, file);
	if (errcode != mins) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	errcode = fputc(secs, file);
	if (errcode != secs) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	errcode = fputc(wday, file);
	if (errcode != wday) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	// zero bytes for 56-69
	for (unsigned int i = 56; i < 70; i++) {
		errcode = fputc(e, file);
		if (errcode != e) {
			printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
			fclose(file);
			remove(filename);
			return -2; // could not write to file
		}
	}

	int ls; // least sig bits
	int ms; // most sig bits

	// bytes 71-82 are all fixed values so I add them one at a time
	ls = 0xFE;
	ms = 0x00;

	errcode = fputc(ls, file);
	if (errcode != ls) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	errcode = fputc(ms, file);
	if (errcode != ms) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	ls = 0x01;
	ms = 0x00;

	errcode = fputc(ls, file);
	if (errcode != ls) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	errcode = fputc(ms, file);
	if (errcode != ms) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	ls = 0xFD;
	ms = 0x00;

	errcode = fputc(ls, file);
	if (errcode != ls) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	errcode = fputc(ms, file);
	if (errcode != ms) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	ls = 0x0D;
	ms = 0x00;

	errcode = fputc(ls, file);
	if (errcode != ls) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	errcode = fputc(ms, file);
	if (errcode != ms) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	ls = 0x00;
	ms = 0x00;

	errcode = fputc(ls, file);
	if (errcode != ls) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	errcode = fputc(ms, file);
	if (errcode != ms) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	ls = 0xC8;
	ms = 0x00;

	errcode = fputc(ls, file);
	if (errcode != ls) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	errcode = fputc(ms, file);
	if (errcode != ms) {
		printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
		fclose(file);
		remove(filename);
		return -2; // could not write to file
	}

	// zero bytes for 82 - 512
	for (unsigned int i = 82; i < 512; i++) {
		errcode = fputc(e, file);
		if (errcode != e) {
			printf("Error: failed to write to the file, perhaps there's not enough disk space?\n");
			fclose(file);
			remove(filename);
			return -2; // could not write to file
		}
	}

	// close the file
	fclose(file);
	return 0; // no errors
}


/*
 * End definition of helper functions
*/

/*
 * handles arguments passed to mkvmufs, creates the target file
 * and writes a blank filesystem to it. If at any point there is not enough memory for any of
 * the structures used the file is deleted and the program terminates with a negative value.
*/

int main(int argv, char ** argc) {
	// validate arguments, simple user input validation
	if (argv < 2) {
		printf("Error: not enough arguments, filename required\n");
		return -1; // invalid arguments
	}
	
	int k;
	int l = 0;
	
	// presence of forward slashes caused segfaults, dont allow those
	for (int i = 0; i < strlen(argc[1]); i++) {
		k = argc[1][i];
		if (k == '/') {
			l = l + 1;
		}
	}


	// any forward slashes should terminate mkvmufs
	if (l > 0) {
		printf("Error: \"%s\" is not a valid filename, it should not contain any '/' characters\n", argc[0]);
		return -1; // invalid arguments
	}

	int errcode = 0;

	// create the target file in this directory

	errcode = createFile(argc[1]);

	// write the user data portion
	if (errcode == 0) {
		errcode = userData(argc[1]);
	}

	// write the unused data portion
	if (errcode == 0) {
		errcode = unusedData(argc[1]);
	}

	// write the single level directory

	if (errcode == 0) {
		errcode = directory(argc[1]);
	}

	// write the FAT
	if (errcode == 0) {
		errcode = FAT(argc[1]);
	}

	// write the root
	if (errcode == 0) {
		errcode = root(argc[1]);
	}

	// return proper error code, or success
	if (errcode != 0) {
		printf("Error: either the file already existed or too much memory was allocated during creation\n");
		return errcode;
	} else {
		return 0;
	}
}

