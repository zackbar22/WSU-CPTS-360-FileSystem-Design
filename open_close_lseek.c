#include "include.h"
#include <time.h>

//Truncates a file
//TODO: All of it...
MINODE* my_truncate(MINODE *mip)
{
	/*
	1. release mip->INODE's data blocks;
	a file may have 12 direct blocks, 256 indirect blocks and 256*256
	double indirect data blocks. release them all.
	2. update INODE's time field
	3. set INODE's size to 0 and mark Minode[ ] dirty
	*/
	
	int i;
	// Free da blocks of this mip
	for (i=0; i< 14; i++){
		mip->INODE.i_block[i] = 0;	
	}
	
	mip->INODE.i_size = 0;
	mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
	return mip;
}

/**
 *
 * TODO:
 * 	-	Set mode as appropriate
 * 	-	Check if inode is a file
 * 	-	Check if file is already in use
 * 	-	How does the global variable oft[] get created?
 */

int laopen_file(char *pathname, char* str_mode ){
	int mode = atoi(str_mode);
	printf("pathname %s mode %d\n", pathname, mode);
	printf("Entered open_file:\n");

	if (pathname[0]=='/'){
		dev = running->cwd->dev;          // root INODE's dev
		printf("open_file obtained the dev: %d\n", dev);
	}

	int ino = my_getino(&dev, pathname);
	MINODE *mip = iget(dev,ino);

	if(mip->ino ==0){
		printf("\n \n Pathname not detected!! \n");
		return 0;
	}
	printf("Obtained inode of inum:%d\n", mip->ino);

	// Return if inode is a directory
	short permission = mip->INODE.i_mode >> 12;
	printf("The type of file as an int is: %d\n", permission);
	if(permission == 4 ){
		printf("Issue: This is a directory\n");
		return 0;
	}

	/*
	* TODO: Is this right?
	Check whether the file is ALREADY opened with INCOMPATIBLE mode:
	   If it's already opened for W, RW, APPEND : reject.

	*/

	int k = 0;
	for (k = 0; k < NFD; k++){
		OFT * file_pointer = 0;
		if( (running->fd[k] != 0)  && (running->fd[k]->inodeptr->ino == mip->ino)){
			if(running->fd[k]->mode != 0){
				printf("File is already open with an incompatible mode \n" );
				return 0;
			}else{
				running->fd[k]->refCount ++;
				printf("File already opened, incrememnting ref_ counter\n\n");
				return 0;
			}
		}

	}

	// 5. allocate a FREE OpenFileTable (OFT) and fill in values:
	OFT *oftp = (OFT *)malloc(sizeof(OFT));
	oftp->mode = mode;      // mode = 0|1|2|3 for R|W|RW|APPEND
	oftp->refCount = 1;
	oftp->inodeptr = mip;  // point at the file's minode[]
	printf("Created a OFT\n");


	// 6. Depending on the open mode 0|1|2|3, set the OFT's offset accordingly:

	switch(mode){
		 case 0 : oftp->offset = 0;     // R: offset = 0
			  break;
		 case 1 : 
		 	  my_truncate(mip);        // W: truncate file to 0 size
			  oftp->offset = 0;
			  break;
		 case 2 : oftp->offset = 0;     // RW: do NOT truncate file
			  break;
		 case 3 : oftp->offset =  mip->INODE.i_size;  // APPEND mode
			  break;
		 default: printf("invalid mode\n");
		  return(0);
	}

	// Also be sure to allocate in extern variable
	int j = 0;
	for(j =0; j < NOFT; j++){
		if( &oft[j] == NULL || oft[j].refCount == 0){
			oft[j] = *oftp;
			printf("allocated oft[%d] \n", j );
			printf("OFT's inode: %d\n", oft[j].inodeptr->ino);
			break;
		}
	}

	//7. find the SMALLEST i in running PROC's fd[ ] such that fd[i] is NULL
	// Let running->fd[i] point at the OFT entry
	// Note the Proc has a fd table
	int i = 0;
	for(i =0; i < NFD; i++){
	 if( running -> fd[i] == NULL || running -> fd[i]->refCount < 1){
		 running -> fd[i] = oftp;
		 printf(" \n allocated fd[%d] \n", i );
		 break;
	 }
	}

	// update INODE's time field
	if(mode == 0){ // If read
	   oftp->inodeptr->INODE.i_atime = time(0L);
	}else{
	   oftp->inodeptr->INODE.i_atime = time(0L);
	   oftp->inodeptr->INODE.i_mtime = time(0L);
	}

	printf("Here's the inode's updated time: %d\n", oftp->inodeptr->INODE.i_atime = time(0L));

	mip->dirty = 1;
	iput(mip);

	// i is the i'th index to OFT[]
	return i;
}

