#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

#include <string.h>

struct path_analyses {
	unsigned char is_valid_path;	//0:invalid   1:valid path
	unsigned char last_is;	//0:does not exist,  1: file   2:dir
	unsigned char slash_at_end;	//0:does not have slash at end,     1:have slash at end
	int parent_inode_no;	//inode_no of 
	int last_inode_no;		//inode_no of the last level
	char last_name[256];
};



unsigned char *disk;
struct ext2_super_block *sb;
struct ext2_group_desc  *gd;
unsigned char *block_bitmap_p;
unsigned char *inode_bitmap_p;
struct ext2_inode *inodes;
int print_uc(unsigned char uc){
    int j;
     for ( j=0;j<8;j++){
	if (uc & 1) printf("1");
	else printf("0");
	uc >>= 1;
            }
        printf(" ");
    return 1;
}

int print_one_inode(int no, struct ext2_inode inode,struct ext2_super_block *sb ){
    int i,num;
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
    for (i=0;i<inode.i_blocks / num ;i++)
    printf(" %d",inode.i_block[i]);
    printf("\n");
    return 1;
}
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
  
    printf("Inode: %d rec_len: %d name_len: %d type= %c name=%.*s\n",dir.inode,dir.rec_len,dir.name_len,t,dir.name_len, name);
    return 1;
}

int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

   sb = (struct ext2_super_block *)(disk + 1024);
    gd =  (struct ext2_group_desc   *)(disk + 2048);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);
    printf("Block group:\n");
    printf("    block bitmap: %d\n", gd->bg_block_bitmap);
    printf("    inode bitmap: %d\n", gd->bg_inode_bitmap);
    printf("    inode table: %d\n", gd->bg_inode_table);
    printf("    free blocks: %d\n", gd->bg_free_blocks_count);
    printf("    free inodes: %d\n", gd->bg_free_inodes_count);
    printf("    used_dirs: %d\n", gd->bg_used_dirs_count);

    printf("Block bitmap: ");
    block_bitmap_p = (unsigned char *)(disk + 3072);
    inode_bitmap_p = (unsigned char *)(disk + 4096);
    int i,j;
    for (i=0;i<sb->s_blocks_count/8;i++){
       //printf(" %b", block_bitmap_p[i]);
       print_uc(block_bitmap_p[i]);
    }
    printf("\nInode bitmap: ");
    for (i=0;i<sb->s_inodes_count/8;i++){
       print_uc(inode_bitmap_p[i]);
    }
    

    printf("\n\nInodes:\n");
    i = 2;
    inodes =  (struct ext2_inode*)(disk + 5120);
    print_one_inode(1, inodes[1],sb);

    for (i=sb->s_first_ino;i<sb->s_inodes_count;i++){
	print_one_inode(i,inodes[i],sb);
    }

    
    printf("\nDirectory Blocks:\n");
    int k,l,num,unit_len,counter,ret;
    struct ext2_dir_entry *dir;
    char *name;
    


   unit_len = sizeof(struct ext2_dir_entry )+ EXT2_NAME_LEN;
   unit_len  = 256;
   num = (1024<<sb->s_log_block_size)/512;
   for (j=0;j<inodes[1].i_blocks / num;j++){
	//dirs = (struct ext2_dir_entry *)(disk + 1024*inodes[1].i_block[j]);
	l =0;
	counter =0;
	printf("    DIR BLOCK NUM: %d (for inode %d)\n", inodes[1].i_block[j],2);
	while (counter  < 1024){
		    dir = (struct ext2_dir_entry *)(disk + 1024*inodes[1].i_block[j] +counter );
		    name = (char *)(disk + 1024*inodes[1].i_block[j] + counter+sizeof(struct ext2_dir_entry ));
	        	    ret= print_one_dir(1, j, *dir,name);
		    l++;
		  counter += dir->rec_len;

	}
    }
    

    unsigned short mode;
    for (i=sb->s_first_ino;i<sb->s_inodes_count;i++){
	//print_one_inode(i,inodes[i],sb);
	mode = inodes[i].i_mode & 0xF000;

         	if (mode  == EXT2_S_IFDIR) {
             	    for (j=0;j<inodes[i].i_blocks / num;j++){
		l =0;
		counter =0;
		printf("    DIR BLOCK NUM: %d (for inode %d)\n", inodes[i].i_block[j],i+1);
		while (counter  < 1024){
			dir = (struct ext2_dir_entry *)(disk + 1024*inodes[i].i_block[j] +counter );
			name = (char *)(disk + 1024*inodes[i].i_block[j] + counter+sizeof(struct ext2_dir_entry ));
			ret= print_one_dir(i, j, *dir,name);
			l++;
			counter += dir->rec_len;
		}

	}
         }
    }
    

	//cp_a_file(16, 12,-1,"afile");
	//cp_a_file(16, 15,-1,"afile");
	//cp_a_file(17, 13,-1,"e2fs.c");
	//ext2_fsal_rm("/afile");
	ext2_fsal_rm("/level1/level2/bfile");
