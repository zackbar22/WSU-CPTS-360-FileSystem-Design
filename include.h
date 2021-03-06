#ifndef INCLUDE_H
#define INCLUDE_H

#include <time.h>
#include "type.h"
#include <stdio.h>

#define RED "\033[1m\x1B[31m"
#define CYN "\033[1m\x1B[36m"
#define GRN "\033[1m\x1B[32m"
#define WHT "\033[0m\x1B[37m"

typedef unsigned int u32;

extern int dev;
//extern static char *name[128];
extern char pathname[128], parameter[128], cwdname[128];
extern char names[128][256];
extern char *command_name;

extern int  nnames;
extern char *rootdev, *slash, *dot;
extern int iblock;

extern MINODE *root; 
extern MINODE minode[NMINODES];
extern MOUNT  mounttab[NMOUNT];
extern PROC   proc[NPROC], *running;
extern OFT    oft[NOFT];

extern MOUNT *getmountp();

extern int DEBUG, nproc;

extern int fd;
extern u32 bg_inode_table;

extern char buf[BLOCK_SIZE];

extern MOUNT *mp;

extern int bmap, imap, inode_start;
extern int ninodes, nblocks, ifree, bfree;

extern char *entry_name(DIR *folder);

/*

extern int dev;
extern int ninodes;
extern int nblocks;
extern int bmap;

extern int print_dir_entries(MINODE *mip);
extern MINODE *iget(int dev, int ino);
extern int iput(MINODE *mip);
extern int get_block(int fd, int blk, char buf[ ]);
extern int fd;
extern MINODE *root;
extern PROC *running;
extern u32 bg_inode_table;
extern int put_block(int fd, int blk, char buf[ ]);
extern int ialloc(int dev);

extern char buf[BLOCK_SIZE];
extern MINODE minode[NMINODES];

*/

#endif








