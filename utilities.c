#include "include.h"

int ninodes;

//extern int search(INODE * inodePtr, char * name);

int iput(MINODE *mip)
{
	mip->refCount--; //decrease refCount by 1
	if (mip->refCount > 0){
		return;
	}
	if (mip->dirty == 0){
		return;
	}
	if (mip->refCount > 0 && mip->dirty == 1){
		//must write the INODE back to disk
		int dev    = mip->dev;
		int ino    = mip->ino;
		int blk    = (ino - 1) / 8 + bg_inode_table;
		int offset = (ino - 1) % 8;
		get_block(fd, blk, buf);
		INODE *ip = (INODE *)buf + offset;
		*ip = mip->INODE;
		put_block(fd, blk, buf);
	}
}

MINODE *iget(int dev, int ino)
{
	/*
	(1). Search minode[i] for an entry whose refCount > 0 with the SAME (dev,ino)
     	     if found: refCount++; mip-return &minode[i];
	(2). Find a minode[i] whose refCount = 0 => let MINODE *mip = &minode[i];
	*/

	MINODE *mip;
	int i;
	for (i = 0; i < NMINODES; i++)
	{
		if (minode[i].dev == dev && minode[i].ino == ino){
			minode[i].refCount++;
			return &minode[i];
		}else if (minode[i].refCount == 0){
			mip = &(minode[i]);
		}	
	}

      /*(3). Use Mailman's algorithm to compute*/

	printf("bg_inode_table = %d\n", bg_inode_table);

	int blk = (ino - 1) / 8 + bg_inode_table;
	int offset = (ino - 1) % 8;

    /*  (4). read blk into buf[ ]; 	*/

	printf("Getting block %d with offset %d\n", blk, offset);
	get_block(fd, blk, buf);
	INODE *ip = (INODE *)buf + offset;
	
      //(5). COPY *ip into mip->INODE

	mip->INODE = *ip;

	//printf("Mip's inode is %d\n", ip);
	
	for (i = 0; i < EXT2_N_BLOCKS; i++){
		printf("i_block[%d] = %d\n", i, ip->i_block[i]);
	}

     // (6). initialize other fields of *mip: 

	mip->ino      = ino; 
	mip->dev      = dev;

	mip->refCount =   1;
	mip->dirty    =   0;
	mip->mounted  =   0;
	mip->mountptr =   0;

	return mip;
}

int tst_bit(char *buf, int bit)
{
	int i, j;
	i = bit/8; j=bit%8;
	if (buf[i] & (1 << j))
		return 1;
	return 0;
}

int set_bit(char *buf, int bit)
{
	int i, j;
	i = bit/8; j=bit%8;
	buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
	int i, j;
	i = bit/8; j=bit%8;
	buf[i] &= ~(1 << j);
}	

int ialloc(int dev)
{
	int  i;
	char buf[BLKSIZE];

	// read inode_bitmap block
	get_block(dev, bg_inode_table, buf);

	for (i=0; i < ninodes; i++){
		if (tst_bit(buf, i)==0){
			set_bit(buf,i);
			decFreeInodes(dev);
			put_block(dev, bg_inode_table, buf);
			return i+1;
		}
	}
	printf("ialloc(): no more free inodes\n");
	return 0;
}

int decFreeInodes(int dev)
{
	char buf[BLKSIZE];

	// dec free inodes count in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count--;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count--;
	put_block(dev, 2, buf);
}

int findmyname(MINODE *parent, int myino, char *myname) 
{
	char *parent_name = parent->name;
	INODE parent_inode = parent->INODE;
	/*
	   Given the parent DIR (MINODE pointer) and myino, this function finds 
	   the name string of myino in the parent's data block. This is the SAME
	   as SEARCH() by myino, then copy its name string into myname[ ].
	*/
}

int search_minode(MINODE *mip, char *name)
{
	int i;
	for (i = 0; i < NMINODES; i++){
		MINODE current = minode[i];
		if (strcmp(current.name, name) == 0){
			return current.ino;	
		}
	}
	return -1;
}

int print_dir_entries(MINODE *mip, char *name)
{
	printf("Printing dir entries\n");
	int i; 
	char *cp, sbuf[BLKSIZE];
	DIR *dp;
	INODE *ip = &(mip->INODE);

	for (i=0; i<12; i++){  // ASSUME DIRs only has 12 direct blocks
		if (ip->i_block[i] == 0)
		{
			printf("Returning because block is 0\n");
			return 0;
		}

		//get ip->i_block[i] into sbuf[ ];
		printf("Printing block %d\n", ip->i_block[i]);
		get_block(fd, (ip->i_block[i]), sbuf);	
		//sbuf = ip->i_block[i];
		dp = (DIR *)sbuf;
		cp = sbuf;
		while (cp < sbuf + BLKSIZE){			  	
			printf("inode=%d rec_len=%d name_len=%d name=%s\n", dp->inode, dp->rec_len, dp->name_len, dp->name);
			
			// print dp->inode, dp->rec_len, dp->name_len, dp->name);
		  	// WRITE YOUR CODE TO search for name: return its ino if found
			int name_ino_num = search(ip, name);
			if (name_ino_num != 0)
			{
				return name_ino_num;
			}
			int blk = (name_ino_num - 1) / 8 + bg_inode_table;
			int offset = (name_ino_num - 1) % 8; 			
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	return 0;
}

int findino(MINODE *mip, int *myino, int *parentino)
{
	/*
	For a DIR Minode, extract the inumbers of . and .. 
	Read in 0th data block. The inumbers are in the first two dir entries.
	*/
	
}




