/**

* Confusion between processe's fd[] and the global array oft[]
*/
int laclose_file(int fd)
{
	printf("Entering Close_file\n");
	if(fd >= NFD){
		printf("OFT index specified is out of range");
		return -1;
	}


	//2. verify running->fd[fd] is pointing at a OFT entry
	if(running -> fd[fd] == NULL)
	{
		printf("running fd is not actually pointing to a file directory...  ");
		return -1;
	}

	//3. The following code segments should be fairly obvious:

	OFT * oftp = running->fd[fd];
	running->fd[fd] = 0;
	oft[fd].refCount --;
	oftp->refCount--;

	if (oftp->refCount > 0){ return 0; }

	// last user of this OFT entry ==> dispose of the Minode[]
	MINODE * mip = oftp->inodeptr;
	iput(mip);
	printf("Closing file pointer at: %d\n", fd);


	return 0;
}



/**
 * Read da Bitch
 */
int laread(int fd,char buf[], int nbytes){
	if( running->fd[fd] == NULL || running->fd[fd]->refCount <=0 ){
		int i =0;
		for(i =0; i < 1024; i++){
			buf[i] = 0;
		}
		printf("this file is not open.\n");
		return 0;
	}

	// If this file is opened for write, please return yourself.
	if(running->fd[fd]->mode == 1 ){
		printf("This file is open for write\n");
		return 0;
	}

	//printf("This is my read\n" );

	//OFT * myfd = &oft[fd];

	return( myread(fd, buf, nbytes) );
}

/* int lseek(int fd, int position)
 {
   From fd, find the OFT entry.
   change OFT entry's offset to position but make sure NOT to over run either end
   of the file.
   return originalPosition
 }*/

 /**
  * Read helper function
  * TODO:
  * 	Recall that offset does not work because L_seek and truncate do not really work
  */
  
int indirect_block(MINODE *mip, int lbk)
{
	char temp_buf[1024]; //so that we don't change buf
	get_block(mip->dev, mip->INODE.i_block[12], temp_buf); //load the indirect blocks into 
	//temp buf
	int blk = temp_buf[lbk - 12]; //since we start at the indirect blocks
	//we must take into account the 12 blocks that came before it
	return blk;
}

int db_indirect_block(MINODE *mip, int lbk){
	char temp_buf[1024];
	get_block(mip->dev, mip->INODE.i_block[13], temp_buf);
	int blk = temp_buf[lbk - 12];
	return blk;
}
  
