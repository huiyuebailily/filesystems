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





//create a new file in the inode(inode_no) , and return the new inode for the new file
//return value:if succeed ,return the new inode_no of this new file
//	0: no free block
//	-1:no free inode
int create_one_file(int inode_no, char *file_name)
{
	int i,block_po;
	int retcode,new_inode_no;
	int record_po;
	struct ext2_dir_entry *dir;
	char *name;
	
	int rec_len;
	rec_len = sizeof(struct ext2_dir_entry) + (strlen(file_name) +sizeof(char*)-1)/sizeof(char*)*sizeof(char*);
	retcode = get_block_position_for_new_dir(inode_no,rec_len, &block_po,&record_po);
	if (retcode < 1){//fail to get block no
		return -1;
	}
	
	int new_block_no;	
	//printf("block_no=%d,record_po=%d,inode_no=%d\n",block_po,record_po,inode_no);
	retcode = get_new_inode(&new_inode_no);	

	if (retcode != 1){//no valid inode
		printf("Error , no valid inode , total=%d,free=%d\n",si.sb->s_blocks_count,si.gd->bg_free_inodes_count);
		return -1;
	}
	dir = (struct ext2_dir_entry *)(si.disk + 1024*(si.inodes[inode_no].i_block[block_po]) +record_po);	//OOOOOOO
	name = (char *)(si.disk + 1024*(si.inodes[inode_no].i_block[block_po])+record_po +sizeof(struct ext2_dir_entry ));//OOOOOOO
	memcpy(name,file_name,strlen(file_name));
	dir->inode = new_inode_no+1;     /* Inode number */
	dir->name_len = strlen(file_name);
	dir->rec_len = 1024 - record_po ;   /* Directory entry length ,last record occupy all left*/
	dir->file_type = EXT2_FT_REG_FILE ;

	if (record_po == 0){
	}

	//set inode info
	//retcode = get_new_block(block_po,&new_block_no);
	retcode = get_new_block(&new_block_no);
	si.inodes[new_inode_no].i_mode = EXT2_S_IFREG;        /* File mode */
	/* Use 0 as the user id for the assignment. */
	si.inodes[new_inode_no].i_uid = 0;         /* Low 16 bits of Owner Uid */
	si.inodes[new_inode_no].i_size = 1024;        /* Size in bytes , will be changed later*/
	si.inodes[new_inode_no].i_atime = 0;
	si.inodes[new_inode_no].i_ctime = 0;
	si.inodes[new_inode_no].i_mtime = 0;
	/* d_time must be set when appropriate */
	//time((time_t *)&si.inodes[new_inode_no].i_dtime);       /* Deletion Time */
	si.inodes[new_inode_no].i_dtime = 0;
	/* Use 0 as the group id for the assignment. */
	si.inodes[new_inode_no].i_gid = 0;         /* Low 16 bits of Group Id */
	si.inodes[new_inode_no].i_links_count = 0; /* Links count */
	si.inodes[new_inode_no].i_blocks = 2;      /* Blocks count IN DISK SECTORS*/
	/* You should set it to 0. */
	si.inodes[new_inode_no].osd1 = 0;          /* OS dependent 1 */
	
	si.inodes[new_inode_no]. i_block[0] = new_block_no+1;   /* Pointers to blocks *///OOOOOOOOOOO
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

	return new_inode_no;	//succeed
}

//If des file has already existed, and it's length is bigger than what we need, we will free them
int free_left_over_space(int file_len,int dst_file_inode_no)
{
	int k,dno;
	int need_block_num;

	need_block_num = (file_len + 1023)/1024*2;
	dno = dst_file_inode_no;
	if (need_block_num >= si.inodes[dno].i_blocks )
	return 0;

	for (k=need_block_num/2 ;k< si.inodes[dno].i_blocks/2 && k<12;k++){
		if (si.inodes[dno].i_block[k] != 0){
			set_block_bitmap(si.inodes[dno].i_block[k] ,0);
			si.inodes[dno].i_block[k] = 0;
		}
	}

	if (need_block_num <=22 && si.inodes[dno].i_block[12] != 0){//delete first indirect block
		int *dir_block;
		dir_block =  (int *)(si.disk + 1024*si.inodes[dno].i_block[12] );
		for (k=0;k<256;k++)
		if (dir_block[k] > 0){
			set_block_bitmap(dir_block[k],0);
		}
		set_block_bitmap(si.inodes[dno].i_block[12],0);
		si.inodes[dno].i_block[12] = 0;
	}
	si.inodes[dno].i_blocks = need_block_num;
	si.inodes[dno].i_size = file_len;

	return 0;
}


