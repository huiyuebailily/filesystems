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

/**
 * TODO: Make sure to add all necessary includes here
 */
#include <stdio.h>
#include "e2fs.h"

 /**
  * TODO: Add any helper implementations here
  */
int print_uc(unsigned char uc){
    int j;
     for ( j=0;j<8;j++){
	if (uc & 1) printf("1");
	else printf("0");
	uc >>= 1;
            }
        printf("%s"," ");
    return 1;
}

int print_one_inode_detail(int no ){
	int i,num;
	char t;

	//struct ext2_inode *inodes =  (struct ext2_inode*)(si.disk + 5120);

 	if ((si.inodes[no].i_mode & EXT2_S_IFLNK) == EXT2_S_IFLNK ) {
		t = 'l';
    	}
    	else if (si.inodes[no].i_mode & EXT2_S_IFREG ) {
         		if (si.inodes[no].i_size == 0) return 0;
		t = 'f';
  	 }
    	else if (si.inodes[no].i_mode & EXT2_S_IFDIR) t = 'd';
    	else return 0;

    
    	printf("inode_no= %d type: %c size: %d links: %d blocks: %d\nBlocks:", no+1, t,si.inodes[no].i_size,si.inodes[no].i_links_count,si.inodes[no].i_blocks);
    	num = (1024<<si.sb->s_log_block_size)/512;
    	for (i=0;i<si.inodes[no].i_blocks / num ;i++)
		printf(" %d",si.inodes[no].i_block[i]);
 	printf("\n");
    return 1;
}


int print_one_inode(int no, struct ext2_inode inode,struct ext2_super_block *sb ){
	int i,num;
	int block_num;
	char t;

	if ((inode.i_mode & EXT2_S_IFLNK) == EXT2_S_IFLNK ) {
		t = 'l';
	}
	else if (inode.i_mode & EXT2_S_IFREG ) {
 		if (inode.i_size == 0) return 0;
		t = 'f';
		}
	else if (inode.i_mode & EXT2_S_IFDIR) t = 'd';
	else return 0;
    
	printf("[%d] type: %c size: %d links: %d blocks: %d\nBlocks:", no+1, t, inode.i_size, inode.i_links_count,inode.i_blocks);
	num = (1024<<sb->s_log_block_size)/512;
   	if (t == 'f'){
		block_num = (inode.i_size+1023)/1024;
	}
	else {
		block_num = inode.i_blocks / num;
	}
    	for (i=0;i<block_num  ;i++)
 		printf(" %d",inode.i_block[i]);
	printf("\n");
	//printf("DDDDDD \i_blocks=%d,num=%d,i_size=%d,s_log_block_size=%d\n",inode.i_blocks,num,inode.i_size,sb->s_log_block_size);
	return 1;
}
#if 0 
int print_one_dir(int ino, int no, struct ext2_dir_entry dir,char *name){
    if  (dir.rec_len <= 0)
    return 0;
    char t;
    if ((dir.file_type & EXT2_FT_SYMLINK    )==EXT2_FT_REG_FILE ) t = 'f';
    else if ((dir.file_type & EXT2_FT_SYMLINK    )==EXT2_FT_DIR ) t = 'd';
    else if ((dir.file_type & EXT2_FT_SYMLINK     )==EXT2_FT_SYMLINK   ) t = 'l';
    else {
	//if (no > 10)
	return 0;
    }
    //printf("    DIR BLOCK NUM: %d (for inode %d)\n", no+1, ino+1);

    printf("Inode: %d rec_len: %d name_len: %d type= %c name=%.*s\n",dir.inode,dir.rec_len,dir.name_len,t,dir.name_len, name);
    return 1;
}
#endif
//lock one inode to avoid other modify its content at same time
int lock_one_inode(int inode_no)
{
	pthread_mutex_lock(&sl.inodelocks[inode_no]);
	return 0;
}
//unlock one inode 
int unlock_one_inode(int inode_no)
{
	pthread_mutex_unlock(&sl.inodelocks[inode_no]);
	return 0;
}
int modify_free_inode_num(int delta)
{//
	pthread_mutex_lock(&sl.glock);
	si.gd->bg_free_inodes_count  +=  delta;
	si.sb->s_free_inodes_count  +=  delta ;
	if (si.gd->bg_free_inodes_count  < 0)
	si.gd->bg_free_inodes_count  = 0;
	else if (si.gd->bg_free_inodes_count  > si.sb->s_inodes_count)
	si.gd->bg_free_inodes_count  =  si.sb->s_inodes_count;

	if (si.sb->s_free_inodes_count < 0)
	si.sb->s_free_inodes_count = 0;
	else 	if (si.sb->s_free_inodes_count > si.sb->s_inodes_count)
	si.sb->s_free_inodes_count = si.sb->s_inodes_count;

	pthread_mutex_unlock(&sl.glock);

	return 1;
}

