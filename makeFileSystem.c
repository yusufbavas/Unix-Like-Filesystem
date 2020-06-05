#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "inode.h"

int main(int argc, char const *argv[])
{
	if(argc != 4){
		fprintf(stderr, "Usage: makeFileSystem <block_size> <number_of_i-nodes> <filesystem_name>\n");
		exit(-1);
	}

	if (atoi(argv[1]) != 1  && atoi(argv[1]) != 2 && atoi(argv[1]) != 4 && atoi(argv[1]) != 8 && atoi(argv[1]) != 16){
		fprintf(stderr,"Block size can only be 1, 2, 4, 8 or 16 KB.\n");
		exit(-2);
	}

	int block_size = atoi(argv[1]) * KB_TO_B;
	int number_of_inodes =atoi(argv[2]);

	char* buffer = (char*) malloc(block_size);
	
    memset(buffer, 0, block_size);

    FILE* file_system = fopen(argv[3], "wb");

    for (int i = 0; i < DISK_SIZE / block_size; ++i)
        fwrite(buffer, 1, block_size, file_system);

    free(buffer);

    // initialize super block
    struct super_block sb;
	sb.block_size = block_size;
	sb.number_of_inodes = number_of_inodes;
	sb.starting_adress_of_fsm_block = ceil( ((float)sizeof(sb)) / block_size);
    sb.starting_adress_of_inode_block = sb.starting_adress_of_fsm_block + ceil(((float)sizeof(int8_t) * (number_of_inodes + (DISK_SIZE/block_size))) / block_size);
	sb.starting_adress_of_data_block = sb.starting_adress_of_inode_block + ceil(((float)(sizeof(struct inode)*number_of_inodes)) / block_size);

	// store super block
    fseek(file_system, 0, SEEK_SET);
	buffer = (char*) malloc(sizeof(sb));	
    memcpy(buffer, &sb, sizeof(sb));
    fwrite(buffer, sizeof(char), sizeof(sb), file_system);
    free(buffer);

    // initizlize and store free space management
    struct free_space_management fsm;
    fsm.free_data_blocks = (int8_t*) malloc(sizeof(int8_t)*(DISK_SIZE / block_size - sb.starting_adress_of_data_block));

    fsm.free_data_blocks[0] = 0; // block using from root directory

    for (int i = 1; i < DISK_SIZE / block_size - sb.starting_adress_of_data_block; ++i)
    	fsm.free_data_blocks[i] = 1;
     
    fsm.free_inode_blocks = (int8_t*) malloc(sizeof(int8_t)*number_of_inodes);
    fsm.free_inode_blocks[0] = 0;
    for (int i = 1; i < number_of_inodes; ++i)
        fsm.free_inode_blocks[i] = 1;

    fseek(file_system, sb.starting_adress_of_fsm_block * block_size, SEEK_SET);
    fwrite(fsm.free_data_blocks, sizeof(int8_t), DISK_SIZE / block_size - sb.starting_adress_of_data_block, file_system);
    fwrite(fsm.free_inode_blocks, sizeof(int8_t), number_of_inodes, file_system);

	free(fsm.free_data_blocks);
	free(fsm.free_inode_blocks);

	// store inodes
	struct inode inodes[number_of_inodes];
	memset(inodes,0,sizeof(inodes));
	inodes[0].type = directory;
	strcpy(inodes[0].name,"/");
	inodes[0].address[0] = sb.starting_adress_of_data_block;
	time_t t = time(NULL);
  	inodes[0].mtime = *localtime(&t);
  	inodes[0].size = block_size;
  	for (int i = 1; i < number_of_inodes; ++i)
  		inodes[i].type = empty;
  	fseek(file_system, sb.starting_adress_of_inode_block * block_size, SEEK_SET);
	fwrite(inodes,sizeof(struct inode),number_of_inodes,file_system);

	// create root directory
	int directory_size = block_size / sizeof(struct directory_to_inode);
	struct directory_to_inode *root_dir = (struct directory_to_inode*) malloc(sizeof(struct directory_to_inode)*directory_size);
	memset(root_dir,0,sizeof(struct directory_to_inode)*directory_size);
	strcpy(root_dir[0].name,".");
	root_dir[0].inode_num = 0;
	strcpy(root_dir[1].name,"..");
	root_dir[1].inode_num = 0;
	for (int i = 2; i < directory_size; ++i)
		root_dir[i].inode_num = -1;
	fseek(file_system, sb.starting_adress_of_data_block * block_size, SEEK_SET);
	fwrite(root_dir,sizeof(struct directory_to_inode),directory_size,file_system);
	free(root_dir);
	

	fclose(file_system);
	return 0;
}