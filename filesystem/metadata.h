
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

#define INODES_SIZE MAX_FILE_NUM/2

/* Superblock type */
typedef struct superblock {
  unsigned int magic_num;	                /* Magic number for checking integrity */
  unsigned int num_inodes; 	              /* Current inodes in filesystem */
  unsigned int device_size;       	      /* Total space in filesystem */
  unsigned int block_num;                 /* Number of blocks = size/2048 */
  char inode_map[MAX_FILE_NUM/8];         /* Map of inodes */
  char block_map[MAX_BLOCK_NUM/8];        /* Map of blocks */
  char padding[BLOCK_SIZE-(4*sizeof(int))-(MAX_FILE_NUM/8)-(MAX_BLOCK_NUM/8)]; /* Padding (for filling the block) */
} superblock_t;

/* Disk inode type */
typedef struct inode {
  char name[MAX_NAME_LENGHT];	             /* Filename */
  unsigned int size;	                     /* Current file size in bytes */
  unsigned int direct_block[5];            /* Number of the direct block */
  uint32_t crc[5];
  char padding[80-76];          
} inode_t;

/* File descriptor table only in memory */
struct {
  int state;  /*open/close*/
  int offset; /* read/write position*/
  int integrity; /* true if it's open with integrity, false if not */
}inodes_x[MAX_FILE_NUM];

/* Define states */
#define OPEN  1
#define CLOSE 0

superblock_t superblock;                // superblock declaration
inode_t firstInodes[INODES_SIZE];   // First inodes block declaration
inode_t secondInodes[INODES_SIZE];  // Second inodes block declaration

#define FALSE 0
#define TRUE  1

int isMounted = FALSE;

// Structure of file system
#define SuperBlock_Block       0    //First block for superblock
#define firstInodes_Block      1    // First block for array of inodes
#define secondInodes_Block     2    // Second block for array of inodes
#define firstDataBlock         3    // Data blocks start at block 3

/*------------ Auxiliar functions ---------------------*/

#define bitmap_getbit(bitmap_, i_) (bitmap_[i_ >> 3] & (1 << (i_ & 0x07)))
static inline void bitmap_setbit(char *bitmap_, int i_, int val_) {
  if (val_)
    bitmap_[(i_ >> 3)] |= (1 << (i_ & 0x07));
  else
    bitmap_[(i_ >> 3)] &= ~(1 << (i_ & 0x07));
}