int modify_free_block_num(int delta)
{//
	pthread_mutex_lock(&sl.glock);
	si.gd->bg_free_blocks_count  +=  delta;
	si.sb->s_free_blocks_count  +=  delta ;
	if (si.gd->bg_free_blocks_count  < 0)
	si.gd->bg_free_blocks_count  = 0;
	else if (si.gd->bg_free_blocks_count  > si.sb->s_blocks_count)
	si.gd->bg_free_blocks_count  =  si.sb->s_blocks_count;

	if (si.sb->s_free_blocks_count < 0)
	si.sb->s_free_blocks_count = 0;
	else 	if (si.sb->s_free_blocks_count > si.sb->s_blocks_count)
	si.sb->s_free_blocks_count= si.sb->s_blocks_count;

	pthread_mutex_unlock(&sl.glock);

	return 1;
}
int get_new_block(int *new_block_no)
{
	int i,j,block_no;
	int start_block_no;
	//int need_find_again;
	unsigned char uc;

	if (si.gd->bg_free_blocks_count == 0)
	return 0;	//no free block
	
	pthread_mutex_lock(&sl.block);
	start_block_no = -1;
	//need_find_again = 1;
     	for (block_no =16;block_no <si.sb->s_blocks_count ;block_no ++){
		i= block_no / 8;
		j = block_no % 8;
		uc = 1;
		uc <<= j;
		if ((si.block_bitmap_p[i] & uc) == 0 ){//2021-12-05
			start_block_no =block_no;
			break;
		}
#if 0
//2021-12-03
		if ((si.block_bitmap_p[i] & uc) == 0 && need_find_again  == 1){
			start_block_no =block_no;
			need_find_again = 0;
		}
		else if ((si.block_bitmap_p[i] & uc) == 1){
			need_find_again = 1;
		}
#endif
	}
	if (start_block_no  == -1){
		pthread_mutex_unlock(&sl.block);
		return 0;	//no free block
	}
	i = start_block_no / 8;
	j = start_block_no % 8;
	uc = 1;
	uc <<= j;
	*new_block_no =start_block_no   ;
	si.block_bitmap_p[i] |= uc;
	//si.gd->bg_free_blocks_count -- ;
	//si.sb->s_free_blocks_count--;	//2021-12-03
	 modify_free_block_num(-1);	//2021-12-05
	pthread_mutex_unlock(&sl.block);
	return 1;
}

int get_new_inode(int *new_inode_no)
{
	int i,j,inode_no;
	int start_inode_no;//,need_find_again;
	unsigned char uc;

	if (si.gd->bg_free_inodes_count == 0)
	return 0;	//no free inode
	start_inode_no = -1;
	//need_find_again = 1;
     	for ( inode_no =si.sb->s_first_ino;inode_no <si.sb->s_inodes_count ;inode_no ++){
		i= inode_no  / 8;
		j = inode_no % 8;
		uc = 1;
		uc <<= j;
		if ((si.inode_bitmap_p[i] & uc) == 0 ){//2021-12-05
			start_inode_no = inode_no;
			break;
		}
#if 0
//2021-12-03
		if ((si.inode_bitmap_p[i] & uc) == 0 && need_find_again  == 1){
			start_inode_no = inode_no;
			need_find_again = 0;
		}
		else if ((si.inode_bitmap_p[i] & uc) == 1){
			need_find_again = 1;
		}
#endif
	}
	if (start_inode_no == -1){
		return 0;	//no free inode
	}
	i = start_inode_no   / 8;
	j = start_inode_no  % 8;
	uc = 1;
	uc <<= j;
	*new_inode_no =start_inode_no   ;
	si.inode_bitmap_p[i] |= uc;
	//si.gd->bg_free_inodes_count -- ;
	//si.sb->s_free_inodes_count -- ;		//2021-12-03
	modify_free_inode_num(-1);			//2021-12-05
	return 1;
}


