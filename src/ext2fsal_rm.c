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
#include <sys/mman.h>
#include "ext2fsal.h"
#include "e2fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int delete_a_file(int parent_inode_no,int file_inode_no, const char *file_name)
{
	int j,k,pno,fno;
	int counter;
	char *name;

	pno = parent_inode_no;
	fno = file_inode_no;
	//free the blocks 
	if (fno <= 0 || pno <= 0)
	return -1;
	for (k=0;k< si.inodes[fno].i_blocks/2 && k<12;k++){
		if (si.inodes[fno].i_block[k] != 0)
		set_block_bitmap(si.inodes[fno].i_block[k] ,0);
	}
	if (si.inodes[fno].i_block[12] != 0){//delete first indirect block
		int *dir_block;
		dir_block =  (int *)(si.disk + 1024*si.inodes[fno].i_block[12] );
		for (j=0;j<256;j++)
		if (dir_block[j] > 0){
			set_block_bitmap(dir_block[j],0);
		}
		set_block_bitmap(si.inodes[fno].i_block[12],0);
	}
	//free file inode_no
	 set_inode_bitmap(file_inode_no, 0);
	//delete inode info from its parent
	struct ext2_dir_entry *dir,*pre_dir;
	for (j=0;j< si.inodes[pno].i_blocks/2 ;j++){
		counter = 0;
		pre_dir = (struct ext2_dir_entry *)NULL;
		while (counter  < 1024){
			dir = (struct ext2_dir_entry *)(si.disk + 1024*si.inodes[pno].i_block[j] +counter );
			name = (char *)(si.disk + 1024*si.inodes[pno].i_block[j]+counter +sizeof(struct ext2_dir_entry ));
			if (!strncmp(name,file_name,dir->name_len)){
				if (counter += dir->rec_len == 1024 && pre_dir !=(struct ext2_dir_entry *)NULL ){//the last dir
					pre_dir->rec_len += dir->rec_len ;
					return 1;
				}
				//free the block
				set_block_bitmap(si.inodes[pno].i_block[j] ,0);
				for (k = j;k< si.inodes[pno].i_blocks/2-1;k++)
				si.inodes[pno].i_block[k] = si.inodes[pno].i_block[k+1];
				si.inodes[pno].i_blocks -= 2;
				return 1;
			}
			pre_dir  = dir;
			counter += dir->rec_len;
		}
	}


	return 0;
}
int32_t ext2_fsal_rm(const char *path)
{
    /**
     * TODO: implement the ext2_rm command here ...
     * the argument 'path' is the path to the file to be removed.
     */

     /* This is just to avoid compilation warnings, remove this line when you're done. */
	struct path_analyses  path_ana;
	
	analyse_one_path(path, &path_ana);

	if (path_ana.is_valid_path != 1){
		//src is not a valid file
		printf("%s is not a valid path, can not rm.%d,%d\n", path,path_ana.is_valid_path,path_ana.last_is);
		return ENOENT;//ENOENT;
	}
	if (path_ana.last_is == 0){
		//src is not a valid file
		unlock_one_inode(path_ana.parent_inode_no);
		printf("%s is not a valid path, can not rm.%d,%d\n", path,path_ana.is_valid_path,path_ana.last_is);
		return ENOENT;//ENOENT;
	}
	if (path_ana.last_is == 2 ){
		//path is not a valid file
		unlock_one_inode(path_ana.last_inode_no);
		printf("%s is a dir, can not rm.%d,%d\n", path,path_ana.is_valid_path,path_ana.last_is);
		return  EISDIR;//EISDIR;
	}
	//printf("path parent_inode_no=%d,last_inode_no=%d,last_name=%s\n",path_ana.parent_inode_no+1,path_ana.last_inode_no+1,path_ana.last_name);
	delete_a_file(path_ana.parent_inode_no,	//inode_no of dest path last level
		path_ana.last_inode_no,		//dest file's inode_no, need create
		path_ana.last_name);		//dest file's inode_no, need create
	msync((void *)si.disk, 128 * 1024, MS_SYNC);
	unlock_one_inode(path_ana.parent_inode_no);
	
    return 0;
}


