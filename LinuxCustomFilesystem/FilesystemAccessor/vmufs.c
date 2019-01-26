/*
 * File containing definitions of fuse related functions to mount the target file system
 * at the target location. This version of vmufs.c only supports read functionality.
 *
 * compiled with:
 * gcc -Wall vmufs.c `pkg-config fuse --cflags --libs` -o vmufs
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

/*
 * BEGIN HELPER FUNCTIONS
*/

// dirbuff holds 200 32-byte values that each represent an entry in the directory,
// if an entire entry is nothing but zero bytes, then it is a blank directory entry
// which is to say there is not a file there.
unsigned char *dirbuff[200];

// FAT info for a given real entry of dirbuff, contains other useful information like
// the seek location of where the file was open last.
struct FAT_info {
	// the FAT chain of the file, this begins with the first block of the file and
	// continues with all allocated locations of file blocks until -1. there are at
	// most 200 blocks allocated to a given file, so it is an integer array of size
	// 200.
	// 
	// For example, a file beginning at block 0 with three blocks of data might have
	// the FAT_chain = {0, 1, 2, -1, ... , -1}
	int FAT_chain[200];

	// size of the file in blocks
	int fileBlocks;

	// size of the file in bytes
	int fileBytes;

	// byte location of where reading should begin
	int fileSeek;
};

// array of pointers to FAT_info structs, I don't allocate memory to a given entry
// in the directory unless it actually exists.
struct FAT_info *dirFAT[200];

// absolute path of vmufs file image, PATH_MAX is from limits.h, defined in main()
static char *realPath;


/*
 * read the entry at the given block and store relavent info in given buffers. 
 * returns the size of the file in blocks upon success, or an appropriate 
 * negative value on failure.
 *
 * fileName: is a character buffer that would be passed to store the name of
 * the file.
 *
 * fileSize: is a integer buffer that would be passed to store the size of the
 * file in bytes (number of blocks * 512). as far as I'm concerned empty files
 * are valid, so 0 byte files are possible.
 *
 * fileLoc: is a integer buffer that would be passed to store the location of
 * the first block of the file, this should be between 0-240 for data.
*/
int readEntry(int block, unsigned char * fileName, unsigned int * fileBytes, unsigned int * fileLoc) {

	// The directory buffer is an array of unsigned characters that make
	// up the directory entry for a given block x of the directory.
	
	// The structure of a directory block (32 bytes) is as follows:
	// offset, dirbuff[x][n], size, description
	// 0x00	0 1 File Type (unimportant for now)
	// 0x01	1 1 Copy Protection (unimportant)
	// 0x02	2,3 2 First Block
	// 0x04	4-15 12 Filename (ASCII)
	// 0x10	16-23 8 Timestamp (unimportant for now)
	// 0x18	24,25 2 File size
	// 0x1A	26,27 2 Header offset (unimportant for now)
	// 0x1C	28-31 4 Unused (always 0 bytes)

	// readdir not called yet, file can't exist yet
	if(dirbuff[0] == NULL) {
		return -ENOENT;
	}

	if(block > 200 || block < 0) {
		return -1; // given entry is not valid
	}

	// as far as I'm concerned, all files must have names, if a file
	// does not have a name then I assume that no entry exists at that
	// block of the directory
	if(dirbuff[block][4] == 0) {
		return -ENOENT; // given entry is not valid
	}

	for(int i = 4; i < 16; i++) {
		// copy the filename into the given buffer, not null terminated
		// by this function if the length is 12. it will be null terminated
		// if the name contains less than 12 chars as it will then contain
		// a zero byte and a zero byte is a null terminator.
		fileName[i-4] = dirbuff[block][i];
	}

	unsigned int fileSize;
	fileSize = 0; // zero out all the bits

	// least sig. bits are read first, most sig. second
	unsigned char lsb = dirbuff[block][24];
	unsigned char msb = dirbuff[block][25];
	
	// piece the 8 bit ints together to form a 16 bit integer
	fileSize  = msb << 8;
	fileSize |= lsb     ;

	// number of bytes is block size * 512
	*fileBytes = fileSize * 512;

	// next few lines are the same as above but for fileLoc instead of size

	unsigned int fileBlock;
	fileBlock = 0;

	lsb = dirbuff[block][2];
	msb = dirbuff[block][3];

	fileBlock  = msb << 8;
	fileBlock |= lsb     ;

	*fileLoc = fileBlock;

	return fileSize;
}