int copy_inode_data(int file_len,	//file data length
	char *file_buf , 		//src file data
	int dst_file_inode_no)
{
	int i,dno;
	int retcode,counter;
	int new_block_no;
	char *src_addr,*dst_addr;

	 free_left_over_space(file_len,dst_file_inode_no);	//free space if need

	//sno = src_file_inode_no;
	dno = dst_file_inode_no;

	//si.inodes[dno].i_size = 1024;	//file size
	//si.inodes[dno].i_blocks =si.inodes[sno].i_blocks;	//f
	//si.inodes[dno]. i_links_count = si.inodes[sno]. i_links_count;	
	//si.inodes[dno]. i_links_count = si.inodes[sno]. i_links_count;	//file size
	i = 0;
	counter = 0;
	//for (i=0;i< inodes[sno].i_blocks/2;i++){
	while (counter  <file_len && counter < 12288){	//save first 12 direct block
		if (si.inodes[dno].i_block[i] == 0){
			retcode= get_new_block(&new_block_no);
			if (retcode != 1){
				return ENOSPC;
			}
			si.inodes[dno].i_block[i] = new_block_no+1;//OOOOOOOOOOOO
		}

		src_addr =(char *)(file_buf + counter);
		dst_addr =(char *)( si.disk + 1024*(si.inodes[dno].i_block[i]));	//OOOOOOO
		if (counter + 1024 <= file_len){
			counter +=1024;
			memcpy(dst_addr ,src_addr ,1024);
			
		}
		else{
			memcpy(dst_addr ,src_addr ,file_len -counter );
			counter = file_len  ;
		}
		i++;
	}
	si.inodes[dno].i_atime =0;
	si.inodes[dno].i_ctime=0;
	si.inodes[dno].i_mtime =0;
	si.inodes[dno].i_dtime =0;
	si.inodes[dno].i_size = file_len;
	si.inodes[dno]. i_links_count =1;
	si.inodes[dno]. i_blocks  =(file_len+1023)/1024*2;


	//request 12 first level indirect block
	if (counter < file_len){
		int *dir_block;
		retcode= get_new_block(&new_block_no);
		if (retcode <= 0){
			 return ENOSPC;
		}
		si.inodes[dno].i_block[12] = new_block_no+1;
		dir_block=(int *)(si.disk + 1024*(new_block_no+1));	//OOOOOOO
		//memset((char *)dir_block, 0, 1024);
		for (i=0;i<256;i++)
		dir_block[i] = 0;
		i = 0;
		while (counter  <file_len ){	//save first 12 direct block
			
			retcode= get_new_block(&new_block_no);
			if (retcode != 1){
				return ENOSPC;
			}
			dir_block[i] = new_block_no+1;
			src_addr =(char *)(file_buf + counter);
			dst_addr =(char *)( si.disk + 1024*(new_block_no+1));	//OOOOOOO
			if (counter + 1024 <= file_len){
				counter +=1024;
				memcpy(dst_addr ,src_addr ,1024);
			}
			else{
				memcpy(dst_addr ,src_addr ,file_len -counter );
				counter = file_len  ;
			}
			i++;
			//dir_block[i] = 0;
		}
		si.inodes[dno]. i_blocks += 2;
	}

	return 0;
}


