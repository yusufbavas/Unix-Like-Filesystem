#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <string.h>
#include <math.h>
//#include <sys/stat.h>
#include "inode.h"

extern struct super_block sb;
extern struct free_space_management fsm;
extern struct inode *inodes;

/*
	Change directory accourding to parameter
	If paths last element is a file, returns it's inode.
	If path is wrong print error and exit (if error is 1 otherwise return NULL)
*/
struct inode* change_directory(FILE* file_system,char* path,struct directory_to_inode* dir,int error){
	
	int directory_size = sb.block_size / sizeof(struct directory_to_inode);
	char *token;
   
	token = strtok(path, "/");
   
	while(token != NULL){

		for (int i = 0; i < directory_size; ++i){

			if (dir[i].inode_num == -1){

				if (error == 1){
					fprintf(stderr, "Error: %s directory does not exist.\n",token);
					exit(-1);
				}
				else
					return NULL;
				
			}

			if (strcmp(dir[i].name,token)==0){
				// if its a directory open that directory
				if(inodes[dir[i].inode_num].type == directory){

					fseek(file_system, inodes[dir[i].inode_num].address[0] * sb.block_size, SEEK_SET);
					fread(dir,sizeof(struct directory_to_inode),directory_size,file_system);
					break;
				}
				// if its a symbolic link find the target file
				else if (inodes[dir[i].inode_num].type == symbolic_link){

					char buffer[sb.block_size];
					fseek(file_system, inodes[dir[i].inode_num].address[0] * sb.block_size, SEEK_SET);
					fread(buffer,1,sb.block_size,file_system);

					int directory_size = sb.block_size / sizeof(struct directory_to_inode);
					
					fseek(file_system, sb.starting_adress_of_data_block * sb.block_size, SEEK_SET);
					fread(dir,sizeof(struct directory_to_inode),directory_size,file_system);

					//return change_directory(file_system,buffer,dir,error);
					struct inode* n;
					n = change_directory(file_system,buffer,dir,0);
					if (n == NULL){
						printf("Error: Broken link.\n");
						exit(-1);
					}
					return n;
				}
				else{
					token = strtok(NULL, "/");
					if (token != NULL){

						if (error == 1){
							fprintf(stderr, "Error: File does not exist - Wrong path.\n");
							exit(-1);
						}
						else
							return NULL;
						
					}
					return &inodes[dir[i].inode_num];
				}
			}
		}

		token = strtok(NULL, "/");
   }

