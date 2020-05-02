
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	filesystem.c
 * @brief 	Implementation of the core file system funcionalities and auxiliary functions.
 * @date	Last revision 01/04/2020
 *
 */

#include "filesystem/filesystem.h" // Headers for the core functionality
#include "filesystem/auxiliary.h"  // Headers for auxiliary functions
#include "filesystem/metadata.h"   // Type and structure declaration of the file system
#include <string.h>
/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize) {

	// Check if the device size is between the limits
	if (deviceSize < MIN_DISK_SIZE || deviceSize > MAX_DISK_SIZE){ 
		return -1;
	}

	// Set default settings
	superblock.magic_num = 383464;
	superblock.num_inodes = 0;
	superblock.device_size = deviceSize;
	superblock.block_num = deviceSize/BLOCK_SIZE;
	
	
	// Set all inode_map bits to 0
	for (int i = 0; i < MAX_FILE_NUM; i++){
		bitmap_setbit(superblock.inode_map, i, 0); //free
	}

	// Set all block_map bits to 0
	for (int i = 0; i < MAX_BLOCK_NUM; i++) {
		bitmap_setbit(superblock.block_map, i, 0); //free
	}
	
	// Initialize all inodes to 0
	for (int i=0; i < INODES_SIZE; i++) {
        memset(&(firstInodes[i]), '\0', sizeof(inode_t) );
    }
	for (int i=0; i < INODES_SIZE; i++) {
        memset(&(secondInodes[i]), '\0', sizeof(inode_t) );
    }

	// Reset all data blocks
	char empty_block[BLOCK_SIZE];
	memset(empty_block, '\0', BLOCK_SIZE);

	for (int i = 0; i < superblock.block_num; i++) {
		if (bwrite(DEVICE_IMAGE, firstDataBlock + i, empty_block) == -1) {
			return -1;
		}
	}

	if (meta_writeToDisk() == -1){
		return -1;
	}

	return 0;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int mountFS(void) {

	if (!isMounted){
		if (meta_readFromDisk() == -1){
			return -1;
		}
		isMounted = TRUE;
	} else {
		return -1;
	}
	return 0;	
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void) {
	if (isMounted){
		if (meta_writeToDisk() == -1){
			return -1;
		}
		isMounted = FALSE;
	} else {
		return -1;
	}

	return 0;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *fileName) {
	
	// If filesystem isn't mounted return error
	if (!isMounted){
		return -2;
	}
	
	int b_id, inode_id;

	// Check if filename alredy exists
	if (name_i(fileName) != -1 ){
		return -1;
	}

	// Alloc the inode and if  there isn't 
	// enought space return -2
	inode_id = ialloc();
    if (inode_id == -1){
        return -2;
	}
    // Alloc the block and if  there isn't 
	// enought space return -2
	b_id = balloc();
    if (b_id == -1) {
		ifree(inode_id); // If there is no block space free the inode also
        return -2;
    }
	// Check that the name size is legal
	if (sizeof(fileName) > MAX_NAME_LENGHT) {return -1;}
	
	// Set default settings for the new inode
	if ((inode_id/INODES_SIZE)==0){
    	strcpy(firstInodes[inode_id].name, fileName);
		firstInodes[inode_id%INODES_SIZE].direct_block[0] = b_id;
		for (int i = 1; i < sizeof(firstInodes[inode_id%INODES_SIZE].direct_block)/4; i++){
			firstInodes[inode_id%INODES_SIZE].direct_block[i] = -1;
		}
		firstInodes[inode_id%INODES_SIZE].size = 0;
	}else {
		strcpy(secondInodes[inode_id%INODES_SIZE].name, fileName);
		secondInodes[inode_id%INODES_SIZE].direct_block[0] = b_id;
		for (int i = 1; i < sizeof(secondInodes[inode_id%INODES_SIZE].direct_block)/4; i++){
			secondInodes[inode_id%INODES_SIZE].direct_block[i] = -1;
		}
		secondInodes[inode_id%INODES_SIZE].size = 0;
	}
    
    
	// Set the offset and state in file desctiptor
	inodes_x[inode_id].offset    = 0;
    inodes_x[inode_id].state     = CLOSE;

	superblock.num_inodes++;
    return 0;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName) {
	// If filesystem isn't mounted return error
	if (!isMounted){
		return -2;
	}
	
	int inode_id;

	// Check if filename exists
	inode_id = name_i(fileName);
	if (inode_id == -1){
		return -1;
	}
	// Free the direct block
	if ((inode_id/INODES_SIZE) == 0){
		for (int i=0; i<5; i++){
			int block = firstInodes[inode_id].direct_block[i];
			if (block != -1){
				if (bfree(block) == -1) { return -2;}
			}
		}
	}else{
		for (int i=0; i<5; i++){
			int block = secondInodes[inode_id].direct_block[i];
			if (block != -1){
				if (bfree(block) == -1) { return -2;}
			}
		}
	}
	

	if (ifree(inode_id) == -1 ){ return -2;} 
	superblock.num_inodes--;
	return 0;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName) {
	// If filesystem isn't mounted return error
	if (!isMounted){
		return -2;
	}

	int inode_id;
	inode_id = name_i(fileName);
	// Check if the fileName exist
	if (inode_id == -1){
		return -1;
	}
	// Check if it's currently opened
	if (inodes_x[inode_id].state == OPEN) {
		return -2;
	}
	// Open the file, set offset to 0 and returns its
	// file descriptor id
	inodes_x[inode_id].state = OPEN;
	inodes_x[inode_id].offset = 0;
	inodes_x[inode_id].integrity = FALSE;
	return inode_id;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor){
	
	// If filesystem isn't mounted return error
	if (!isMounted){
		return -1;
	}

	if (inodes_x[fileDescriptor].integrity == TRUE) {return -1;}

	// Check if it's currently closed
	if (inodes_x[fileDescriptor].state == CLOSE){
		return -1;
	}

	//Close the file and return 0
	inodes_x[fileDescriptor].state = CLOSE;
	return 0;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes) {
	if (!isMounted) { return -1; }
	// Check that the file is legal and exists
	if (fileDescriptor > MAX_FILE_NUM || fileDescriptor < 0) { return -1;}
	if (bitmap_getbit(superblock.inode_map, fileDescriptor) == 0){ return -1; }
	// Check that numBytes has the right size
	if (numBytes < 0) { return -1; }
	if (numBytes == 0) { return 0;}
	
	int size, position = inodes_x[fileDescriptor].offset;
	int blocks, readed = 0, toread = 0;
	char b[BLOCK_SIZE];
	memset(b, '\0', BLOCK_SIZE);

	if ((fileDescriptor/INODES_SIZE)==0){
		size = firstInodes[fileDescriptor%INODES_SIZE].size;
		if (size == position){
			return 0;
		}
	}else{
		size = secondInodes[fileDescriptor%INODES_SIZE].size;
		if (size == position){
			return 0;
		}
	}

	// If the bytes to read are greater than the available bytes
	//  then read only the bytes available
	if (numBytes > size - position){
		numBytes = size - position;
	}

	blocks = ((position+numBytes)/BLOCK_SIZE) + 1; // Number of blocks where we are going to write
	
	for (int i = 0; i < blocks; i++){
		int block_id = b_map(fileDescriptor, position); // Get the number of block to read
		if (block_id == -1){ return -1; }
		
		if ((i!=blocks-1 && (blocks>1))){
			toread = BLOCK_SIZE - position%BLOCK_SIZE; // Get the size to read
		}else{
			toread = numBytes - readed; // Get the size to write
		}
		// Read the block, and append the bytes 'toread'
		// to the buffer
		if (bread(DEVICE_IMAGE, firstDataBlock + block_id, b) == -1){ return -1; };
		memmove(buffer+readed, b+position%BLOCK_SIZE, toread);
		readed += toread;
		position += toread;
	}	
	// Update offset and size
	inodes_x[fileDescriptor].offset += numBytes;

	return numBytes;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes){
	if (!isMounted) {return -1;}
	// Check that the file is legal and exists
	if (fileDescriptor > MAX_FILE_NUM || fileDescriptor < 0) {return -1;}
	if (bitmap_getbit(superblock.inode_map, fileDescriptor) == 0){return -1;}
	// Check that numBytes has the right size
	if (numBytes < 0) {return -1;}
	if (numBytes == 0 || inodes_x[fileDescriptor].offset == MAX_FILE_SIZE) {return 0;}

	int position = inodes_x[fileDescriptor].offset;
	int blocks, writed = 0;
	char b[BLOCK_SIZE];
	memset(b, '\0', BLOCK_SIZE);

	
	if (numBytes > MAX_FILE_SIZE - position){
		numBytes = MAX_FILE_SIZE - position;
	}
	blocks = ((position+numBytes)/BLOCK_SIZE) + 1; // Number of blocks where we are going to write
	for (int i = 0; i < blocks; i++){
		// Case: first block
		if (i == 0){
			int block_id = b_map(fileDescriptor, position); // Get the number of block to write
			if (block_id == -1){return -1; }
			int towrite = BLOCK_SIZE - position%BLOCK_SIZE; // Get the size to write
			// Read the block, append the first 'towrite' bytes of buffer
			// and then write it in disk
			if (bread(DEVICE_IMAGE, firstDataBlock + block_id, b) == -1){return -1;};
			memmove(b+(position%BLOCK_SIZE), buffer, towrite);
			if (bwrite(DEVICE_IMAGE, firstDataBlock +block_id, b) == -1){return -1;};
			writed += towrite;
			position += towrite;
		}
		// Case: last block
		else if (i==blocks-1){
			int block_id = b_map(fileDescriptor, position); // Get the number of block to write
			if (block_id == -1){return -1; }
			int towrite = numBytes - writed; // Get the size to write
			// Read the block append it at the begining the 
			// last 'towrite' bytes of buffer and write it on disk
			if (bread(DEVICE_IMAGE, firstDataBlock + block_id, b) == 1) { return -1;};
			memmove(b, buffer+writed, towrite);
			if (bwrite(DEVICE_IMAGE, firstDataBlock + block_id, b) == -1) {return -1;}
			writed += towrite;
			position += towrite;				
		}
		// Case: middle block
		else {
			int block_id = b_map(fileDescriptor, position); // Get the number of block to write
			if (block_id == -1){ return -1; }
			memmove(b, buffer+writed, BLOCK_SIZE);
			if (bwrite(DEVICE_IMAGE, firstDataBlock + block_id, b) == -1) {return -1;}
			writed += BLOCK_SIZE;
			position += BLOCK_SIZE;
		}
	}
	// Update offset and size
	inodes_x[fileDescriptor].offset += numBytes;
	if ((fileDescriptor/INODES_SIZE) == 0){
		firstInodes[fileDescriptor%INODES_SIZE].size += numBytes;
	}else {
		secondInodes[fileDescriptor%INODES_SIZE].size += numBytes;
	}
	return numBytes;
	
}

/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence) {
	if (!isMounted) {return -1;}
	if ( fileDescriptor < 0 || fileDescriptor > MAX_FILE_NUM) {return -1;}
	if (bitmap_getbit(superblock.inode_map, fileDescriptor) == 0) {return -1;}
	
	if (whence == FS_SEEK_BEGIN){
		inodes_x[fileDescriptor].offset = 0;
	}else if (whence == FS_SEEK_CUR){
		int newPosition = (inodes_x[fileDescriptor].offset + offset);
		if (newPosition < 0 || newPosition > MAX_FILE_SIZE){return -1;}
		inodes_x[fileDescriptor].offset = newPosition; 
	}else{
		if ((fileDescriptor/INODES_SIZE) == 0){
			inodes_x[fileDescriptor].offset = firstInodes[fileDescriptor].size;
		} else {
			inodes_x[fileDescriptor].offset = secondInodes[fileDescriptor].size;	
		}
	}
	return 0;
}