/*
 * read the entry at the given filename, store relavent info in given buffers. 
 * returns the size of the file in blocks upon success, or an appropriate 
 * negative value on failure.
 *
 * path: path of the file in question
 *
 * fileSize: is a integer buffer that would be passed to store the size of the
 * file in bytes (number of blocks * 512). as far as I'm concerned empty files
 * are valid, so 0 byte files are possible.
 *
 * fileLoc: is a integer buffer that would be passed to store the location of
 * the first block of the file, this should be between 0-240 for data.
 *
 * block: location of the file in dirbuff, useful for future calls to readEntry()
 * or other accesses to the dirFAT.
*/
int readEntryName(const char * path, unsigned int * fileBytes, unsigned int * fileLoc, unsigned int * block) {
	// only difference between this and readEntry is that I have to actually
	// search for the entry in the directory buffer rather than go straight
	// to it. being able to search dirbuff by path is convenient for fuse.
	
	// The directory buffer is an array of unsigned characters that make
	// up the directory entry for a given block x of the directory.
	
	// The structure of a directory block (32 bytes) is as follows:
	// offset, dirbuff[x][n], size, description
	// 0x00	0 1 File Type (unimportant for now)
	// 0x01	1 1 Copy Protection (unimportant)
	// 0x02	2,3 2 First Block
	// 0x04	4-15 12 Filename (ASCII)
	// 0x10	16-23 8 Timestamp (unimportant for now)
	// 0x18	24,25 2 File size
	// 0x1A	26,27 2 Header offset (unimportant for now)
	// 0x1C	28-31 4 Unused (always 0 bytes)

	FILE * fp;
	fp = fopen("output.txt", "a+");
	fprintf(fp, "		readEntryName: path = %s\n", path);
	fclose(fp);

	// readdir not called yet, file can't exist yet
	if(dirbuff[0] == NULL) {
		return -ENOENT;
	}

	// file names should be at most 12 characters (excluding the '/')
	if(strlen(path) > 13) {

		fp = fopen("output.txt", "a+");
		fprintf(fp, "			readEntryName: -NOENT\n");
		fclose(fp);
		return -ENOENT; // ENOENT
	}

	// copy path into pathName excluding the '/'
	unsigned char pathName[13];
	pathName[12] = '\0'; // null terminate just in case
	for(int i = 0; i < strlen(path) - 1; i++) {
		pathName[i] = (unsigned char)path[i+1];
	}

	int where; // actual entry in the dirbuff
	int found; // boolean value whether or not the entry was found

	// perform linear search of directory entries, this is inefficient
	// but there aren't really that many to begin with so its OK for
	// only a couple thousand bytes.
	for(int i = 0; i < 200; i++) {
		// get the fileName at this dirbuff entry
		unsigned char fileName[13];
		fileName[12] = '\0'; // null terminate just in case
		for(int j = 4; j < 16; j++) {
			// will be null terminated if dirbuff file name contains
			// zero bytes
			fileName[j-4] = dirbuff[i][j];
		}	

		// check the file name against the path given, if they are equal
		// then the entry was found, if the for loop terminates and no
		// equal was found then the entry is not contained in dirbuff
		if(strcmp(pathName, fileName) == 0) {
			// entry found, print to output.txt
			fp = fopen("output.txt", "a+");
			fprintf(fp, "			readEntryName: %s == %s\n", pathName, fileName);
			fclose(fp);
			where = i; // exact entry in dirbuff, could be useful later
			found = 1;
			i = 200; // stop the for loop
		} else {
			// not found yet print to output.txt
			found = 0;
			fp = fopen("output.txt", "a+");
			fprintf(fp, "			readEntryName: %s != %s\n", pathName, fileName);
			fclose(fp);
		}
	}

	// if the entry wasn't found, return appropriate error
	if(found == 0) {
		fp = fopen("output.txt", "a+");
		fprintf(fp, "			readEntryName: -NOENT\n");
		fclose(fp);
		return -ENOENT; // ENOENT
	}

	// set block
	*block = where;
	
	// perform regular readEntry functionality
	
	unsigned int fileSize;
	fileSize = 0; // zero out all the bits

	unsigned char lsb = dirbuff[where][24];
	unsigned char msb = dirbuff[where][25];
	
	fileSize  = msb << 8;
	fileSize |= lsb     ;

	*fileBytes = fileSize * 512;

	unsigned int fileBlock;
	fileBlock = 0;

	lsb = dirbuff[where][2];
	msb = dirbuff[where][3];

	fileBlock  = msb << 8;
	fileBlock |= lsb     ;

	*fileLoc = fileBlock;

	return fileSize;
}