int cp_a_file(int file_len,			//file data length
	char *file_buf , 			//src file data
 	int dst_dir_inode_no,int dst_file_inode_no,char *dst_file_name)
{
	int retcode;
	//int new_block_no;

	//if (dst_file_inode_no <= 0){
	//	get_new_inode(&dst_file_inode_no);
	//}
	if (dst_file_inode_no < 0)
	dst_file_inode_no = create_one_file(dst_dir_inode_no, dst_file_name);
	if (dst_file_inode_no < 0)
	return ENOSPC;

	retcode = copy_inode_data(file_len,file_buf, dst_file_inode_no);
	
	return retcode;
}

int32_t ext2_fsal_cp(const char *src,
                     const char *dst)
{
    /**
     * TODO: implement the ext2_cp command here ...
     * Arguments src and dst are the cp command arguments described in the handout.
     */

     /* This is just to avoid compilation warnings, remove these 2 lines when you're done. */
    //(void)src;
    //(void)dst;
    	char *file_buf;
	int retcode;
	time_t cur_time;
	struct path_analyses  dst_ana;//src_ana, 

	time(&cur_time);
	//2021-12-03
	int file_len;
	FILE *file = fopen(src,"rb");
	if ( !file ){
		printf("File %s does not exist , can not cp\n",src);
   		return ENOENT;
	}
	fseek(file,0,SEEK_END);
	file_len= ftell(file);
	file_buf = (char *)malloc(file_len);
	fseek(file,0,SEEK_SET);
	retcode = fread(file_buf ,sizeof(char), file_len, file);
	if (retcode != file_len){
		printf("Fail to read file %s  , can not cp\n",src);
   		return ENOENT;
	}
	fclose(file);
	
	char file_name[256];
	get_file_name_from_path(src,file_name);
	analyse_one_path(dst, &dst_ana);

	if (dst_ana.is_valid_path != 1 ){
		//dst is not a valid path
		printf("%s is not a valid path, can not cp.\n", dst);
		return ENOENT;
	}
	if (dst_ana.last_is == 1 &&  dst_ana.slash_at_end == 1){
		//last level of dst path is a file , but there is a slash at the end , it is not a valid path
		printf("%s is not a valid path, can not cp.\n", dst);
		unlock_one_inode(dst_ana.parent_inode_no);
		return ENOENT;
	}
	if (dst_ana.last_is == 2 ){
		//last level of dst path is a dir ,  need re analyse to judge the file is already exists
		char * new_dst;
		new_dst = (char *)malloc(strlen(dst)+ strlen(file_name)+3);
		strcpy(new_dst ,dst);
		if (dst_ana.slash_at_end == 0) strcat(new_dst ,"/");
		strcat(new_dst,file_name);
		//unlock_one_inode(dst_ana.parent_inode_no);	
		unlock_one_inode(dst_ana.last_inode_no);	
		analyse_one_path(new_dst, &dst_ana);

		unlock_one_inode(dst_ana.parent_inode_no);

	}
	
	//judge if there is enough space
	int need_block_num;
	need_block_num = 0;
	if (dst_ana.last_is == 1 ){
		if (file_len > dst_ana.file_len){
			need_block_num = (file_len -dst_ana.file_len+1023)/1024*2;
			if (file_len > 12288)
			need_block_num  += 2;	//add one indirect block
		}
	}
	else {
		need_block_num = (file_len +1023)/1024*2;
		if (file_len > 12288)
		need_block_num  += 2;	//add one indirect block
	}
	if (need_block_num   > si.gd->bg_free_blocks_count){
		unlock_one_inode(dst_ana.parent_inode_no);
		printf("File len=%d(%d),need block num=%d(%d), no enough space, can not cp file\n",file_len,
			dst_ana.file_len,need_block_num ,si.gd->bg_free_blocks_count);
		return ENOSPC;
	}
	retcode = cp_a_file(file_len,		//file data length
		file_buf , 			//src file data
		dst_ana.parent_inode_no,	//inode_no of dest path last level
		dst_ana.last_inode_no,	//dest file's inode_no, need create
		dst_ana.last_name);		//dest file name , same as src
	retcode = msync((void *)si.disk, 128 * 1024, MS_SYNC);
	unlock_one_inode(dst_ana.parent_inode_no);

	free(file_buf );
	
    	return 0;
}