int myread(int fd, char buf[], int nbytes){
    // What is file size??

	OFT * oftp =  running->fd[fd];
	MINODE *mip = oftp->inodeptr;

	// I'm not too certian on this filesize variable...
	int fileSize = mip->INODE.i_size;
	int avil = fileSize - oftp->offset;

	char *cq = buf;                // cq points at buf[ ]
	char readbuf[BLKSIZE] = { 0 };

//	printf("Now in myread\n avil = %d,  \n", avil);


	int count =0;

    while (nbytes && avil){

        //Compute LOGICAL BLOCK number lbk and startByte in that block from offset;

             int lbk       = oftp->offset / BLKSIZE;
             int startByte = oftp->offset % BLKSIZE;
             int blk =0;
             if (lbk < 12){                     // lbk is a direct block
                  blk = mip->INODE.i_block[lbk]; // map LOGICAL lbk to PHYSICAL blk
                  //printf("Calculated block: %d\n",blk );
             }
             else if (lbk >= 12 && lbk < 256 + 12) {
                   //  TODO: indirect blocks
                   printf("hit indirect blocks\n");
                   getchar();
                   blk = indirect_block(mip, lbk);
             }
             else{
                   //  TODO: double indirect blocks
             }

              /* get the data block into readbuf[BLKSIZE] */
              get_block(mip->dev, blk, readbuf);

              /* copy from startByte to buf[ ], at most remain bytes in this block */
              char *cp = readbuf + startByte;

              //printf("")
              int remain = BLKSIZE - startByte;   // number of bytes remain in readbuf[]
              //printf("the remain: %d\n", remain);


              int smaller = nbytes;
              if(avil < nbytes )
            	  smaller = avil;


              count += smaller;

              //printf("Adding an offset to the file pointer\n");
              oftp->offset += smaller;
              //printf("Added an offset,  THE OFFSET IS: %d\n",oftp->offset );

              //TODO: What if doesn't fit in one set of 1024?
              if(smaller <= BLKSIZE)
               {
            	  //printf("Transfering buff data\n");
            	  //printf("cq = %s, cp = %s, smaller=,%d ", cq,cp,smaller);
            	  strncpy(cq, cp, smaller);

            	  break;
               }
              else {
            	  strncpy(cq, cp, BLKSIZE);
            	  avil -= 1024;
            	  nbytes -= 1024;
              }

              /* KC's code
              while (remain > 0){
                   *cq++ = *cp++;             // copy byte from readbuf[] into buf[]
                    oftp->offset++;           // advance offset
                    count++;                  // inc count as number of bytes read
                    avil--; nbytes--;  remain--;
                    if (nbytes <= 0 || avil <= 0)
                        break;
              }
               */

              // if one data block is not enough, loop back to OUTER while for more ...


    }// end while

    //printf("myread: read %d char from file descriptor %d\n", count, fd);
    return count;   // count is the actual number of bytes read
   }


/**
 * MEOW!
 *
 */
int my_cat(char *pathname){

	//printf("I am here at 315\n");

	  char mybuf[1024] = { 0 };
	  int dummy = 0;  // a null char at end of mybuf[ ]
	   int n=0;


	  int fd = laopen_file(pathname, "0");

//	  printf("The fd =%d\n", fd);

	  while( n = laread(fd, mybuf, 1024) ){

	//	  printf("I'm atline 327\n");
	       mybuf[n] = 0;             // as a null terminated string
	       printf("%s\n", mybuf);   //<=== THIS works but not good
	       // TODO: This -> spit out chars from mybuf[ ] but handle \n properly;
	   }

	  laclose_file(fd);

	  return 1;
}

int my_write(){
	int dest_fd; 
	char read_line[256];
	
	dest_fd = atoi(pathname);
	
	strcpy(read_line, parameter);

	//TODO 2. verify fd is indeed opened for WR or RW or APPEND mode
	
	
	if (!(dest_fd >= 0 && dest_fd <= NFD)){ //checks for invalid indices
		printf("%d is an invalid fd\n", dest_fd);
		return;
	}
	
	OFT *file_pointer = running->fd[dest_fd];
	
	if (file_pointer->mode == 0 || file_pointer->mode > 3){
		printf("File opened with an incompatible mode %d\n", file_pointer->mode);
		return;
	}
	 
	MINODE *fnode = file_pointer->inodeptr;
	
	//3. copy the text string into a buf[] and get its length as nbytes.
	
	printf("Preparing to write %s with length %d to fd %d\n", read_line, strlen(read_line), dest_fd);
	
	
	
	return mywrite(dest_fd, read_line, strlen(read_line));
}