/*
 * @brief	Checks the integrity of the file.
 * @return	0 if success, -1 if the file is corrupted, -2 in case of error.
 */

int checkFile(char *fileName){
	int inode_id;
	if (!isMounted){return -2;}
	if ((inode_id = name_i(fileName))==-1) {return -2;}

	char b[BLOCK_SIZE];
	memset(b, '\0', BLOCK_SIZE);

	if ((inode_id/INODES_SIZE)==0){
		int blocks[5];
		memcpy(blocks,  firstInodes[inode_id%INODES_SIZE].direct_block, 5*sizeof(int));
		for (int i = 0; i<sizeof(blocks)/sizeof(int); i++){
			if (blocks[i] != -1){
				if(bread(DEVICE_IMAGE, firstDataBlock + blocks[i], b) == -1){return -2;}
				uint32_t expected = CRC32((const unsigned char*)b, BLOCK_SIZE);
				uint32_t got = firstInodes[inode_id%INODES_SIZE].crc[i];
				if (expected != got ){
					return -1;
				}
			}
		}
	}else{
		int blocks[5];
		memcpy(blocks,  secondInodes[inode_id%INODES_SIZE].direct_block, 5*sizeof(int));
		for (int i = 0; i<sizeof(blocks)/sizeof(int); i++){
			if (blocks[i] != -1){
				if(bread(DEVICE_IMAGE, firstDataBlock + blocks[i], b) == -1){return -2;}
				uint32_t expected = CRC32((const unsigned char*)b, BLOCK_SIZE);
				uint32_t got = secondInodes[inode_id%INODES_SIZE].crc[i];
				if (expected != got ){
					return -1;
				}
			}
		}
	}
	return 0;
}