/*
 * Populate the directory buffer starting from the first 32-byte block of the
 * directory on down. If the file at the given path does not have a correct
 * directory format then return an appropriate error number. return 0 upon
 * success.
 *
 * After the directory buffer is populated, populate dirFAT but only for files
 * that are real. (whereas dirbuff contains entries for files whether or not
 * their entry is all zero bytes, dirFAT is a little more picky as it requires
 * a lot more memory).
*/
int readDirectory() {
	// want the most up to date data, if dirbuff and dirFAT contain anything,
	// go ahead and free them.
	for(int i = 0; i < 200; i++) {
		if(dirbuff[i] != NULL) {
			free(dirbuff[i]);
		}

		if(dirFAT[i] != NULL) {
			free(dirFAT[i]);
		}
	}

	// don't let anything touch global variable realPath
	char realPathCopy[PATH_MAX+1];
	strcpy(realPathCopy, realPath);

	FILE *ofp;
	ofp = fopen("output.txt", "a+");
	fprintf(ofp, "	readDirectory: realPathCopy = %s\n", realPathCopy);
	fclose(ofp);

	FILE *fp;
	fp = fopen(realPathCopy, "r");

	if(fp == NULL) {
		ofp = fopen("output.txt", "a+");
		fprintf(ofp, "	readDirectory: realPathCopy = %s\n", realPathCopy);
		fclose(ofp);

		printf("	readDirectory: File does not exist\n");
		fclose(fp);
		return -ENOENT; // file does not exist
	} 

	int err; // return value of fseek
	size_t readErr; // return value of fread
	
	// make sure that the file given is the correct size by going to the
	// end, seeing how many bytes forward we are, and then going back when
	// finished
	fseek(fp, 0L, SEEK_END);

	err = ftell(fp);

	if(err != 131072) {
		fclose(fp);
		printf("	readDirectory: File is of incorrect format\n");
		return -EINVAL; // invalid file size, not 131072 bytes
	}

	fseek(fp, 0L, SEEK_SET);

	

	// keep track of which blocks have been read in
	int blockCounter = 253;

	// keep track of where to put in the 32 byte strings
	int bufferCounter = 0;

	// read the directory blocks in order from first to last so from 253-241
	// keeping in mind that the last 8*32 bytes of the last block are not used.
	for (blockCounter; blockCounter > 239; blockCounter--) {
		err = fseek(fp, 512*blockCounter, SEEK_SET);

		if(err < 0) {
			printf("	readDirectory: File is of incorrect format\n");
			fclose(fp);
			return -EINVAL; // file is of incorrect format, perhaps too small
		}

		// for block 241, only expect 8 entries instead of 16
		if(blockCounter == 240) {
			for(int i = 0; i < 8; i++) {
				unsigned char * directoryEntry = malloc(sizeof(unsigned char) * 32);

				if(directoryEntry == NULL) {
					for(int k = 0; k < bufferCounter; k++) {
						free(dirbuff[k]); // free already allocated space
					}
					fclose(fp);
					return -ENOMEM; // not enough memory to fill dirbuff
				}				

				readErr = fread(directoryEntry, sizeof(unsigned char), 32, fp);
				dirbuff[bufferCounter] = directoryEntry;
				bufferCounter++;

				if(readErr < 0) {
					fclose(fp);
					return -EINVAL; // file is of incorrect format, perhaps too small
				}
			}
		} else {
			for(int i = 0; i < 16; i++) {
				unsigned char * directoryEntry = malloc(sizeof(unsigned char) * 32);

				if(directoryEntry == NULL) {
					for(int k = 0; k < bufferCounter; k++) {
						free(dirbuff[k]); // free already allocated space
					}
					printf("	readDirectory: Not enough memory to fill dirbuff\n");
					fclose(fp);
					return -ENOMEM; // not enough memory to fill dirbuff
				}				

				readErr = fread(directoryEntry, sizeof(unsigned char), 32, fp);
				dirbuff[bufferCounter] = directoryEntry;
				bufferCounter++;

				if(readErr < 0) {
					printf("	readDirectory: File is of incorrect format\n");
					fclose(fp);
					return -EINVAL; // file is of incorrect format, perhaps too small
				}
			}
		}
	}

	fclose(fp);

	// dirbuff successfully filled, attempt to fill dirFAT

	// dirbuff[block] and dirFAT[block] correspond to the same directory entry where
	// block is some directory entry from 0-199. so I fill dirFAT in much the same way
	// as dirbuff.
	for(int i = 0; i < 200; i++) {
		unsigned int block; // first block of the file
		int blockSize; // how many blocks the directory entry has
		unsigned int byteSize; // how many bytes the directory entry has
		unsigned char name[13]; // name of the directory entry, not used.
		name[12] = '\0';

		blockSize = readEntry(i, name, &byteSize, &block);

		// only consider making dirFAT entries for real files, dirFAT entries are quite
		// large in size.
		if(blockSize >= 0) {
			if(blockSize == 0) {
				// don't need to follow the FAT chain, there isn't one for an empty file
				dirFAT[i] = malloc(sizeof(struct FAT_info));
				dirFAT[i]->FAT_chain[0] = -1; // first entry is -1, no blocks in the
				                              // FAT
				dirFAT[i]->fileSeek = 0; // empty file
				dirFAT[i]->fileBlocks = 0; // empty file
				dirFAT[i]->fileBytes = 0; // empty file
			} else {
				// need to follow the FAT chain starting at the first block of the
				// file in the FAT and ending at the end of chain marker.

				strcpy(realPathCopy, realPath);
				fp = fopen(realPathCopy, "r");
				

				dirFAT[i] = malloc(sizeof(struct FAT_info));
				
				if(dirFAT[i] == NULL) {
					fclose(fp);
					printf("	readDirectory: Not enough memory to fill dirFAT\n");
					return -ENOMEM;
				}

				dirFAT[i]->FAT_chain[0] = block; // first block in var block
				dirFAT[i]->fileSeek = 0;
				dirFAT[i]->fileBlocks = blockSize;
				dirFAT[i]->fileBytes = byteSize;
				
				int blockCounter = 1; // already logged the first block
				unsigned int nextBlock = block; // next block is the first in the chain
				                                // to begin with		
				
				for(int j = 1; j < blockSize+1; j++) {
					size_t readErr; // for fread
					int err; // for fseek

					// go to the beginning of the file allocation table
					err = fseek(fp, 512*254, SEEK_SET);

					// go to the next block in our FAT chain according to the value
					// at the last FAT entry (multiplied by 2 because that's how many
					// bytes I need to offset)
					err = fseek(fp, nextBlock*2, SEEK_CUR);

					unsigned char lsb[1]; // least sig. bits of the next block
					unsigned char msb[1]; // most sig. bits of the next block

					readErr = fread(lsb, sizeof(unsigned char), 1, fp);
					readErr = fread(msb, sizeof(unsigned char), 1, fp);

					nextBlock = 0; // zero out the next block
					
					nextBlock  = msb[0] << 8; // msb first
					nextBlock |= lsb[0]     ; // lsb second

					if(nextBlock < 240)
						dirFAT[i]->FAT_chain[j] = nextBlock;

					// a block has been allocated to the file that couldn't possibly
					// belong to it.
					if(nextBlock > 240 && nextBlock <= 255) {
						fclose(fp);
						printf("	readDirectory: File is of incorrect format\n");
						return -EINVAL; // incorrect format image file
					}

					// a block has been allocated to the file that is actually free
					// this means that our image file has an incorrect format.
					if(nextBlock == 0xfffc) {
						fclose(fp);
						printf("	readDirectory: File is of incorrect format\n");
						return -EINVAL; // incorrect format image file
					}

					// end of chain marker found, this is the last block allocated
					// to the file.
					if(nextBlock == 0xfffa && j != blockSize) {
						// the directory says the file is bigger than it is						
						fclose(fp);
						printf("	readDirectory: File is of incorrect format\n");
						return -EINVAL; // incorrect format image file
					}
	
					// read in as many blocks as expected, but still never found
					// end of chain marker, the file is bigger than the directory
					// says it is
					if(j == blockSize && nextBlock != 0xfffa) {
						fclose(fp);
						printf("	readDirectory: File is of incorrect format\n");
						return -EINVAL; // incorrect format image file
					}
				}

				dirFAT[i]->FAT_chain[blockSize] = -1; // last block in the chain
				                                      // is before this one
				
				fclose(fp); // FAT chain built and placed in dirFAT for block entry
				            // i
			}
		}
	}

	return 0; // success, dirbuff and dirFAT populated for the current directory state
}

