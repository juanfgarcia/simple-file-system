
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	test.c
 * @brief 	Implementation of the client test routines.
 * @date	01/03/2017
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "filesystem/filesystem.h"


// Color definitions for asserts
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_RED   "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_BLUE  "\x1b[34m"

#define N_BLOCKS 240				   // Number of blocks in the device
#define RIGHT_DEV_SIZE N_BLOCKS *BLOCK_SIZE // Device size, in bytes
#define BIG_DEV_SIZE 700*BLOCK_SIZE


void assertHelper(char * test, int got, int want){
	if (got != want ){
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, test , ANSI_COLOR_RED, " FAILED", ANSI_COLOR_RESET);
		fprintf(stdout, "%s%s%d%s%d%s", ANSI_COLOR_RED, " Expected ", want, " but got ", got, "\n");
	}else {
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, test, ANSI_COLOR_GREEN, " SUCCESS\n", ANSI_COLOR_RESET);
	}    
}

void assertString(char * test, char *got, char *want){
	if (strcmp(got, want)){
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, test , ANSI_COLOR_RED, " FAILED", ANSI_COLOR_RESET);
		fprintf(stdout, "%s%s%s%s%s%s", ANSI_COLOR_RED, " Expected ", want, " but got ", got, "\n");
	}else {
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, test, ANSI_COLOR_GREEN, " SUCCESS\n", ANSI_COLOR_RESET);
	}    
}

char *test;
int got, want;
/*
* @Name: mkFS with big size 
* @Description: check that the mkFS with a bigger size returns -1 
*/
void test_mkFS_bigSize(){
    test = "mkFS with big size";
	got = mkFS(BIG_DEV_SIZE);
	want = -1;
	assertHelper(test, got, want);
}

/*
* @Name: mkFS with right size 
* @Description: check that the mkFS with the right size returns 0 
*/
void test_mkFS_rightSize(){
    test = "mkFS with right size";
	got = mkFS(RIGHT_DEV_SIZE);
	want = 0;
	assertHelper(test, got, want);
}

/*
* @Name: Mount basic
* @Description: check that mount doesn't return error 
*/
void test_mountFS() {
	test = "Basic mountFS";
	got = mountFS();
	want = 0;
	assertHelper(test, got, want);
}

/*
* @Name: Mount when is mounted
* @Description:  Check that when FS is previously mounted returns error
*/
void test_mountFS_isMounted(){
	test = "MountFS when is mounted";
	got = mountFS();
	want = -1;
	assertHelper(test, got, want);
}

/*
* @Name: Create a file basic test
* @Description:  Check that create file doesn't return error
*/
void test_createFile() {
	test = "Create a file basic test";
	got = createFile("test.txt");
	want = 0;
	assertHelper(test, got, want);
}

/*
* @Name: Create a file that alredy exists
* @Description: Check that creating a file that alredy exists returns -1
*/
void test_createFile_exists() {
	test = "Create a file that alredy exists";
	got = createFile("test.txt");
	want = -1;
	assertHelper(test, got, want);
}

/*
* @Name: Create a file whith illegal size
* @Description: Check that creating a file with illegal name size return error
*/
void test_createFile_illegalName() {
	test = "Create a file whith illegal size";
	char * illegal_name = malloc(FILENAME_MAX+2);
	memset(illegal_name, 'a', FILENAME_MAX+1);
	got = createFile(illegal_name);
	want = -2;
	assertHelper(test, got, want);
	free(illegal_name);
}

/*
* @Name: Open a file that exists
* @Description: Check that opening a file that exists returns 0
*/
void test_openFile_exists() {
	test = "Open a file that exists";
	got = openFile("test.txt");
	want = 0; // Should returns fd but as its first file should be 0
	assertHelper(test, got, want);
}

/*
* @Name: Open a file that doesnt exists
* @Description: Check that opening a file that alredy exists returns -1
*/
void test_openFile_notExists() {
	test = "Open a file that don't exists";
	got = openFile("otro.txt");
	want = -1; 
	assertHelper(test, got, want);
}

/*
* @Name: Close a file that exists
* @Description: Check that closing a file that exists return 0
*/
void test_closeFile_exists() {
	test = "Close a file that exists";
	got = closeFile(0); //Close file descriptor 0 (test.txt)
	want = 0; 
	assertHelper(test, got, want);
}

/*
* @Name: Close a file that exists
* @Description: Check that closing a file that doesnt exists returns -1
*/
void test_closeFile_dontExists() {
	test = "Close a file that doesn't exists";
	got = closeFile(100); //Close file descriptor 0 (test.txt)
	want = -1; 
	assertHelper(test, got, want);
}

/*
* @Name: Close a file that exists but isn't opened
* @Description: Check that closing a file that exists but isnt opened returns error
*/
void test_closeFile_notOpened() {
	test = "Close a file that exists but isn't opened";
	createFile("test_.txt");
	got = closeFile(1); //Close file descriptor 1 (test_.txt)
	want = -1; 
	assertHelper(test, got, want);
}

/*
* @Name: Remove a file that exists 
* @Description: Check that remove a file that exists  returns 0
*/
void test_removeFile_exists() {
	test = "Remove a file that exists";
	got = removeFile("test_.txt"); 
	want = 0; 
	assertHelper(test, got, want);
}

