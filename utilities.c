#include "include.h"

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

	MINODE *mip = 0;
	int i;
	for (i = 0; i < NMINODES; i++)
	{
		if (minode[i].dev == dev && minode[i].ino == ino){
			minode[i].refCount++;
			return &minode[i];
		}else if (minode[i].refCount == 0){
			mip = &minode[i];
		}	
	}

      /*(3). Use Mailman's algorithm to compute*/

	int blk = (mip->ino - 1) / 8 + bg_inode_table;
	int offset = (mip->ino - 1) % 8;

    /*  (4). read blk into buf[ ]; 	*/

	get_block(fd, blk, buf);
	INODE *ip = (INODE *)buf + offset;
	
      //(5). COPY *ip into mip->INODE

	mip->INODE = *ip;

     // (6). initialize other fields of *mip: 

	mip->ino      = ino; 
	mip->dev      = dev;

	mip->refCount =   1;
	mip->dirty    =   0;
	mip->mounted  =   0;
	mip->mountptr =   0;

	return mip;
}

int ialloc(int dev)
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeInodes(dev);

       put_block(dev, imap, buf);

       return i+1;
    }
  }
  printf("ialloc(): no more free inodes\n");
  return 0;
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

























