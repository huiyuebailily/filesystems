/*
 *------------
 * This code is provided solely for the personal and private use of
 * students taking the CSC369H5 course at the University of Toronto.
 * Copying for purposes other than this use is expressly prohibited.
 * All forms of distribution of this code, whether as given or with
 * any changes, are expressly prohibited.
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2019 MCS @ UTM
 * -------------
 */

#include "ext2fsal.h"
#include "e2fs.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>


void ext2_fsal_init(const char* image)
{
    /**
     * TODO: Initialization tasks, e.g., initialize synchronization primitives used,
     * or any other structures that may need to be initialized in your implementation,
     * open the disk image by mmap-ing it, etc.
     */
    int k;
    int fd = open(image, O_RDWR);
    si.disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(si.disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    si.sb = (struct ext2_super_block *)(si.disk + 1024);
    si.gd =  (struct ext2_group_desc   *)(si.disk + 2048);
    si. block_bitmap_p = (unsigned char *)(si.disk + 3072);
    si.inode_bitmap_p = (unsigned char *)(si.disk + 4096);
    si.inodes =  (struct ext2_inode*)(si.disk + 5120);
#if 0
    printf("Inodes: %d\n", si.sb->s_inodes_count);
    printf("Blocks: %d\n", si.sb->s_blocks_count);
    printf("Block group:\n");
    printf("    block bitmap: %d\n", si.gd->bg_block_bitmap);
    printf("    inode bitmap: %d\n", si.gd->bg_inode_bitmap);
    printf("    inode table: %d\n", si.gd->bg_inode_table);
    printf("    free blocks: %d\n", si.gd->bg_free_blocks_count);
    printf("    free inodes: %d\n", si.gd->bg_free_inodes_count);
    printf("    used_dirs: %d\n", si.gd->bg_used_dirs_count);

    printf("Block bitmap: ");
   int i,j;
    for (i=0;i<si.sb->s_blocks_count/8;i++){
       //printf(" %b", block_bitmap_p[i]);
       print_uc(si.block_bitmap_p[i]);
    }
    printf("\nInode bitmap: ");
    for (i=0;i<si.sb->s_inodes_count/8;i++){
       print_uc(si.inode_bitmap_p[i]);
    }
    

    printf("\n\nInodes:\n");
    i = 2;

    print_one_inode(1,si.inodes[1],si.sb);

    for (i=si.sb->s_first_ino;i<si.sb->s_inodes_count;i++){
	print_one_inode(i,si.inodes[i],si.sb);
    }

    
    printf("\nDirectory Blocks:\n");
    int l,num,counter;
    struct ext2_dir_entry *dir;
    char *name;
    
   num = (1024<<si.sb->s_log_block_size)/512;
   for (j=0;j<si.inodes[1].i_blocks / num;j++){
	l =0;
	counter =0;
	printf("    DIR BLOCK NUM: %d (for inode %d)\n", si. inodes[1].i_block[j],2);
	while (counter  < 1024){
		    dir = (struct ext2_dir_entry *)(si.disk + 1024*si.inodes[1].i_block[j] +counter );
		    name = (char *)(si.disk + 1024*si.inodes[1].i_block[j] + counter+sizeof(struct ext2_dir_entry ));
	        	    print_one_dir(1, j, *dir,name);
		    l++;
		  counter += dir->rec_len;

	}
    }
    

    unsigned short mode;
    for (i=si.sb->s_first_ino;i<si.sb->s_inodes_count;i++){
	mode = si.inodes[i].i_mode & 0xF000;

         	if (mode  == EXT2_S_IFDIR) {
             	    for (j=0;j<si.inodes[i].i_blocks / num;j++){
		l =0;
		counter =0;
		printf("    DIR BLOCK NUM: %d (for inode %d)\n",  si.inodes[i].i_block[j],i+1);
		while (counter  < 1024){
			dir = (struct ext2_dir_entry *)(si.disk + 1024*si.inodes[i].i_block[j] +counter );
			name = (char *)(si.disk + 1024*si.inodes[i].i_block[j] + counter+sizeof(struct ext2_dir_entry ));
			print_one_dir(i, j, *dir,name);
			l++;
			counter += dir->rec_len;
		}

	}
         }
    }
#endif

	//initialize locks
	pthread_mutex_init(&sl.glock,NULL);
	pthread_mutex_init(&sl.block,NULL);
	pthread_mutex_init(&sl.ilock,NULL);
	
	for (k=0;k<INODES_NUM;k++)
	pthread_mutex_init(&sl.inodelocks[k],NULL);
}

void ext2_fsal_destroy()
{
    /**
     * TODO: Cleanup tasks, e.g., destroy synchronization primitives, munmap the image, etc.
     */
	int k;
	msync((void *)si.disk, 128 * 1024, MS_SYNC);
	pthread_mutex_destroy(&sl.glock);
	pthread_mutex_destroy(&sl.block);
	pthread_mutex_destroy(&sl.ilock);
	
	for (k=0;k<INODES_NUM;k++)
	pthread_mutex_destroy(&sl.inodelocks[k]);
}