msync((void *)disk, 128 * 1024, MS_SYNC);
    return 0;

}





int get_new_block(int *new_block_no)
{
	int i,j,block_no;
	int start,start_block_no;
	int need_find_again;
	unsigned char uc;

	if (gd->bg_free_blocks_count == 0)
	return 0;	//no free block
	

	start_block_no = -1;
	need_find_again = 1;
     	for (block_no =16;block_no <sb->s_blocks_count ;block_no ++){
		i= block_no / 8;
		j = block_no % 8;
		uc = 1;
		uc <<= j;

		if ((block_bitmap_p[i] & uc) == 0 && need_find_again  == 1){
			start_block_no =block_no;
			need_find_again = 0;
		}
		else if ((block_bitmap_p[i] & uc) == 1){
			need_find_again = 1;
		}
	}
	if (start_block_no  == -1){
		return 0;	//no free block
	}
	i = start_block_no / 8;
	j = start_block_no % 8;
	uc = 1;
	uc <<= j;
	*new_block_no =start_block_no   ;
	block_bitmap_p[i] |= uc;
	gd->bg_free_blocks_count -- ;

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
		inode_bitmap_p[i] |= uc;
		gd->bg_free_inodes_count ++;
	}
	else{
	 	inode_bitmap_p[i] &= (~uc);
		gd->bg_free_inodes_count  --;
	}
	
	return 1;
}

int set_block_bitmap(int block_no, int flag)
{
	int i,j;
	unsigned char uc;

	i = block_no  / 8;
	j = block_no % 8;
	uc = 1;
	uc <<= j;
	if (flag == 1){
		block_bitmap_p[i] |= uc;
		gd->bg_free_blocks_count ++;
	}
	else{
	 	block_bitmap_p[i] &= (~uc);
		gd->bg_free_blocks_count --;
	}
	
	return 1;
}


int get_new_inode(int *new_inode_no)
{
	int i,j,inode_no;
	int start_inode_no,need_find_again;
	unsigned char uc;

	if (gd->bg_free_inodes_count == 0)
	return 0;	//no free inode
	start_inode_no = -1;
	need_find_again = 1;
     	for ( inode_no =sb->s_first_ino;inode_no <sb->s_inodes_count ;inode_no ++){
		i= inode_no  / 8;
		j = inode_no % 8;
		uc = 1;
		uc <<= j;

		if ((inode_bitmap_p[i] & uc) == 0 && need_find_again  == 1){
			start_inode_no = inode_no;
			need_find_again = 0;
		}
		else if ((inode_bitmap_p[i] & uc) == 1){
			need_find_again = 1;
		}
	}
	if (start_inode_no == -1){
		return 0;	//no free inode
	}
	i = start_inode_no   / 8;
	j = start_inode_no  % 8;
	uc = 1;
	uc <<= j;
	*new_inode_no =start_inode_no   ;
	inode_bitmap_p[i] |= uc;
	gd->bg_free_inodes_count -- ;
	return 1;
}


