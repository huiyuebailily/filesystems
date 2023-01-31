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

#include <unistd.h>
#include <pthread.h>

#ifndef CSC369_E2FS_H
#define CSC369_E2FS_H

#define INODES_NUM	32	

#include "ext2.h"
#include <string.h>

/**
 * TODO: add in here prototypes for any helpers you might need.
 * Implement the helpers in e2fs.c
 */
struct path_analyses {
	unsigned char is_valid_path;	//0:invalid   1:valid path
	unsigned char last_is;	//0:does not exist,  1: file   2:dir
	unsigned char slash_at_end;	//0:does not have slash at end,     1:have slash at end
	int parent_inode_no;	//inode_no of 
	int last_inode_no;		//inode_no of the last level
	int file_len;		//if last level is a file, this is file len, else = 0
	char last_name[256];
};

struct system_info{
	unsigned char *disk;
	struct ext2_super_block *sb;
	struct ext2_group_desc  *gd;
	unsigned char *block_bitmap_p;
	unsigned char *inode_bitmap_p ;
	struct ext2_inode *inodes;


};
struct system_locks_info{
	pthread_mutex_t  glock;	//global lock for modifying sb, gd
	pthread_mutex_t  block;	//lock for modifying  block_bitmap_p
	pthread_mutex_t  ilock;	//lock for modifying  inode_bitmap_p

	pthread_mutex_t  inodelocks[INODES_NUM];	//lock for modifying  inodes
};

struct system_info si;	//system pointers
struct system_locks_info sl;	//locks info


int print_uc(unsigned char uc);
int print_one_inode_detail(int no );
int print_one_inode(int no, struct ext2_inode inode,struct ext2_super_block *sb );
int print_one_dir(int ino, int no, struct ext2_dir_entry dir,char *name);

int get_new_inode(int *new_inode_no);
int get_new_block(int *new_block_no);
int set_inode_bitmap(int inode_no, int flag);
int set_block_bitmap(int block_no, int flag);


int judge_if_one_dir_exist_in_a_inode(int inode_no, char *dir_name,int *rec_len);
int analyse_one_path(const char *path, struct path_analyses  *path_ana);
int get_file_name_from_path(const char *path, char*file_name);
int get_block_position_for_new_dir(int inode_no,int rec_len,int *block_po, int *record_po);

//lock one inode to avoid other modify its content at same time
int lock_one_inode(int inode_no);
//unlock one inode 
int unlock_one_inode(int inode_no);



#endif