//If the dir exist in one inode 
//return value:
//0: the dir doest not exist
//>0: the dir exists
//<0: it is a file , and return value is 0-inode_no
int judge_if_one_dir_exist_in_a_inode(int inode_no, char *dir_name,int *rec_len)
{	//rec_len :only for a file
	int j,l,num,counter;
	struct ext2_dir_entry *dir;
	char *name;

    	//struct ext2_inode *inodes =  (struct ext2_inode*)(si.disk + 5120);
	num = (1024<<si.sb->s_log_block_size)/512;
	*rec_len = 0;
	for (j=0;j<si.inodes[inode_no].i_blocks / num;j++){
		l =0;
		counter =0;
		while (counter  < 1024){
			dir = (struct ext2_dir_entry *)(si.disk + 1024*si.inodes[inode_no].i_block[j] +counter );
			name = (char *)(si.disk + 1024*si.inodes[inode_no].i_block[j] + counter+sizeof(struct ext2_dir_entry ));
			if (!strncmp(name ,dir_name,dir->name_len) && dir->name_len == strlen(dir_name)){
				if ((dir->file_type & EXT2_FT_SYMLINK    )==EXT2_FT_DIR ) 
					return dir->inode-1;	//dir exist
				else {
					*rec_len = si.inodes[dir-> inode].i_size;
					return 1-dir->inode;		//exist but not dir
				}
			}
			//ret= print_one_dir(1, j, *dir,name);
			 l++;
			counter += dir->rec_len;
		}
	 }

	return 0;	//do not exist
}



int analyse_one_path(const char *path, struct path_analyses  *path_ana)
{	
	int	start_po,len;
	int	counter,file_len;
	int	inode_no;
	//int	last_dir;		//1: is dir    2:is file    3: not exist
	//int	path_level;	//0: root dir, then increase by 1 when go into subsir
	char	path1[256];
	//char	*tmp_path;
	//char	**dir_names;
	len = strlen(path);
	pthread_mutex_lock(&sl.ilock);
	if (path[0] == '/' && len ==1){	// '/' root dir
		path_ana->is_valid_path = 1;
		path_ana->parent_inode_no = 1;	//inode_no of 
		path_ana->last_inode_no = -1;	//inode_no of the last level
		lock_one_inode(path_ana->parent_inode_no);
		pthread_mutex_unlock(&sl.ilock);
		return 1;
	}
	if (path[0] != '/' || len < 2 ){	//invalid path
		path_ana->is_valid_path = 0;
		pthread_mutex_unlock(&sl.ilock);
		return 0;
	}
	path_ana->is_valid_path = 1;	
	if (path[len-1] == '/')
		path_ana->slash_at_end  = 1;	//0:does not have slash at end,     1:have slash at end
	else 	path_ana->slash_at_end  = 0;	//0:does not have slash at end,     1:have slash at end
	path_ana->parent_inode_no = 1;	//inode_no of 
	path_ana->last_inode_no = -1;		//inode_no of the last level
	path_ana->last_name[0] = 0;
	path_ana->file_len = 0;
	inode_no = 1;			//first time search root dir
	
	//i = 0;
	counter = 0;
	//last_dir = 1;			//1: is dir    2:is file    3: not exist
	while (counter<len ){

		if (path[counter ] == '/') {
			//tmp_path[counter ] = 0;
			counter ++;
		}
		else {
	        		//dir_names[i] = tmp_path + counter;
	        		start_po = counter;
	        		while (counter< len  && path[counter ] != '/') counter ++;
	        		memcpy(path1,path +start_po ,counter - start_po);
	        		path1[counter - start_po] = '\0';
			strcpy(path_ana->last_name,path1);
	       	 	counter ++;

			if (inode_no < 1){	//upper level is not a valid dir , so the path is invalid
				path_ana->is_valid_path = 0;	
				pthread_mutex_unlock(&sl.ilock);
				return 0;
			}
			path_ana->parent_inode_no = inode_no ;
			inode_no = judge_if_one_dir_exist_in_a_inode( inode_no, path1,&file_len);
	    	}
	}
	if (inode_no == 0) {	//last level does not exist
		//path_ana->parent_inode_no = inode_no ;
		path_ana->last_is = 0;			//0:does not exist,  1: file   2:dir
		lock_one_inode(path_ana->parent_inode_no);
		pthread_mutex_unlock(&sl.ilock);
		return 1;
	}
	else if (inode_no > 0) {	//full path exists
		path_ana->last_inode_no = inode_no ;
		path_ana->last_is = 2;			//0:does not exist,  1: file   2:dir
		lock_one_inode(path_ana->last_inode_no);
		pthread_mutex_unlock(&sl.ilock);
		return 1;
	}
	else {	//last exists but not dir
		path_ana->last_inode_no = 0 -inode_no ;
		path_ana->last_is = 1;			//0:does not exist,  1: file   2:dir
		path_ana->file_len = file_len;			//last level if a file
		lock_one_inode(path_ana->parent_inode_no);
		pthread_mutex_unlock(&sl.ilock);

		return 1;
	}
 	return 0;
}