//If the dir exist in one inode 
//return value:
//0: the dir doest not exist
//>0: the dir exists
//<0: it is a file , and return value is 0-inode_no
int judge_if_one_dir_exist_in_a_inode(int inode_no, char *dir_name)
{
	int j,l,num,counter;
	struct ext2_dir_entry *dir;
	char *name;

    	struct ext2_inode *inodes =  (struct ext2_inode*)(disk + 5120);
	num = (1024<<sb->s_log_block_size)/512;

	for (j=0;j<inodes[inode_no].i_blocks / num;j++){
		l =0;
		counter =0;
		while (counter  < 1024){
			dir = (struct ext2_dir_entry *)(disk + 1024*inodes[inode_no].i_block[j] +counter );
			name = (char *)(disk + 1024*inodes[inode_no].i_block[j] + counter+sizeof(struct ext2_dir_entry ));
			if (!strncmp(name ,dir_name,dir->name_len) && dir->name_len == strlen(dir_name)){
				if ((dir->file_type & EXT2_FT_SYMLINK    )==EXT2_FT_DIR ) 
					return dir->inode-1;	//dir exist
				else return 1-dir->inode;		//exist but not dir
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
	int	i,counter;
	int	inode_no;
	//int	last_dir;		//1: is dir    2:is file    3: not exist
	//int	path_level;	//0: root dir, then increase by 1 when go into subsir
	char	path1[256];
	//char	*tmp_path;
	//char	**dir_names;
	len = strlen(path);
	
	if (path[0] == '/' && len ==1){	// '/' root dir
		path_ana->is_valid_path = 1;
		path_ana->parent_inode_no = 1;	//inode_no of 
		path_ana->last_inode_no = -1;	//inode_no of the last level
		return 1;
	}
	if (path[0] != '/' || len < 2 ){	//invalid path
		path_ana->is_valid_path = 0;
		return 0;
	}
	path_ana->is_valid_path = 1;	
	if (path[len-1] == '/')
		path_ana->slash_at_end  = 1;	//0:does not have slash at end,     1:have slash at end
	else 	path_ana->slash_at_end  = 0;	//0:does not have slash at end,     1:have slash at end
	path_ana->parent_inode_no = 1;	//inode_no of 
	path_ana->last_inode_no = -1;	//inode_no of the last level
	path_ana->last_name[0] = 0;
	inode_no = 1;			//first time search root dir
	

	i = 0;
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
printf("xxx 111 counter=%d,\n",counter);
	        		while (counter< len  && path[counter ] != '/') counter ++;
printf("xxx 222 start_po=%d, counter=%d,\n",start_po,counter);
	        		memcpy(path1,path +start_po ,counter - start_po);
	        		path1[counter - start_po] = '\0';
			strcpy(path_ana->last_name,path1);
	       	 	counter ++;
printf("xxx 333 path1=%s\n",path1);
			if (inode_no < 1){	//upper level is not a valid dir , so the path is invalid
				path_ana->is_valid_path = 0;	
				return 0;
			}
			path_ana->parent_inode_no = inode_no ;
			inode_no = judge_if_one_dir_exist_in_a_inode( inode_no, path1);
printf("111 path1=%s inode_no=%d,path_ana->parent_inode_no=%d\n",path1, inode_no,path_ana->parent_inode_no);
	    	}
	}
	if (inode_no == 0) {	//last level does not exist
		//path_ana->parent_inode_no = inode_no ;
		path_ana->last_is = 0;			//0:does not exist,  1: file   2:dir
printf("222 inode_no=%d,path_ana->parent_inode_no=%d\n",inode_no,path_ana->parent_inode_no);
		return 1;
	}
	else if (inode_no > 0) {	//full path exists
		path_ana->last_inode_no = inode_no ;
		path_ana->last_is = 2;			//0:does not exist,  1: file   2:dir
printf("333 inode_no=%d,path_ana->parent_inode_no=%d\n",inode_no,path_ana->parent_inode_no);
		return 1;
	}
	else {	//last exists but not dir
		path_ana->last_inode_no = 0 -inode_no ;
		path_ana->last_is = 1;			//0:does not exist,  1: file   2:dir
printf("444 inode_no=%d,path_ana->parent_inode_no=%d\n",inode_no,path_ana->parent_inode_no);
		return 1;
	}
 	return 0;
}
int get_block_position_for_new_dir(int inode_no,int rec_len,int *block_po, int *record_po)
{
	int j,num,counter;
	int retcode;
	int new_block_po;
	int old_block_po;
	struct ext2_dir_entry *dir;
	//char *name;

    	struct ext2_inode *inodes =  (struct ext2_inode*)(disk + 5120);
	num = (1024<<sb->s_log_block_size)/512;
	j = inodes[inode_no].i_blocks / num -1;	//find the last block
	
	old_block_po= 16;
	//find the tail of this block
	if (j < 14){
		counter =0;
		while (counter  < 1024){
			dir = (struct ext2_dir_entry *)(disk + 1024*inodes[inode_no].i_block[j] +counter );
			old_block_po=  inodes[inode_no].i_block[j] ;
printf("0PPP inode_no=%d,blocks=%d,j=%d, old_block_po=%d, rec_len=%d\n",inode_no,inodes[inode_no].i_blocks ,j,old_block_po, dir->rec_len );
			if (counter + dir->rec_len == 1024){//last record 
				int cur_real_rec_len = sizeof(struct ext2_dir_entry ) + (dir->name_len+3)/4*4;
printf("name=%s,name_len=%d,rec_len=%d\n",dir->name,dir->name_len,dir->rec_len);
				if (cur_real_rec_len  + counter < 1024){//enough space
					dir->rec_len =cur_real_rec_len ;
printf("2PPP inode_no=%d,blocks=%d,j=%d, cur_real_rec_len=%d, counter=%d\n",inode_no,inodes[inode_no].i_blocks ,j,cur_real_rec_len, counter);
					*block_po = j ;
					//inodes[inode_no].i_blocks += 2;
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
		//retcode = get_new_block(old_block_po,&new_block_po);
		retcode = get_new_block(&new_block_po);
		if (retcode == 1){
			*block_po =j+1;
			*record_po = 0;
			inodes[inode_no].i_blocks +=2;
			inodes[inode_no].i_block[j+1]  = new_block_po;
			return inode_no;
		}
	}


	return 0;	//no available space
}






//create a new file in the inode(inode_no) , and return the new inode for the new file
//return value:if succeed ,return the new inode_no of this new file
//	0: no free block
//	-1:no free inode
int create_one_file(int inode_no, char *file_name)
{
	int i,block_po,counter;
	int retcode,new_inode_no;
	int record_po;
	struct ext2_dir_entry *dir;
	char *name;
   	struct ext2_inode *inodes =  (struct ext2_inode*)(disk + 5120);
	
	int rec_len;
	rec_len = sizeof(struct ext2_dir_entry) + (strlen(file_name) +sizeof(char*)-1)/sizeof(char*)*sizeof(char*);
	retcode = get_block_position_for_new_dir(inode_no,rec_len, &block_po,&record_po);
printf("bbb1:%d,inode_no=%d,rec_len=%d,block_po=%d,record_po=%d\n",retcode,inode_no,rec_len,block_po,record_po);
	//if (retcode != 1){//fail to get block no
	if (retcode < 1){//fail to get block no
printf("bbb1:%d\n",retcode);
		return 0;
	}
//printf("    07 free inodes: %d\n", gd->bg_free_inodes_count);
	
	int new_block_no;
	//if (retcode  != inode_no) inode_no = retcode;
	
	printf("block_no=%d,record_po=%d,inode_no=%d\n",block_po,record_po,inode_no);
	//set dir entry info
	//inodes[inode_no].i_block[block_po] = new_block_no;
	//inodes[inode_no].i_blocks= (block_po+1)*2;
printf("  1aaaaaa inode_no=%d,new_inode_no: %d,block_num=%d,block_po=%d\n", inode_no,new_inode_no,inodes[inode_no].i_blocks,block_po);
	//retcode = get_new_inode(inode_no,&new_inode_no);
	retcode = get_new_inode(&new_inode_no);
	if (retcode != 1){//no valid inode
		printf("Error , no valid inode , total=%d,free=%d\n",sb->s_blocks_count,gd->bg_free_inodes_count);
		return -1;
	}
	dir = (struct ext2_dir_entry *)(disk + 1024*inodes[inode_no].i_block[block_po] +record_po);
	name = (char *)(disk + 1024*inodes[inode_no].i_block[block_po]+record_po +sizeof(struct ext2_dir_entry ));
	memcpy(name,file_name,strlen(file_name));
	dir->inode = new_inode_no+1;     /* Inode number */
	dir->name_len = strlen(file_name);
	dir->rec_len = 1024 - record_po ;   /* Directory entry length ,last record occupy all left*/
	dir->file_type = EXT2_FT_REG_FILE ;
printf("  2aaaaaa inode_no=%d,new_inode_no: %d,dir_name=%s,record_po=%d,new rec_len=%d\n",
 inode_no,new_inode_no,file_name,record_po,dir->rec_len );


	if (record_po == 0){
	}

	//set inode info
printf("zzz 00:%d,%d\n",block_po,new_block_no);
	//retcode = get_new_block(block_po,&new_block_no);
	retcode = get_new_block(&new_block_no);
printf("zzz 01:%d,%d\n",block_po,new_block_no);
	inodes[new_inode_no].i_mode = EXT2_S_IFREG;        /* File mode */
	/* Use 0 as the user id for the assignment. */
	inodes[new_inode_no].i_uid = 0;         /* Low 16 bits of Owner Uid */
	inodes[new_inode_no].i_size = 1024;        /* Size in bytes , will be changed later*/
	/* d_time must be set when appropriate */
	time((time_t *)&inodes[new_inode_no].i_dtime);       /* Deletion Time */
	/* Use 0 as the group id for the assignment. */
	inodes[new_inode_no].i_gid = 0;         /* Low 16 bits of Group Id */
	inodes[new_inode_no].i_links_count = 0; /* Links count */
	inodes[new_inode_no].i_blocks = 2;      /* Blocks count IN DISK SECTORS*/
	/* You should set it to 0. */
	inodes[new_inode_no].osd1 = 0;          /* OS dependent 1 */
	
	inodes[new_inode_no]. i_block[0] = new_block_no;   /* Pointers to blocks */
	for (i=1;i<15;i++)
	inodes[new_inode_no]. i_block[i] = 0;   /* Pointers to blocks */
	/* You should use generation number 0 for the assignment. */
	inodes[new_inode_no]. i_generation = 0;  /* File version (for NFS) */
	/* The following fields should be 0 for the assignment.  */
	inodes[new_inode_no]. i_file_acl = 0;    /* File ACL */
	inodes[new_inode_no]. i_dir_acl = 0;     /* Directory ACL */
	inodes[new_inode_no]. i_faddr = 0;       /* Fragment address */
	inodes[new_inode_no]. extra[0] = 0;
	inodes[new_inode_no]. extra[1] = 0;
	inodes[new_inode_no]. extra[2] = 0;
printf("zzz 02:%d,%d\n",block_po,new_block_no);



	return new_inode_no;	//succeed
}
int copy_inode_data(int src_file_inode_no,int dst_file_inode_no)
{
	int i,j,sno,dno;
	int retcode;
	int new_block_no;
	char *src_addr,*dst_addr;

	sno = src_file_inode_no;
	dno = dst_file_inode_no;
printf("BBBBBBB1:sno=%d,:dno=%d,ssize=%d,dszie=%d\n",sno, dno ,inodes[sno].i_size,inodes[dno].i_size);
	inodes[dno].i_size = inodes[sno].i_size;	//file size
	inodes[dno].i_blocks = inodes[sno].i_blocks;	//f
	inodes[dno]. i_links_count = inodes[sno]. i_links_count;	
	inodes[dno]. i_links_count = inodes[sno]. i_links_count;	//file size
printf("BBBBBBB2:sno=%d,:dno=%d,ssize=%d,dszie=%d,blocks=%d\n",sno, dno ,inodes[sno].i_size,inodes[dno].i_size,inodes[dno].i_blocks);
	j = 0;
	for (i=0;i< inodes[sno].i_blocks/2;i++){
printf("BBBBBBB3:i=%d,:block=%d\n",i,inodes[dno].i_block[i]);
		if (inodes[dno].i_block[i] == 0){
printf("BBBBBBB4:i=%d,:block=%d\n",i,inodes[dno].i_block[i]);
			retcode= get_new_block(&new_block_no);
printf("BBBBBBB5:i=%d,:block=%d,new block =%d,ret=%d\n",i,inodes[dno].i_block[i],new_block_no,retcode );
			if (retcode != 1){
				return -1;
			}
			inodes[dno].i_block[i] = new_block_no;
printf("BBBBBBB6:i=%d,:block=%d\n",i,inodes[dno].i_block[i]);
		}
printf("BBBBBBB7:i=%d,:block=%d\n",i,inodes[dno].i_block[i]);
		src_addr =(char *)( disk + 1024*inodes[sno].i_block[i]);
		dst_addr =(char *)( disk + 1024*inodes[dno].i_block[i]);
		memcpy(dst_addr ,src_addr ,1024);
	}
printf("BBBBBBB3:sno=%d,:dno=%d,ssize=%d,dszie=%d\n",sno, dno ,inodes[sno].i_size,inodes[dno].i_size);
}


int cp_a_file(int src_inode_no, int dst_dir_inode_no,int dst_file_inode_no,const char *dst_file_name)
{
	int retcode;
	//int new_block_no;

	if (dst_file_inode_no <= 0){
		get_new_inode(&dst_file_inode_no);
	}
//printf("AAAAAA1:%d\n",inodes[16].i_size);
	dst_file_inode_no = create_one_file(dst_dir_inode_no, dst_file_name);
	//new_block_no = get_new_block(&new_block_no);
printf("AAAAAA2:%d,%d\n",dst_file_inode_no ,inodes[dst_file_inode_no ].i_size);
	copy_inode_data(src_inode_no, dst_file_inode_no);
printf("AAAAAA3:%d,%d\n",dst_file_inode_no ,inodes[dst_file_inode_no ].i_size);
}


int delete_a_file(int parent_inode_no,int file_inode_no, const char *file_name)
{
	int j,k,pno,fno;
	int retcode,counter;
	int new_block_no;
	char *name;

	pno = parent_inode_no;
	fno = file_inode_no;

	//free the blocks 
printf("aaaaaa1:%d\n",fno);
	if (fno <= 0 || pno <= 0)
	return -1;
	for (k=0;k< inodes[fno].i_blocks/2;k++){
		if (inodes[fno].i_block[k] != 0)
		set_block_bitmap(inodes[fno].i_block[k] ,0);
	}
//printf("aaaaaa2\n");
	//free file inode_no
	 set_inode_bitmap(file_inode_no, 0);
//printf("aaaaaa3\n");
	//delete inode info from its parent
	struct ext2_dir_entry *dir,*pre_dir;
	for (j=0;j< inodes[pno].i_blocks/2;j++){
//printf("aaaaaa4:%d\n",j);
		counter = 0;
		pre_dir = (struct ext2_dir_entry *)NULL;
		while (counter  < 1024){
//printf("aaaaaa4:%d,counter=%d\n",j,counter);
			dir = (struct ext2_dir_entry *)(disk + 1024*inodes[pno].i_block[j] +counter );
			name = (char *)(disk + 1024*inodes[pno].i_block[j]+counter +sizeof(struct ext2_dir_entry ));
			if (!strncmp(name,file_name,dir->name_len)){
				if (counter += dir->rec_len == 1024 && pre_dir !=(struct ext2_dir_entry *)NULL ){//the last dir
					pre_dir->rec_len += dir->rec_len ;
					return 1;
				}
				//free the block
				set_block_bitmap(inodes[pno].i_block[j] ,0);
				for (k = j;k< inodes[pno].i_blocks/2-1;k++)
				inodes[pno].i_block[k] = inodes[pno].i_block[k+1];
				inodes[pno].i_blocks -= 2;
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

	printf("ZZZZZZ 1:%d,%d,%d,%d,%d\n",path_ana.is_valid_path,	//0:invalid   1:valid path
		path_ana.last_is,	//0:does not exist,  1: file   2:dir
		path_ana.slash_at_end,	//0:does not have slash at end,     1:have slash at end
		path_ana.parent_inode_no,	//inode_no of 
		path_ana.last_inode_no);	//inode_no of the last level

	if (path_ana.is_valid_path != 1 || path_ana.last_is == 0){
		//src is not a valid file
		printf("%s is not a valid path, can not rm.%d,%d\n", path,path_ana.is_valid_path,path_ana.last_is);
		return -1;//ENOENT;
	}
	if (path_ana.last_is == 2 ){
		//path is not a valid file
		printf("%s is a dir, can not rm.%d,%d\n", path,path_ana.is_valid_path,path_ana.last_is);
		return  -2;//EISDIR;
	}
	printf("path parent_inode_no=%d,last_inode_no=%d,last_name=%s\n",path_ana.parent_inode_no+1,path_ana.last_inode_no+1,path_ana.last_name);
	delete_a_file(path_ana.parent_inode_no,	//inode_no of dest path last level
		path_ana.last_inode_no,		//dest file's inode_no, need create
		path_ana.last_name);		//dest file's inode_no, need create

    return 0;
}