/*
 * @brief	Include integrity on a file.
 * @return	0 if success, -1 if the file does not exists, -2 in case of error.
 */

int includeIntegrity(char *fileName) {
	int inode_id;
	if (!isMounted){return -2;}
	if ((inode_id = name_i(fileName))==-1) {return -1;}

	char b[BLOCK_SIZE];
	if ((inode_id/INODES_SIZE)==0){
		int blocks[5];
		memcpy(blocks,  firstInodes[inode_id%INODES_SIZE].direct_block, 5*sizeof(int));
		for (int i = 0; i<sizeof(blocks)/sizeof(int); i++){
			if (blocks[i] != -1){
				if(bread(DEVICE_IMAGE, firstDataBlock + blocks[i], b)){return -2;};
				uint32_t crc = CRC32((const unsigned char*)b, BLOCK_SIZE);
				firstInodes[inode_id%INODES_SIZE].crc[i] = crc;
			}
		}
	}else{
		int blocks[5];
		memcpy(blocks,  secondInodes[inode_id%INODES_SIZE].direct_block, 5*sizeof(int));
		for (int i = 0; i<sizeof(blocks)/sizeof(int); i++){
			if (blocks[i] != -1){
				if(bread(DEVICE_IMAGE, firstDataBlock + blocks[i], b)){return -2;};
				uint32_t crc = CRC32((const unsigned char*)b, BLOCK_SIZE);
				secondInodes[inode_id%INODES_SIZE].crc[i] = crc;
			}
		}
	}
	return 0;
}

