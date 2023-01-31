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

#include <sys/mman.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>



char **get_dir_names(const char *path)
{	
	int start_po;
	int i,counter,max_dir_num;
	char *tmp_path;
	char **dir_names;
	tmp_path = (char *)malloc(strlen(path)+1);
	strcpy(tmp_path,path);
	max_dir_num = strlen(path);
	dir_names = (char **)malloc(sizeof(char*)*(max_dir_num+1));
	for (i=0;i<max_dir_num+1 ;i++)
	dir_names[i] =(char *)NULL;
	
	i = 0;
	counter = 0;
	while (counter< max_dir_num){
	    if (tmp_path[counter ] == '/') {
	        tmp_path[counter ] = 0;
	         counter ++;
	    }
	    else {
	        //dir_names[i] = tmp_path + counter;
	        start_po = counter;
	        while (counter< max_dir_num && tmp_path[counter ] != '/') counter ++;
	        dir_names[i] = (char *)malloc(counter - start_po +1);

	        memcpy(dir_names[i],tmp_path +start_po ,counter - start_po);
	        dir_names[i][counter - start_po] = '\0';
	        i++;
	    }
	   
	}
	free(tmp_path );
 	return dir_names;
}




//create a new dir in the inode(inode_no) , and return the new inode for the new dir
//return value:if succeed ,return the new inode_no of this new dir
//	0: no free block
//	-1:no free inode
int create_one_dir(int inode_no, char *dir_name)
{
	int i,block_po,counter;
	int retcode,new_inode_no;
	int record_po;
	struct ext2_dir_entry *dir,*new_dir;
	char *name;
   	//struct ext2_inode *inodes =  (struct ext2_inode*)(disk + 5120);
	
	int rec_len;
	rec_len = sizeof(struct ext2_dir_entry) + (strlen(dir_name) +sizeof(char*)-1)/sizeof(char*)*sizeof(char*);
	retcode = get_block_position_for_new_dir(inode_no,rec_len, &block_po,&record_po);
	//if (retcode != 1){//fail to get block no
	if (retcode < 1){//fail to get block no
		return 0;
	}
	
	int new_block_no;
	
	//printf("block_no=%d,record_po=%d,inode_no=%d\n",block_po,record_po,inode_no);
	//set dir entry info
	retcode = get_new_inode(&new_inode_no);
	if (retcode != 1){//no valid inode
		printf("Error , no valid inode , total=%d,free=%d\n",si.sb->s_blocks_count,si.gd->bg_free_inodes_count);
		return -1;
	}
	dir = (struct ext2_dir_entry *)(si.disk + 1024*(si.inodes[inode_no].i_block[block_po]) +record_po);	//OOOOOO
	name = (char *)(si.disk + 1024*si.inodes[inode_no].i_block[block_po]+record_po +sizeof(struct ext2_dir_entry ));
	memcpy(name,dir_name,strlen(dir_name));
	dir->inode = new_inode_no+1;     /* Inode number */
	dir->name_len = strlen(dir_name);
	dir->rec_len = 1024 - record_po ;   /* Directory entry length ,last record occupy all left*/
	dir->file_type = EXT2_FT_DIR ;

	if (record_po == 0){
	}

	//set inode info
	//retcode = get_new_block(block_po,&new_block_no);
	retcode = get_new_block(&new_block_no);
	si.inodes[new_inode_no].i_mode = EXT2_S_IFDIR;        /* File mode */
	/* Use 0 as the user id for the assignment. */
	si.inodes[new_inode_no].i_uid = 0;         /* Low 16 bits of Owner Uid */
	si.inodes[new_inode_no].i_size = 1024;        /* Size in bytes */
	si.inodes[new_inode_no].i_atime = 0;
	si.inodes[new_inode_no].i_ctime = 0;
	si.inodes[new_inode_no].i_mtime = 0;
	/* d_time must be set when appropriate */
	si.inodes[new_inode_no].i_dtime = 0;
	//time((time_t *)&inodes[new_inode_no].i_dtime);       /* Deletion Time */
	/* Use 0 as the group id for the assignment. */
	si.inodes[new_inode_no].i_gid = 0;         /* Low 16 bits of Group Id */
	si.inodes[new_inode_no].i_links_count = 2; /* Links count */
	si.inodes[new_inode_no].i_blocks = 2;      /* Blocks count IN DISK SECTORS*/
	/* You should set it to 0. */
	si.inodes[new_inode_no].osd1 = 0;          /* OS dependent 1 */
	si.inodes[new_inode_no]. i_block[0] = new_block_no+1;   /* Pointers to blocks *///OOOOOOO
	for (i=1;i<15;i++)
	si.inodes[new_inode_no]. i_block[i] = 0;   /* Pointers to blocks */

	/* You should use generation number 0 for the assignment. */
	si.inodes[new_inode_no]. i_generation = 0;  /* File version (for NFS) */
	/* The following fields should be 0 for the assignment.  */
	si.inodes[new_inode_no]. i_file_acl = 0;    /* File ACL */
	si.inodes[new_inode_no]. i_dir_acl = 0;     /* Directory ACL */
	si.inodes[new_inode_no]. i_faddr = 0;       /* Fragment address */
	si.inodes[new_inode_no]. extra[0] = 0;
	si.inodes[new_inode_no]. extra[1] = 0;
	si.inodes[new_inode_no]. extra[2] = 0;

	new_dir = (struct ext2_dir_entry *)(si.disk + 1024*(new_block_no+1));

	//first set current dir info
	counter  = 0;
	new_dir = (struct ext2_dir_entry *)(si.disk + 1024*(new_block_no+1) +counter  );
	name = (char *)(si.disk + 1024*(new_block_no+1)+sizeof(struct ext2_dir_entry ));
	name[0]='.';
	new_dir->inode = new_inode_no+1;     /* Inode number */
	new_dir->name_len = 1;
	//new_dir->rec_len = sizeof(struct ext2_dir_entry) + sizeof(char*) ;   /* Directory entry length */
	new_dir->rec_len = sizeof(struct ext2_dir_entry) +4;	//2021-12-03
	new_dir->file_type = EXT2_FT_DIR ;
	counter  = new_dir->rec_len;
	//second, set parent dir info
	new_dir = (struct ext2_dir_entry *)(si.disk + 1024*(new_block_no +1)+counter  );
	name = (char *)(si.disk + 1024*(new_block_no +1)+counter+sizeof(struct ext2_dir_entry ));
	name[0]='.';
	name[1]='.';
	new_dir->inode = inode_no +1;     /* Inode number */
	new_dir->name_len = 2;
	//new_dir->rec_len = sizeof(struct ext2_dir_entry) + sizeof(char*) ;   /* Directory entry length */
	new_dir->rec_len = 1024 - counter;
	new_dir->file_type = EXT2_FT_DIR ;
	//set_block_bitmap(new_inode_no);
	
	si.inodes[inode_no]. i_links_count ++;	//2021-12-03
	return new_inode_no;	//succeed
}