int mywrite(int dest_fd, char buf[], int nbytes){

	OFT *file_pointer = running->fd[dest_fd];

	char wbuf[1024];
	OFT *oftp   = running->fd[dest_fd];
	MINODE *mip = oftp->inodeptr;
	INODE *ip   = &mip->INODE;
	
	int new_size = nbytes;
	
	if (file_pointer->mode == 3){
		new_size += ip->i_size;
	}
	
	if (oftp == 0){
		printf("oftp is null\n"); return;
	}else if (mip == 0){
		printf("mip is null\n"); return;
	}else if (ip == 0){
		printf("ip is null\n"); return;
	}
	
	while(nbytes > 0){
		//compute LOGICAL BLOCK (lbk) and the startByte in that lbk:
		int lbk        = oftp->offset / BLKSIZE;
		int start_byte = oftp->offset % BLKSIZE;
		char zero_buf[1024];
		memset(zero_buf, 0, 1024);
		printf("Logical block = %d\n", lbk);
		printf("Start byte = %d\n", start_byte);
		int blk;
		if (lbk < 12){ // direct block
			printf("writing to direct block\n");
			if (ip->i_block[lbk] == 0){ // if no data block yet
				blk = mip->INODE.i_block[lbk] = balloc(mip->dev); // MUST ALLOCATE a block
			        // write a block of 0's to blk on disk: OPTIONAL for data block 
			        write(mip->dev, blk, zero_buf);
   				//but MUST for I or D blocks
			}
			blk = mip->INODE.i_block[lbk];
		}
		else if (lbk >= 12 && lbk < 256 + 12){
			// indirect blocks
			/*
				I_block[13] points to a block, 
				which points to 256 blocks,
                    		each of which point to 256 blocks.
			*/
			printf("writing to indirect block\n");
			unsigned int *first_ptr = &mip->INODE.i_block[13];
			unsigned int in_start[256][256];
			memcpy(in_start, &first_ptr, sizeof(int)*256);
			int first_index = lbk / 256;
			if (in_start[first_index][lbk] == 0){
				in_start[first_index][lbk] = balloc(mip->dev);
			}
			blk = in_start[first_index][lbk];
		}
		else{
			//double indirect blocks
			printf("writing to double indirect block\n");
		}
		/* all cases come to here : write to the data block */
		printf("Writing to dev %d at block %d\n", mip->dev, blk);
		get_block(mip->dev, blk, wbuf);
		char *cp      = wbuf + start_byte;
		char *cq      = buf;
		int remain    = BLKSIZE - start_byte;
		int kc_method = 1;
		
		char the_buf[1024];
		INODE * write_back = 0;

		int cur_blk    = (mip->ino - 1) / 8 + bg_inode_table;
		int cur_offset = (mip->ino - 1) % 8;
		get_block(mip->dev, cur_blk, the_buf);
		write_back = (INODE *)the_buf + cur_offset;


		if (kc_method){	
			while (remain > 0){
				*cp++ = *cq++;
				nbytes--; remain--;
				oftp->offset++;
				if (oftp->offset > mip->INODE.i_size){
					write_back->i_size++;
					mip->INODE.i_size++;
				}
				if (nbytes <= 0) break;
			}		
		}
		else{
			while(remain > 0){
				int copied = remain;
				memcpy(cp, cq, copied);
				nbytes -= copied; remain -= copied;
				oftp->offset += copied;
				if (oftp->offset > mip->INODE.i_size){
					mip->INODE.i_size++;
				}
				if (nbytes <= 0) break;
			}
		}
		printf("new file size is %d\n", new_size);
		mip->INODE.i_size = new_size;
		ip->i_size = new_size;

		put_block(mip->dev, blk, wbuf);
		put_block(mip->dev, cur_blk, the_buf);
	}

	mip->dirty = 1;	
	iput(mip);
	return nbytes;
}

/*
 * The lseek function
 */
int my_lseek(char *s_fd, char *s_position){
	int fd = atoi(s_fd);
	int position = atoi(s_position);
	OFT *file_pointer = running->fd[fd];
	if(position <0 || position > file_pointer->inodeptr->INODE.i_size){
		printf("Invalid position entered \n");
		return 0;
	}

	file_pointer->offset = position;
	printf("file offset successful \n");
	return 0;
}


/**
 * print all the open files
 */
int pfd(char * pathname){
	printf("* * * * *pfd:* * * * \n");
	printf(" Open files: \n");
	OFT * fd =0;
	int i =0;
	char the_mode[200] = { 0 };
	for(i =0; i<NFD; i++){
		fd = running->fd[i];
		//running->fd
		if (fd == 0) continue;

		if( fd->refCount >=1){
			printf("\n  Open file fd: %d\n", i);
			printf("fd\tmode\toffset\tINODE\n");
			printf("__________________\n");
			if(fd->mode == 0){
				strcpy(the_mode, "READ");
			}
			else {
				strcpy(the_mode,"WRITE");
			}
			printf("%d %d %.4d [%d, %d]\n", i, fd->mode, fd->offset, fd->inodeptr->dev, fd->inodeptr->ino);
		}
	}
	printf("\n");
	return 0;
}