   return NULL;
}
/*
	Print directory content (ls -l)
	Print size, modification time and name of file or directory
	if file is a symbolic link print its target (like link -> target of link)
	if entry in directory is a directory, its background will be green
	if its a normal file, its color will be green
	if its a symbolic link, its color will be blue
	if its a broke symbolic link, its color will be red
*/
void print_directory(FILE* file_system,struct directory_to_inode* dir){

	int directory_size = sb.block_size / sizeof(struct directory_to_inode);

	for (int i = 2; i < directory_size; ++i){

			if (dir[i].inode_num == -1)
				break;
			if (inodes[dir[i].inode_num].type == symbolic_link){
				char buffer[sb.block_size];
				char sympath[sb.block_size];

				fseek(file_system, inodes[dir[i].inode_num].address[0] * sb.block_size, SEEK_SET);
				fread(buffer,1,sb.block_size,file_system);
				strcpy(sympath,buffer);
				
				int directory_size = sb.block_size / sizeof(struct directory_to_inode);
				struct directory_to_inode symdir[directory_size];
				
				fseek(file_system, sb.starting_adress_of_data_block * sb.block_size, SEEK_SET);
				fread(symdir,sizeof(struct directory_to_inode),directory_size,file_system);

				printf("%7d	%d-%02d-%02d %02d:%02d:%02d %s%s -> %s"DEFAULT,inodes[dir[i].inode_num].size,
				inodes[dir[i].inode_num].mtime.tm_year + 1900, 
				inodes[dir[i].inode_num].mtime.tm_mon + 1, 
				inodes[dir[i].inode_num].mtime.tm_mday, 
				inodes[dir[i].inode_num].mtime.tm_hour, 
				inodes[dir[i].inode_num].mtime.tm_min, 
				inodes[dir[i].inode_num].mtime.tm_sec,
				(change_directory(file_system,buffer,symdir,0) == NULL ? RED : BLUE),dir[i].name,sympath);
			}
			else{
				printf("%7d	%d-%02d-%02d %02d:%02d:%02d %s%s"DEFAULT,inodes[dir[i].inode_num].size,
				inodes[dir[i].inode_num].mtime.tm_year + 1900, 
				inodes[dir[i].inode_num].mtime.tm_mon + 1, 
				inodes[dir[i].inode_num].mtime.tm_mday, 
				inodes[dir[i].inode_num].mtime.tm_hour, 
				inodes[dir[i].inode_num].mtime.tm_min, 
				inodes[dir[i].inode_num].mtime.tm_sec,
				(inodes[dir[i].inode_num].type == directory ? BACKGROUND : GREEN),dir[i].name);
			}
			printf("\n");
		}
}
/*
	Create a directory into the given directory
	update modification time of parent directory
*/
void create_directory(FILE* file_system,char* name,struct directory_to_inode* dir){

	int directory_size = sb.block_size / sizeof(struct directory_to_inode);
	int inode_num = -1;
	int data_block = -1;
	// if file name is empty or it's lenght is more than maximum lenght(14) print error message and exit.
	if (strlen(name) == 0 || strlen(name) >= MAX_LEN){
		fprintf(stderr, "Error: Invalid file name.\n");
		exit(-1);
	}

	for (int i = 0; i < directory_size; ++i){

		if (dir[i].inode_num == -1)
    		break;
    	else if(strcmp(dir[i].name,name) == 0){
    		fprintf(stderr, "%s directory already exist.\n",name);
    		exit(-1);
    	}
	}

	for (int i = 0; i < sb.number_of_inodes; ++i){
		if (fsm.free_inode_blocks[i] == 1){
			inode_num = i;
			fsm.free_inode_blocks[i] = 0;
			break;
		}
	}
	if (inode_num == -1){
		fprintf(stderr, "Error: Inode blocks full.\n");
		exit(-1);
	}

	for (int i = 0; i < DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block; ++i){
		if (fsm.free_data_blocks[i] == 1){
			data_block = i;
			fsm.free_data_blocks[i] = 0;
			break;
		}
	}
	if (inode_num == -1){
		fprintf(stderr, "Error: Data blocks full.\n");
		exit(-1);
	}

	// update inodes
	inodes[inode_num].type = directory;
	strcpy(inodes[inode_num].name,name);
	inodes[inode_num].address[0] = sb.starting_adress_of_data_block + data_block;
	time_t t = time(NULL);
	inodes[dir[0].inode_num].mtime = *localtime(&t); // update directory time
  	inodes[inode_num].mtime = *localtime(&t);
  	inodes[inode_num].size = sb.block_size;
  	fseek(file_system, sb.starting_adress_of_inode_block * sb.block_size, SEEK_SET);
	fwrite(inodes,sizeof(struct inode),sb.number_of_inodes,file_system);

	// update free space management
	fseek(file_system, sb.starting_adress_of_fsm_block * sb.block_size, SEEK_SET);
    fwrite(fsm.free_data_blocks, sizeof(int8_t), DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block, file_system);
    fwrite(fsm.free_inode_blocks, sizeof(int8_t), sb.number_of_inodes, file_system);

    // update data block
    for (int i = 0; i < directory_size; ++i){
    	if (dir[i].inode_num == -1){
    		strcpy(dir[i].name,name);
    		dir[i].inode_num = inode_num;
    		break;
    	}
    	else if (i == directory_size -1){
    		fprintf(stderr, "Error: Directory full.\n");
    		exit(-1);
    	}
    }
    fseek(file_system, inodes[dir[0].inode_num].address[0] * sb.block_size, SEEK_SET);
	fwrite(dir,sizeof(struct directory_to_inode),directory_size,file_system);

	struct directory_to_inode *new_dir = (struct directory_to_inode*) malloc(sizeof(struct directory_to_inode)*directory_size);
	memset(new_dir,0,sizeof(struct directory_to_inode)*directory_size);
	strcpy(new_dir[0].name,".");
	new_dir[0].inode_num = inode_num;
	strcpy(new_dir[1].name,"..");
	new_dir[1].inode_num = dir[0].inode_num;
	for (int i = 2; i < directory_size; ++i)
		new_dir[i].inode_num = -1;
	
	fseek(file_system, (sb.starting_adress_of_data_block + data_block) * sb.block_size, SEEK_SET);
	fwrite(new_dir,sizeof(struct directory_to_inode),directory_size,file_system);
	free(new_dir);
}
/*
	remove directory in given directory
	If directory is not empty print error message
	update parent directory modification time
*/
void remove_directory(FILE* file_system,struct directory_to_inode* dir){
	
	int directory_size = sb.block_size / sizeof(struct directory_to_inode);
	struct directory_to_inode *parent_dir = (struct directory_to_inode*) malloc(sizeof(struct directory_to_inode)*directory_size);
	int deleted_index = -1;
	char *name = inodes[dir[0].inode_num].name;
	// Root directory can not be deleted
	if (dir[0].inode_num == 0)
	{
		fprintf(stderr, "Error: Access denied.\n");
		exit(-1);
	}

	if (dir[2].inode_num != -1)
	{
		fprintf(stderr, "Error: %s directory is not empty.\n",name);
		exit(-1);
	}

	fseek(file_system, inodes[dir[1].inode_num].address[0] * sb.block_size, SEEK_SET);
	fread(parent_dir,sizeof(struct directory_to_inode),directory_size,file_system);
	
	for (int i = 0; i < directory_size; ++i){

		if (parent_dir[i].inode_num == -1){

			parent_dir[deleted_index].inode_num = parent_dir[i-1].inode_num;
			parent_dir[i-1].inode_num = -1;
			strcpy(parent_dir[deleted_index].name,parent_dir[i-1].name);
			break;
			
		}
		if (strcmp(parent_dir[i].name,name)==0)
			deleted_index = i;	
	}

	fsm.free_data_blocks[inodes[dir[0].inode_num].address[0] - sb.starting_adress_of_data_block] = 1;
	fsm.free_inode_blocks[dir[0].inode_num] = 1;

	// update inodes
	inodes[dir[0].inode_num].type = empty;
	time_t t = time(NULL);
	inodes[parent_dir[0].inode_num].mtime = *localtime(&t); // update directory time
	fseek(file_system, sb.starting_adress_of_inode_block * sb.block_size, SEEK_SET);
	fwrite(inodes,sizeof(struct inode),sb.number_of_inodes,file_system);

	// update data block
	fseek(file_system, inodes[parent_dir[0].inode_num].address[0] * sb.block_size, SEEK_SET);
	fwrite(parent_dir,sizeof(struct directory_to_inode),directory_size,file_system);

	// update free space management block
	fseek(file_system, sb.starting_adress_of_fsm_block * sb.block_size, SEEK_SET);
    fwrite(fsm.free_data_blocks, sizeof(int8_t), DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block, file_system);
    fwrite(fsm.free_inode_blocks, sizeof(int8_t), sb.number_of_inodes, file_system);

	free(parent_dir);
}