/*
 * @brief	Opens an existing file and checks its integrity
 * @return	The file descriptor if possible, -1 if file does not exist, -2 if the file is corrupted, -3 in case of error
 */
int openFileIntegrity(char *fileName){
	int inode_id;
	if (!isMounted){return -3;} //Error
	if ((inode_id = name_i(fileName))==-1) {return -1;} //File doesn't exist
	
	int err = checkFile(fileName);
	if (err == -2) {return -3;} 	 // Error 
	else if (err == -1) {return -2;} //File is corrupted
	
	err = openFile(fileName);
	inodes_x[inode_id].integrity = TRUE;
	if (err==-2) {return -3;}
	return err;
}

/*
 * @brief	Closes a file and updates its integrity.
 * @return	0 if success, -1 otherwise.
 */
int closeFileIntegrity(int fileDescriptor) {
	int err;
	if (!isMounted){return -3;} //Error

	if (inodes_x[fileDescriptor].integrity == FALSE) {return -1;}
	
	if ((fileDescriptor/INODES_SIZE) == 0) {
		err = includeIntegrity(firstInodes[fileDescriptor%INODES_SIZE].name);
		if (err < 0) {return -1;} 	 // Error 
	}else{
		err = includeIntegrity(secondInodes[fileDescriptor%INODES_SIZE].name);
		if (err < 0) {return -1;} 	 // Error 
	}	
	
	err = closeFile(fileDescriptor);
	if (err==-1) {return -1;}
	return err;
}

