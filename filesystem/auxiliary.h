
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	auxiliary.h
 * @brief 	Headers for the auxiliary functions required by filesystem.c.
 * @date	Last revision 01/04/2020
 *
 */

/*
 * @brief 	Allocates a inode in memory
 * @return 	Position if success, -1 otherwise.
 */
int ialloc(void);

/*
 * @brief 	Allocates a block in disk
 * @return 	Position if success, -1 otherwise.
 */
int balloc(void);

/*
 * @brief 	Free a indoe in memory
 * @return 	0 if success, -1 otherwise.
 */
int ifree(int inode);

/*
 * @brief 	Free a block in memory
 * @return 	0 if success, -1 otherwise.
 */
int bfree ( int block_id );

/*
 * @brief 	Search for a inode with name 'fname'
 * @return 	inode id if success, -1 otherwise.
 */
int name_i ( char *fname );

/*
 * @brief 	Gives the datablock where the file with offset is
 * @return 	block id if success, -1 otherwise.
 */
int b_map ( int inode_id, int offset );

/*
 * @brief 	Read metadata from disk to memory
 * @return 	0 if success, -1 otherwise.
 */
int meta_readFromDisk ( void );

/*
 * @brief 	Write metadata from memory to disk
 * @return 	0 if success, -1 otherwise.
 */
int meta_writeToDisk ( void );