/*
 * END HELPER FUNCTIONS
*/

/*
 * BEGIN FUSE RELATED FUNCTIONS
*/

/*
 * Same definition as the one used in hello.c
*/
int vmu_getattr(const char *path, struct stat *stbuf) {
	int err = 0; // return code

	FILE * fp;
	fp = fopen("output.txt", "a+");
	fprintf(fp, "vmu_getattr: %s\n", path);
	fclose(fp);
	
	if(strcmp(path, "/") == 0) {
		// this directory or the parent directory
		fp = fopen("output.txt", "a+");
		fprintf(fp, "	first condition\n");
		fclose(fp);

		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strlen(path) > 1 && path[0] == '/') {
		// object potentially contained inside of the directory
		fp = fopen("output.txt", "a+");
		fprintf(fp, "	second condition\n");
		fclose(fp);

		int err; // return values
		unsigned int dirBuffBlock; // entry number in the dirbuff
		unsigned int fileBytes; // number of bytes in the file (multiple of 512)
		unsigned int fileBlock; // first block of the file in the volume

		// translate path to directory buffer, see if the file is located in
		// the directory buffer. If it is this returns the number of blocks
		// used by the file.
		err = readEntryName(path, &fileBytes, &fileBlock, &dirBuffBlock);

		if(err < 0) {
			return -ENOENT; // readEntryName could not find the file at path
			                // within the directory buffer
		}

		stbuf->st_mode = S_IFREG | 0444; // regular file for now
		stbuf->st_nlink = 1; // assume only one hard link to the file
		stbuf->st_size = fileBytes; // multiple of 512
	} else {
		// object is neither a file that could be contained in our directory
		// and is not our directory or the parent directory.
		fp = fopen("output.txt", "a+");
		fprintf(fp, "	third condition\n");
		fclose(fp);

		err = -ENOENT; // file does not exist
	}

	return err;
}

