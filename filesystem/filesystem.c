
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
	
	superblock.FirstDaBlock = firstDataBlock;
	superblock.Indirect_block = IndirectBlock_block;
	
	// Set all inode_mat bits to 0
	for (int i = 0; i < MAX_FILE_NUM; i++){
		bitmat_setbit(superblock.inode_map, i, 0); //free
	}

	// Set all block_map bits to 0
	for (int i = 0; i < MAX_BLOCK_NUM; i++) {
		bitmat_setbit(superblock.block_map, i, 0); //free
	}

	// Initialize all inodes to 0
	for (int i=0; i < MAX_FILE_NUM; i++) {
        memset(&(inodes[i]), 0, sizeof(inode_t) );
    }

	if (meta_writeToDisk() ==-1){
		return -1;
	}

	//Initialize all indirect blocks to -1
	int b[BLOCK_SIZE/4];
	for (int i=0; i<BLOCK_SIZE/4; i++){
		b[i] = -1;
	}
	// Write indirect_block block to disk
	if (bwrite(DEVICE_IMAGE, superblock.Indirect_block, &b) == -1){
		return -1;
	}

	// Reset all data blocks
	char empty_block[BLOCK_SIZE];
	memset(empty_block, '0', BLOCK_SIZE);

	for (int i = 0; i < superblock.block_num; i++) {
		if (bwrite(DEVICE_IMAGE, i+firstDataBlock, empty_block) == -1) {
			return -1;
		}
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
	
	int b_id, inode_id ;

	// Check if filename alredy exists
	if (name_i(fileName) != -1 ){
		return -1;
	}

	//If there isn't enought space return -2
	inode_id = ialloc();
    if (inode_id == -1){
        return -2;
	}
    
	b_id = balloc();
	// If there isn't disk space then free the inode
    if (b_id == -1) {
		ifree(inode_id);
        return -2;
    }

	// Set dafualt settings for the new inode
    strcpy(inodes[inode_id].name, fileName);
    inodes[inode_id].direct_block = b_id;
	inodes[inode_id].size = 0;
    
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
	
	int b_id, inode_id;

	// Check if filename exists
	inode_id = name_i(fileName);
	if (inode_id == -1){
		return -1;
	}
	// Free the direct block
	if (bfree(inodes[inode_id].direct_block) == -1) {return -2;}
	
	// Get the number of blocks occupied
	int blocks = inodes[inode_id].size/2048;
	if (blocks > 1){
		blocks = blocks-1;
		// Free all the indirect blocks
		for (int i = 0; i<blocks; i++){
			b_id = b_map(inode_id, (BLOCK_SIZE*i)+1);
			if (bfree(b_id) == -1) {return -2;}
		}

		// Reset indirectBlocks_block lines to -1 
		int b[BLOCK_SIZE/4];
		if (bread(DEVICE_IMAGE, superblock.Indirect_block, &b) == -1){return -2;}
		for (int i=0; i<blocks; i++){
			b[(inode_id*4)+i]=-1;
		}
		if (bwrite(DEVICE_IMAGE, superblock.Indirect_block, &b) == -1){return -2;}
	}

	if (ifree(inode_id) == -1 ){return -2;} 
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
	int position, blocks, toread;

	// If filesystem isn't mounted return error
	if (!isMounted){
		return -1;
	}
	
	//Check that the file exist
	if(bitmap_getbit(superblock.inode_map, fileDescriptor) == 0){
		return -1;
	}
	
	// Check that the file is open
	if(inodes_x[fileDescriptor].state == CLOSE){
		return -1;
	}

	// Check that the size to read is less than 0
	if( numBytes <= 0){
		return -1;
	}
	
	// Check that the size to read is less than the remaining size
	position = inodes_x[fileDescriptor].offset;
	if (position == inodes[fileDescriptor].size - 1) {return 0;} 
	if (numBytes > inodes[fileDescriptor].size - position) {
		numBytes = numBytes - (inodes[fileDescriptor].size - position);
	}

	blocks = ((position + numBytes)/BLOCK_SIZE) +1;
	char b[BLOCK_SIZE];

	int readed = 0;
	int toread, block_id;
	for (int i = 0; i < blocks; i++){
		if (i==0){//First block to read
			block_id = b_map(fileDescriptor, position); 
			toread = BLOCK_SIZE - (position%BLOCK_SIZE);
			bread(DEVICE_IMAGE, superblock.FirstDaBlock + block_id, b); 
			memmove(buffer+readed, b+position%BLOCK_SIZE, toread); 
			readed += toread; 
			position += toread;
		} 
		else if(i==blocks-1){ //Last block to read
			block_id = b_map(fileDescriptor, position);
			toread = numBytes - readed;
			bread(DEVICE_IMAGE, superblock.FirstDaBlock + block_id, b);
			memmove(buffer+readed, b, toread);
			readed += toread;
			position += toread;
		}
		else{ //Intermediate blocks
			block_id = b_map(fileDescriptor, position);
			bread(DEVICE_IMAGE, superblock.FirstDaBlock + block_id, b);
			memmove(buffer+readed, b, BLOCK_SIZE);  
			readed += BLOCK_SIZE;
			position += BLOCK_SIZE;
		}		
	}
	
	inodes_x[fileDescriptor].offset = position; //Hay que llamar a lseek?
	return readed;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	return -1;
}