/*
 * @brief	Creates a symbolic link to an existing file in the file system.
 * @return	0 if success, -1 if file does not exist, -2 in case of error.
 */
int createLn(char *fileName, char *linkName){
	return -1;
}

/*
 * @brief 	Deletes an existing symbolic link
 * @return 	0 if the file is correct, -1 if the symbolic link does not exist, -2 in case of error.
 */
int removeLn(char *linkName)
{
	return -2;
}

/*------------ Auxiliar functions ---------------------*/

/*
 * @brief 	Allocates a inode in memory
 * @return 	Position if success, -1 otherwise.
 */
int ialloc(void){

	// search for the first free inode
	for (int i = 0; i < MAX_FILE_NUM; i++){
		if (bitmap_getbit(superblock.inode_map, i) == 0) {
			bitmap_setbit(superblock.inode_map, i, 1); // Set it as occupied
			// Check if the first free inode it's in first inode block
			if ((i/INODES_SIZE) == 0){
				memset(&(firstInodes[i%INODES_SIZE]), '\0', sizeof(inode_t));	
			}else{
				memset(&(secondInodes[i%INODES_SIZE]), '\0', sizeof(inode_t));	
			}
			// We return it's position
			return i;
		}
	}
	// Return -1 if not found
	return -1;
}

/*
 * @brief 	Allocates a block in disk
 * @return 	Position if success, -1 otherwise.
 */
int balloc(void){

	// We define a dummy block for writing in memory
	char b[BLOCK_SIZE];

	// Search for first free block
	for (int i = 0; i < superblock.block_num; ++i) {
		if (bitmap_getbit(superblock.block_map, i) == 0){
			bitmap_setbit(superblock.block_map, i, 1); // Set it as occupied

			// We write the dummy block
			memset(b, '\0', BLOCK_SIZE);
			bwrite(DEVICE_IMAGE, firstDataBlock + i, b);

			// We return it's position
			return i;
		}
	}
	// Return -1 if not found
	return -1;
}

/*
 * @brief 	Free a inode in memory
 * @return 	0 if success, -1 otherwise.
 */
int ifree(int inode_id) {
	// Check that inode_id is a legal and non-free id
	if (inode_id > MAX_FILE_NUM){ return -1; } 
	if (bitmap_getbit(superblock.inode_map, inode_id) == 0){
		return -1;
	}

	// free inode
	bitmap_setbit(superblock.inode_map, inode_id, 0);
	//Set inode to 0
	if ( (inode_id/INODES_SIZE) == 0) { 
		memset(&(firstInodes[inode_id%INODES_SIZE]), '\0', sizeof(inode_t));	
	} else {
		memset(&(secondInodes[inode_id%INODES_SIZE]), '\0', sizeof(inode_t));	
	}
	
	return 0;
}

/*
 * @brief 	Free a block in memory
 * @return 	0 if success, -1 otherwise.
 */
int bfree(int block_id){
	// Check that inode_id is a legal and non-free id
	if (block_id > superblock.block_num) { return -1; }
	if (bitmap_getbit(superblock.block_map, block_id) == 0){
		return -1;
	}

	// free the bit in the bitmap
	bitmap_setbit(superblock.block_map, block_id, 0);
	
	// free the block in memory
	char b[BLOCK_SIZE]; // dummy block for reseting 
	memset(b, '\0', BLOCK_SIZE);
	bwrite(DEVICE_IMAGE, firstDataBlock + block_id, b);
	return 0;
}