/*
 * Read in the directory, perform some validation, update my own directory buffer located in
 * dirbuff, and use the filler function to update the contents of the directory.
*/
int vmu_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	int err;

	FILE * fp;
	fp = fopen("output.txt", "a+");
	fprintf(fp, "vmu_readdir: %s\n", path);
	fclose(fp);

	// fill the table containing all of the directory entries, this fills a 200x32 byte
	// array of character pointers containing verbatem directory entries from the directory
	// in the vmufs, file is the absolute path of the image file given when running the fuse
	// client, if an absolute path is not given, I try to generate one. It's best to just
	// use an absolute path.
	err = readDirectory();
	
	if(err < 0) {
		return err; // see readDirectory for all possible errors returned
	}

	unsigned char fileName[13]; // null terminated file name, at most 12 characters.
	fileName[12] = '\0'; // add a null terminator to the end, good if name is length 12
	unsigned int fileBytes; // size of the file in bytes (divide by 512 for number
	                        // of blocks used by this file on the volume)
	unsigned int firstBlock; // first block of the file in the volume

	filler(buf, ".", NULL, 0); // entry for this directory
	filler(buf, "..", NULL, 0); // entry for the parent directory

	for(int i = 0; i < 200; i++) {
		// check every directory entry found to see if it is a valid file, as in
		// all other functions, I check the filename to see if a file is valid.
		// files with a name containing all zero bytes are deemed invalid files
		// and are not mounted to my directory. here checking to see if the first
		// byte is zero seems sufficient to make my case that a file contains no
		// name.
		err = readEntry(i, fileName, &fileBytes, &firstBlock);
		
		// readEntry returns block size of the file if it is valid, negative values
		// for invalid files. (block size of zero is OK)
		if(err >= 0) {
			filler(buf, fileName, NULL, 0);
		}				
	}

	return 0; // success
}