/*
* @Name: Remove a file that doesnt exists 
* @Description: Remove a file that doesnt exists 
*/
void test_removeFile_dontExists() {
	test = "Remove a file that doesnt exists";
	got = removeFile("test__.txt"); 
	want = -1; 
	assertHelper(test, got, want);
}

/*
* @Name: Write a file that exists
* @Description: Check that writing a file that exists returns the writed bytes
*/
void test_writeFile() {
	test = "Write a file that exists";

	int fd = openFile("test.txt");
	char *buff = malloc(20+1);
	memset(buff, 'a', 20);buff[20] = '\0';
	
	got = writeFile(fd, buff, strlen(buff));
	want = strlen(buff); 
	assertHelper(test, got, want);
	free(buff);
}

/*
* @Name: Write more than a block to a file
* @Description: Check that writing more than a block return the writed bytes
*/
void test_writeFile_bigWrite() {
	test = "Write more than a block in a file";
	
	// Reset file system for simplicity
	mkFS(RIGHT_DEV_SIZE);mountFS();createFile("test.txt");

	int fd = openFile("test.txt");
	char *buff = malloc(BLOCK_SIZE*3+1);
	memset(buff, 'a', BLOCK_SIZE);
	memset(&buff[BLOCK_SIZE], 'b', BLOCK_SIZE);
	memset(&buff[2*BLOCK_SIZE], 'c', BLOCK_SIZE);
	
	got = writeFile(fd, buff, strlen(buff));
	want = strlen(buff); 
	assertHelper(test, got, want);
	free(buff);
}

/*
* @Name: Write a file that exists
* @Description: Check that writing more than 10k only writes 10k
*/
void test_writeFile_outOfLimits() {
	test = "Write more than max size";
	
	// Reset file system for simplicity
	mkFS(RIGHT_DEV_SIZE);mountFS();createFile("test.txt");

	int fd = openFile("test.txt");
	char *buff = malloc((MAX_FILE_SIZE*2)+1);
	memset(buff, 'a', MAX_FILE_SIZE*2);
	buff[MAX_FILE_SIZE*2] = '\0';	

	got = writeFile(fd, buff, strlen(buff));
	want = MAX_FILE_SIZE; 
	assertHelper(test, got, want);
	free(buff);
}

/*
* @Name: Write a file doesnt exists
* @Description: Check that writing a file that doesnt exist retur error
*/
void test_writeFile_dontExist() {
	// Reset file system for simplicity
	mkFS(RIGHT_DEV_SIZE);mountFS();

	test = "Write a file that doesn't exists";
	char buff[10];
	got = writeFile(5, buff, 0); // file descriptor 5 doesn't exist
	want = -1;
	assertHelper(test, got, want);
}

/*
* @Name: Write a file that isn't opened exists
* @Description: Check that writing a file that exist but isnt opened return error
*/
void test_writeFile_notOpened() {
	test = "Write a file that isn't opened";
	char buff[10];
	// Reset file system for simplicity
	mkFS(RIGHT_DEV_SIZE);mountFS();
	got = writeFile(0, buff, 0);// File descriptro of test.txt 
	want = -1;
	assertHelper(test, got, want);
}

/*
* @Name: Read a file  basic test
* @Description: Check that readed is the same as writed
*/
void test_readFile() {
	test = "Read a file previusly writed";
	//char *readed = malloc(20+1); readed[20]='\0';
	

	// Reset file system for simplicity
	mkFS(RIGHT_DEV_SIZE);mountFS();createFile("test.txt");

	char *buff = malloc(20+1);
	memset(buff, 'a', 20);buff[20] = '\0';
	
	int fd = openFile("test.txt");
	writeFile(fd, buff, strlen(buff));
	//lseekFile(fd, 0, FS_SEEK_BEGIN);
	
	//got = readFile(fd, readed, strlen(buff));
	//want = strlen(buff);

	//assertHelper(test, got, want);
	//assertString(test, readed, buff); 
}

/*
* @Name: Read more bytes than file size
* @Description: Check that reading more than what is writed return only what is writed
*/
void test_readFile_more() {}

/*
* @Name: Read a file that isn't opened
* @Description: Check that reading more than what is writed return only what is writed
*/
void test_readFile_dontExist() {}


/*
* @Name: Read a file that isn't opened
* @Description: Check that reading more than what is writed return only what is writed
*/
void test_readFile_notOpened() {}





int main() {
	test_mkFS_bigSize();
	test_mkFS_rightSize();

	test_mountFS();
	test_mountFS_isMounted();

	test_createFile();
	test_createFile_exists();
	test_createFile_illegalName();

	test_openFile_exists();
	test_openFile_notExists();

	test_closeFile_exists();
	test_closeFile_dontExists();
	test_closeFile_notOpened();

	test_removeFile_exists();
	test_removeFile_dontExists();

	test_writeFile();
	test_writeFile_bigWrite();
	test_writeFile_outOfLimits();
	test_writeFile_dontExist();
	test_writeFile_notOpened();

	test_readFile();

	unmountFS();

	return 0;
}