/*
 * @brief 	Search for a inode with name 'fname'
 * @return 	inode id if success, -1 otherwise.
 */
int name_i(char *fname){

	// search an i-node with name <fname> in the first inode block
	for (int i = 0; i < INODES_SIZE; i++){
		if (!strcmp(firstInodes[i%INODES_SIZE].name, fname)){
			//Return de inode id
			return i;
		}
	}
	// search an i-node with name <fname> in the second inode block
	for (int i = 0; i < INODES_SIZE; i++){
		if (!strcmp(secondInodes[i%INODES_SIZE].name, fname)){
			//Return de inode id
			return i;
		}
	}

	//Return -1 if not found
	return -1;
}

/*
 * @brief 	Gives the datablock where the file with offset is
 * @return 	block id if success, -1 otherwise.
 */
int b_map(int inode_id, int offset) {

	// Check that the inode_id is legal and non-free
	if (inode_id>MAX_FILE_NUM) {return -1;}
	if (bitmap_getbit(superblock.inode_map, inode_id) == 0){
		return -1;
	}
	
	if ( (inode_id/INODES_SIZE) == 0 ){
		
		int block = offset/BLOCK_SIZE; // Calculate the block of this offset
		// Check if it's alredy initializated and it not
		// make a balloc, finally return the block number
		if (firstInodes[inode_id%INODES_SIZE].direct_block[block] == -1 ){
			int block_id = balloc();
			firstInodes[inode_id%INODES_SIZE].direct_block[block] = block_id;
		}
		return firstInodes[inode_id%INODES_SIZE].direct_block[block];
	}else{

		int block = offset/BLOCK_SIZE; // Calculate the block of this offset
		// Check if it's alredy initializated and it not
		// make a balloc, finally return the block number
		if (secondInodes[inode_id%INODES_SIZE].direct_block[block] == -1 ){
			int block_id = balloc();
			secondInodes[inode_id%INODES_SIZE].direct_block[block] = block_id;
		}
		return secondInodes[inode_id%INODES_SIZE].direct_block[block];
	}

	return -1;
}

/*
 * @brief 	Read metadata from disk to memory
 * @return 	0 if success, -1 otherwise.
 */
int meta_readFromDisk(void){

	char b[BLOCK_SIZE];

	// Read the superblock from disk to memory
	if (bread(DEVICE_IMAGE, SuperBlock_Block, b) == -1){return -1;}
	memcpy((char*)&superblock, b, BLOCK_SIZE);

	// Read the frist 24 inodes from disk to memory
	if (bread(DEVICE_IMAGE, firstInodes_Block, b) == -1){return -1;}
	memcpy((char*)&firstInodes, b, BLOCK_SIZE);

	// Read the second 24 inodes from disk to memory
	if (bread(DEVICE_IMAGE, secondInodes_Block, b) == -1){return -1;}
	memcpy((char*)&secondInodes, b, BLOCK_SIZE);

	return 0;
}

/*
 * @brief 	Write metadata from memory to disk
 * @return 	0 if success, -1 otherwise.
 */
int meta_writeToDisk(void){

	// write in disk the superblock
	char buff[BLOCK_SIZE];
	memset(buff, '\0', BLOCK_SIZE);
	memcpy(buff, (char*) &superblock, sizeof(superblock));
	if (bwrite(DEVICE_IMAGE, 0, buff) == -1) { return -1; }

	// Write the first 24 inodes from memory to disk
	memset(buff, '\0', BLOCK_SIZE);
	memcpy(buff, (char*)&firstInodes, BLOCK_SIZE);
	if (bwrite(DEVICE_IMAGE, firstInodes_Block, buff)){return -1;}

	// Write the second 24 inodes from memory to disk
	memset(buff, '\0', BLOCK_SIZE);
	memcpy(buff, (char*)&secondInodes, BLOCK_SIZE);
	if (bwrite(DEVICE_IMAGE, secondInodes_Block, buff)){return -1;}	

	return 0;
}

