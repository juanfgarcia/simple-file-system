
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	metadata.h
 * @brief 	Definition of the structures and data types of the file system.
 * @date	Last revision 01/04/2020
 *
 */
#define MAX_FILE_NUM 48
#define MAX_NAME_LENGHT 32

#define MIN_DISK_SIZE 460*1024
#define MAX_DISK_SIZE 600*1024

#define MAX_BLOCK_NUM 48*5

/* Superblock type */
typedef struct superblock {
  unsigned int magic_num;	                /* Magic number for checking integrity */
  unsigned int num_inodes; 	              /* Current inodes in filesystem */
  unsigned int device_size;       	      /* Total space in filesystem */
  unsigned int block_num;                 /* Number of blocks = size/2048 */
  unsigned int Indirect_block;            /* First block for the indirect blocks */
  unsigned int FirstDaBlock;              /* First block of data */  
  char inode_map[MAX_FILE_NUM/8];         /* Map of inodes */
  char block_map[MAX_BLOCK_NUM/8];        /* Map of blocks */
  char padding[BLOCK_SIZE-(7*sizeof(int))-(MAX_FILE_NUM/8)-(MAX_BLOCK_NUM/8)]; /* Padding (for filling the block) */
} superblock_t;

/* Disk inode type */
typedef struct inode {
  char name[MAX_NAME_LENGHT];	             /* Filename */
  unsigned int size;	                     /* Current file size in bytes */
  unsigned int direct_block;               /* Number of the direct block */          
} inode_t;

/* File descriptor table only in memory */
struct {
  int state;  /*open/close*/
  int offset; /* read/write position*/
}inodes_x[MAX_FILE_NUM];

/* Define states */
#define OPEN  1
#define CLOSE 0

superblock_t superblock;        // superblock declaration
inode_t inodes[MAX_FILE_NUM];   //inodes declaration 

#define FALSE 0
#define TRUE  1

int isMounted = FALSE;

int indirect_block_1[216];  //Array for the indirect block of first 24 inodes (9 lines each)
int indirect_block_2[216];  //Array for the indirect block of second 24 inodes (9 lines each)

// Structure of file system
#define SuperBlock_Block       0    //First block for superblock
#define Inodes_Block           1    // Second block for array of inodes
#define IndirectBlock_block    2    // Third block for the first array of indirect block
#define firstDataBlock         3    // Data blocks start at block 3

/*------------ Auxiliar functions ---------------------*/

#define bitmap_getbit(bitmap_, i_) (bitmap_[i_ >> 3] & (1 << (i_ & 0x07)))
static inline void bitmap_setbit(char *bitmap_, int i_, int val_) {
  if (val_)
    bitmap_[(i_ >> 3)] |= (1 << (i_ & 0x07));
  else
    bitmap_[(i_ >> 3)] &= ~(1 << (i_ & 0x07));
}