int32_t ext2_fsal_mkdir(const char *path)
{
    /**
     * TODO: implement the ext2_mkdir command here ...
     * the argument path is the path to the directory that is to be created.
     */

     /* This is just to avoid compilation warnings, remove this line when you're done. */
    //(void)path;
   	int retcode;
	struct path_analyses  path_ana;

	retcode = analyse_one_path(path, &path_ana);
	if (path_ana.is_valid_path == 0){//
		printf("%s is not a valid path\n",path);
		return ENOENT;
	}
	else if (path_ana.last_is == 1){//last level is file
		printf("%s is a file, can not mkdir \n",path);
		unlock_one_inode(path_ana.parent_inode_no);
		return ENOENT;
	}
	else if (path_ana.last_is == 2){//last level is an existing dir
		printf("The dir:%s exists, can not create againg\n",path);
		unlock_one_inode(path_ana.last_inode_no);
		return  EEXIST;
	}
	else {//create dir 
		retcode = create_one_dir(path_ana.parent_inode_no, path_ana.last_name);
		
		if (retcode  > 0){//successfully create a dir
			si.gd->bg_used_dirs_count++;	//2021-12-03
		}
		retcode = msync((void *)si.disk, 128 * 1024, MS_SYNC);
		unlock_one_inode(path_ana.parent_inode_no);
		
	}
    return 0;
}