/*
 * Same definition as the one used in hello.c
*/
int vmu_open(const char *path, struct fuse_file_info *fi) {
	FILE * fp;
	fp = fopen("output.txt", "a+");
	fprintf(fp, "vmu_open %s\n", path);
	fclose(fp);

	// started using popen() and passing a file descriptor with fi,
	// but that seemed rather dangerous, so instead I just use the
	// open call to make sure the file actually exists.
	
	// required arguments that aren't actually used within the scope
	// of this function.
	unsigned int byteSize;
	unsigned int blockSize;
	unsigned int firstBlock;
	unsigned int block;

	blockSize = readEntryName(path, &byteSize, &firstBlock, &block);
	if(blockSize < 0) {
		return -ENOENT; // file doesn't exist.
	}

	return 0; // file exists, continue with IO
}

/*
 * Very similar to hello.c, instead of trying to deal with file handlers
 * I just copy the entire file in the vmufs to local memory and then move
 * that into the buffer. 
 *
 * The return of this function is min(buffersize, fileSize - offset)
*/
int vmu_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	FILE * ofp;
	ofp = fopen("output.txt", "a+");
	fprintf(ofp, "vmu_read: path = %s\n", path);
	fprintf(ofp, "vmu_read: size = %i\n", size);
	fprintf(ofp, "vmu_read: offset = %i\n", offset);
	fclose(ofp);

	unsigned int byteSize;
	unsigned int blockSize;
	unsigned int firstBlock;
	unsigned int block;	
	blockSize = readEntryName(path, &byteSize, &firstBlock, &block);
	
	// already validated this earlier, but do it again
	if(blockSize < 0) {
		return -ENOENT; // somethings wrong, io error.
	}

	if(blockSize > 0) {
		// for now, data will contain the entire file.
		char * data = malloc(sizeof(unsigned char) * byteSize);

		if(data == NULL) {
			return -ENOMEM; // not enough memory to copy the entire file into
			                // a single variable.
		}
	
		FILE * fp;
		fp = fopen(realPath, "r");

		int err;
		size_t readErr;
	
		// read a block at a time into data, use the FAT to figure out where
		// each block is located.
		for(int i = 0; i < blockSize; i++) {
			err = fseek(fp, 512*dirFAT[block]->FAT_chain[i], SEEK_SET);

			if(err < 0) {
				free(data);
				fclose(fp);
				return -EIO; // somethings wrong, io error
			}

			readErr = fread(data + (512 * i), sizeof(unsigned char), 512, fp);

			if(readErr < 0) {
				free(data);
				fclose(fp);
				return -EIO; // somethings wrong, io error
			}
		}

		fclose(fp); // done reading, close the file

		unsigned char dataPrint[9];
		dataPrint[8] = '\0';

		memcpy(dataPrint, data, 8);		

		ofp = fopen("output.txt", "a+");
		fprintf(ofp, "	vmu_read: data[0-8] = %s\n", dataPrint);
		fclose(ofp);

		if( (int)size > byteSize-offset ) {
			// all of the data can fit in the buffer, and the buffer isn't full
			// size (third param) should be min(byeSize-offset, size)
			ofp = fopen("output.txt", "a+");
			fprintf(ofp, "	vmu_read: copying into a buffer that's too large\n");
			fclose(ofp);
	
			int length = byteSize - offset;
			memmove(buf, data+offset, byteSize-offset);

			memcpy(dataPrint, buf, 8);
			ofp = fopen("output.txt", "a+");
			fprintf(ofp, "	vmu_read: buf[0-8] = %s\n", dataPrint);
			fclose(ofp);

			free(data);
			return length;
		} else {
			// either all of the data fits perfectly, or theres not enough of
			// a buffer for all of the data
						
			ofp = fopen("output.txt", "a+");
			fprintf(ofp, "	vmu_read: copying into a buffer that's too small\n");
			fclose(ofp);

			memmove(buf, data+offset, size);
			
			memcpy(dataPrint, buf, 8);
			ofp = fopen("output.txt", "a+");
			fprintf(ofp, "	vmu_read: buf[0-8] = %s\n", dataPrint);
			fclose(ofp);

			free(data);
			return size;
		}
	}

	return 0; // file is empty, buffer is not altered, 0 bytes returned

}