int get_file_name_from_path(const char *path, char*file_name)
{	
	int	start_po,len;
	int	counter;
	//int	inode_no;
	char	path1[256];

	len = strlen(path);
	file_name[0] = 0;
	//i = 0;
	counter = 0;
	while (counter<len ){
		if (path[counter ] == '/') {
			counter ++;
		}
		else {
	        		start_po = counter;
	        		while (counter< len  && path[counter ] != '/') counter ++;
	        		memcpy(path1,path +start_po ,counter - start_po);
	        		path1[counter - start_po] = '\0';
			strcpy(file_name,path1);
	       	 	counter ++;
	    	}
	}

 	return 1;
}


int set_inode_bitmap(int inode_no, int flag)
{
	int i,j;
	unsigned char uc;

	i = inode_no  / 8;
	j = inode_no % 8;
	uc = 1;
	uc <<= j;
	if (flag == 1){
		si.inode_bitmap_p[i] |= uc;
		//si.gd->bg_free_inodes_count --;
		//si.sb->s_free_inodes_count -- ;		//2021-12-03
		modify_free_inode_num(-1);			//2021-12-05
	}
	else{
	 	si.inode_bitmap_p[i] &= (~uc);
		//si.gd->bg_free_inodes_count ++;
		//si.sb->s_free_inodes_count++ ;		//2021-12-03
		modify_free_inode_num(1);			//2021-12-05
	}
	
	return 1;
}

int set_block_bitmap(int block_no6, int flag)
{
	int i,j;
	unsigned char uc;

	int block_no = block_no6 -1;	//OOOOOO
	i = block_no  / 8;
	j = block_no % 8;
	uc = 1;
	uc <<= j;
	if (flag == 1){
		si.block_bitmap_p[i] |= uc;
		//si.gd->bg_free_blocks_count --;
		//si.sb->s_free_blocks_count--;	//2021-12-03
		modify_free_block_num(-1);	//2021-12-05
	}
	else{
	 	si.block_bitmap_p[i] &= (~uc);
		//si.gd->bg_free_blocks_count++;
		//si.sb->s_free_blocks_count++;	//2021-12-03
		modify_free_block_num(1);	//2021-12-05
	}
	
	return 1;
}

int get_block_position_for_new_dir(int inode_no,int rec_len,int *block_po, int *record_po)
{
	int j,num,counter;
	int retcode;
	int new_block_po;
	//int old_block_po;
	struct ext2_dir_entry *dir;
	//char *name;

    	//struct ext2_inode *inodes =  (struct ext2_inode*)(disk + 5120);
	num = (1024<<si.sb->s_log_block_size)/512;
	j = si.inodes[inode_no].i_blocks / num -1;	//find the last block
	
	//old_block_po= 16;
	//find the tail of this block
	if (j < 14){
		counter =0;
		while (counter  < 1024){
			dir = (struct ext2_dir_entry *)(si.disk + 1024*si.inodes[inode_no].i_block[j] +counter );
			//old_block_po=  si.inodes[inode_no].i_block[j] ;
			if (counter + dir->rec_len == 1024){//last record 
				int cur_real_rec_len = sizeof(struct ext2_dir_entry ) + (dir->name_len+3)/4*4;
				if (cur_real_rec_len  + counter < 1024){//enough space
					dir->rec_len =cur_real_rec_len ;
					*block_po = j ;
					*record_po = counter + dir->rec_len ;
					return inode_no;
				}
				else {//TODO need new block
				}

			}
			//j++;
			counter += dir->rec_len;
		 }
		//TODO
		retcode = get_new_block(&new_block_po);
		if (retcode == 1){
			*block_po =j+1;
			*record_po = 0;
			si.inodes[inode_no].i_blocks +=2;
			si.inodes[inode_no].i_block[j+1]  = new_block_po+1;//OOOOOO
			return inode_no;
		}
	}


	return 0;	//no available space
}


  // .....