/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{
	return -1;
}

/*
 * @brief	Checks the integrity of the file.
 * @return	0 if success, -1 if the file is corrupted, -2 in case of error.
 */

int checkFile(char *fileName)
{
	return -2;
}

/*
 * @brief	Include integrity on a file.
 * @return	0 if success, -1 if the file does not exists, -2 in case of error.
 */

int includeIntegrity(char *fileName)
{
	return -2;
}

/*
 * @brief	Opens an existing file and checks its integrity
 * @return	The file descriptor if possible, -1 if file does not exist, -2 if the file is corrupted, -3 in case of error
 */
int openFileIntegrity(char *fileName)
{

	return -2;
}

/*
 * @brief	Closes a file and updates its integrity.
 * @return	0 if success, -1 otherwise.
 */
int closeFileIntegrity(int fileDescriptor)
{
	return -1;
}

/*
 * @brief	Creates a symbolic link to an existing file in the file system.
 * @return	0 if success, -1 if file does not exist, -2 in case of error.
 */
int createLn(char *fileName, char *linkName)
{
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
	int i;

	// search for the first free inode
	for (i = 0; i < MAX_FILE_NUM; i++){
		if (bitmap_getbit(superblock.inode_map, i) == 0) {
			// Now inode its not free
			bitmap_setbit(superblock.inode_map, i, 1);
			memset(&(inodes[i]), 0, sizeof(inode_t));
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
	for (int i = 0; i < superblock.block_num; ++i)
	{
		if (bitmap_getbit(superblock.block_map, i) == 0)
		{
			// Now it's not free
			bitmap_setbit(superblock.block_map, i, 1);

			// We write the dummy block
			memset(b, 0, BLOCK_SIZE);
			bwrite(DEVICE_IMAGE, superblock.FirstDaBlock + i, b);

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
	
	if (bitmap_getbit(superblock.inode_map, inode_id) == 0){
		return -1;
	}

	// free inode
	bitmap_setbit(superblock.inode_map, inode_id, 0);

	return 0;
}

/*
 * @brief 	Free a block in memory
 * @return 	0 if success, -1 otherwise.
 */
int bfree(int block_id){

	if (bitmap_getbit(superblock.block_map, block_id) == 0){
		return -1;
	}

	// free block
	bitmap_setbit(superblock.block_map, block_id, 0);

	return 0;
}

/*
 * @brief 	Search for a inode with name 'fname'
 * @return 	inode id if success, -1 otherwise.
 */
int name_i(char *fname){

	// search an i-node with name <fname>
	for (int i = 0; i < superblock.num_inodes; i++){
		if (!strcmp(inodes[i].name, fname)){
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
	int b[BLOCK_SIZE / 4];

	if (bitmap_getbit(superblock.inode_map, inode_id) == 0){
		return -1;
	}

	// Check that the offset is smaller than maximum size
	if (offset > inodes[inode_id].size){
		return -1;
	}

	// return direct block
	if (offset < BLOCK_SIZE){
		return inodes[inode_id].direct_block;
	}
	// return indirect block
	else {
		// Get inode first index in indirect_block block
		int firstLine = inode_id * 4;
		// Get the absolute index where inode is
		int line = firstLine + (offset/BLOCK_SIZE);
		// Read from memory the indirect_blocks block
		bread(DEVICE_IMAGE, superblock.Indirect_block, b);
		// If it isn't initialized, then alloc a block
		if (b[line] == -1) {
			int b_id = balloc();
			b[line] = b_id;
			bwrite(DEVICE_IMAGE, superblock.Indirect_block, (&b));
			return b_id;
		}else {
			return b[line];
		}
	}

	return -1;
}

/*
 * @brief 	Read metadata from disk to memory
 * @return 	0 if success, -1 otherwise.
 */
int meta_readFromDisk(void){

	// Read the superblock from disk to memory
	if (bread(DEVICE_IMAGE, SuperBlock_Block, &superblock) == -1){
		return -1;
	}

	// Read the inodes from disk to memory
	if (bread(DEVICE_IMAGE, Inodes_Block, &inodes) == -1){
		return -1;
	}

	return 0;
}

/*
 * @brief 	Write metadata from memory to disk
 * @return 	0 if success, -1 otherwise.
 */
int meta_writeToDisk(void){

	// Write the superblock from memory to disk
	if (bwrite(DEVICE_IMAGE, SuperBlock_Block, &superblock) == -1){
		return -1;
	}
	// Write the inodes from memory to disk
	if (bwrite(DEVICE_IMAGE, Inodes_Block, &inodes) == -1){
		return -1;
	}

	return 0;
}