struct fuse_operations vmu_oper = {
	.getattr = vmu_getattr,
	.readdir = vmu_readdir,
	.open = vmu_open,
	.read = vmu_read
};

/*
 * END FUSE RELATED FUNCTIONS
*/

/*
 * Main function expects vmufs to be ran with at least an image file and mount point
 * where the image file is a VMUFS in the current working directory and mount is the name
 * of a single level directory where the files located in the VMUFS should be placed.
*/
int main(int argc, char * argv[]) {
	// clear garbage from last run in output.txt
	// all errors go to output.txt as its easier to read them there than in debug mode
	// note that my error tracking with output.txt DOES NOT occur when fuse is not run
	// with the -f and -d flags.
	FILE *fp;
	fp = fopen("output.txt", "w+");
	fprintf(fp, "");

	char * imageFile = argv[1]; // the vmufs file path, absolute paths work best.

	// make sure that the file is OK
	fp = fopen(imageFile, "r");
	if(fp == NULL) {
		printf("vmufs: can't open the given file \"%s\", make sure it exists.\n", imageFile);
		printf("vmufs: absolute paths work best.\n");
	}

	char absolutePath[PATH_MAX+1];
	
	// the file is OK, so try to get its real path and store it in absolutePath
	if(realpath(imageFile, absolutePath) == NULL) {
		printf("vmufs: can't derive a real path for given file \"%s\"\n", imageFile);
		printf("vmufs: absolute paths work best.\n");
	}

	// global realPath points to absolutePath
	realPath = absolutePath;

	// fuse freaks out when it gets too many args, so take the file out
	// of the args and then reassemble fuse args.
	char * fuse_args[argc-1];
	fuse_args[0] = argv[0];
	fprintf(fp, "main: %s\n", fuse_args[0]);
	for(int i = 2; i < argc; i++) {
		fuse_args[i-1] = argv[i];
		fprintf(fp, "main: %s\n", fuse_args[i-1]);
	}
	fclose(fp);

	return fuse_main(argc-1, fuse_args, &vmu_oper, NULL);

	// I'm not quite sure how to free my dynamically allocated dirbuff and dirFAT
	// so memory leaks are not only expected but guaranteed.
}
