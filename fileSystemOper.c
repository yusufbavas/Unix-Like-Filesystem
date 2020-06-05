#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include "inode.h"

struct super_block sb;
struct free_space_management fsm;
struct inode *inodes;

int main(int argc, char const *argv[]){

	if (argc < 3){

		fprintf(stderr, "Usage: fileSystemOper <filesystem> <operation> <parameters>\n");
		exit(-1);
	}

	FILE* file_system = fopen(argv[1], "rb+");

	// read super block
	fread(&sb, sizeof(char), sizeof(sb), file_system);

	// read free space management block
    fsm.free_data_blocks = (int8_t*) malloc(sizeof(int8_t)*(DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block)); 
    fsm.free_inode_blocks = (int8_t*) malloc(sizeof(int8_t)*sb.number_of_inodes);

    fseek(file_system, sb.starting_adress_of_fsm_block * sb.block_size, SEEK_SET);
    fread(fsm.free_data_blocks, sizeof(int8_t), DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block, file_system);
    fread(fsm.free_inode_blocks, sizeof(int8_t), sb.number_of_inodes, file_system);

    // read inode block
    inodes = (struct inode*) malloc(sizeof(struct inode)*sb.number_of_inodes);
  	fseek(file_system, sb.starting_adress_of_inode_block * sb.block_size, SEEK_SET);
	fread(inodes,sizeof(struct inode),sb.number_of_inodes,file_system);

	// read root directory
	int directory_size = sb.block_size / sizeof(struct directory_to_inode);
	struct directory_to_inode dir[directory_size];
	
	fseek(file_system, sb.starting_adress_of_data_block * sb.block_size, SEEK_SET);
	fread(dir,sizeof(struct directory_to_inode),directory_size,file_system);

	char *parameter = NULL;
	if (argc > 3){
		
		parameter = (char*) malloc(strlen(argv[3]) + 1);
		strcpy(parameter,argv[3]);

		if (parameter[0] != '/'){
			fprintf(stderr, "Error: Invalid path.\n");
			exit(-1);
		}
	}

	if (strcmp(argv[2],"list") == 0){

		if(argc != 4){
			fprintf(stderr, "Usage: fileSystemOper filesystem list <path>\n");
			exit(-1);
		}
		struct inode * n = NULL;
		n = change_directory(file_system,parameter,dir,1);
		if (n != NULL){
			fprintf(stderr, "Error: %s is not a directory.\n",n->name);
			exit(-1);
		}
		print_directory(file_system,dir);

	}
	else if(strcmp(argv[2],"mkdir") == 0){
		
		if(argc != 4){
			fprintf(stderr, "Usage: fileSystemOper filesystem mkdir <path>\n");
			exit(-1);
		}
		int c = 0;
		for (int i = 0; i < strlen(parameter); ++i)
			if (parameter[i] == '/')
				c = i;
		parameter[c] = '\0';
		change_directory(file_system,parameter,dir,1);
		create_directory(file_system,&parameter[c+1],dir);
	}
	else if(strcmp(argv[2],"rmdir") == 0){

		if(argc != 4){
			fprintf(stderr, "Usage: fileSystemOper filesystem rmdir <path>\n");
			exit(-1);
		}
		struct inode * n = NULL;
		n = change_directory(file_system,parameter,dir,1);
		if (n != NULL){
			fprintf(stderr, "Error: %s is not a directory.\n",n->name);
			exit(-1);
		}
		remove_directory(file_system,dir);
		
	}
	else if(strcmp(argv[2],"dumpe2fs") == 0){

		if(argc != 3){
			fprintf(stderr, "Usage: fileSystemOper filesystem dumpe2fs\n");
			exit(-1);
		}
		dumpe2fs(file_system);
	}
	else if(strcmp(argv[2],"write") == 0){

		if(argc != 5){
			fprintf(stderr, "Usage: fileSystemOper filesystem write <path> <LinuxFile>\n");
			exit(-1);
		}
		int c = 0;
		for (int i = 0; i < strlen(parameter); ++i)
			if (parameter[i] == '/')
				c = i;
		parameter[c] = '\0';
		change_directory(file_system,parameter,dir,1);
		write_file(file_system,&parameter[c+1],dir,argv[4]);
	}
	else if(strcmp(argv[2],"read") == 0){

		if(argc != 5){
			fprintf(stderr, "Usage: fileSystemOper filesystem read <path> <LinuxFile>\n");
			exit(-1);
		}
		struct inode * n = NULL;
		n = change_directory(file_system,parameter,dir,1);
		read_file(file_system,n,argv[4]);
	}
	else if(strcmp(argv[2],"del") == 0){

		if(argc != 4){
			fprintf(stderr, "Usage: fileSystemOper filesystem del <path>\n");
			exit(-1);
		}
		int c = 0;
		for (int i = 0; i < strlen(parameter); ++i)
			if (parameter[i] == '/')
				c = i;
		parameter[c] = '\0';
		change_directory(file_system,parameter,dir,1);
		delete_file(file_system,&parameter[c+1],dir);
	}
	else if(strcmp(argv[2],"ln") == 0){

		if(argc != 5){
			fprintf(stderr, "Usage: fileSystemOper filesystem ln <source_file> <destination_file>\n");
			exit(-1);
		}
		char *parameter2 = (char*) malloc(strlen(argv[4]) +1);
		strcpy(parameter2,argv[4]);
		if (parameter2[0] != '/'){
			fprintf(stderr, "Error: Invalid path.\n");
			exit(-1);
		}
		create_link(file_system,dir,parameter,parameter2);
		free(parameter2);
	}
	else if(strcmp(argv[2],"lnsym") == 0){
		if(argc != 5){
			fprintf(stderr, "Usage: fileSystemOper filesystem lnsym <source_file> <destination_file>\n");
			exit(-1);
		}
		char *parameter2 = (char*) malloc(strlen(argv[4]) +1);
		strcpy(parameter2,argv[4]);
		if (parameter2[0] != '/'){
			fprintf(stderr, "Error: Invalid path.\n");
			exit(-1);
		}
		int c = 0;
		for (int i = 0; i < strlen(parameter2); ++i)
			if (parameter2[i] == '/')
				c = i;
		parameter2[c] = '\0';

		change_directory(file_system,parameter2,dir,1);
		create_symlink(file_system,dir,parameter,&parameter2[c+1]);
		free(parameter2);
	}
	else if(strcmp(argv[2],"fsck") == 0){
		if(argc != 3){
			fprintf(stderr, "Usage: fileSystemOper filesystem fsck\n");
			exit(-1);
		}
		
		fsck(file_system);
	}
	else {
		printf("Unkown operation\n");
	}


	if (argc > 3)
		free(parameter);

	free(fsm.free_data_blocks);
	free(fsm.free_inode_blocks);
	free(inodes);	

    fclose(file_system);
	return 0;
}
/*
	Create hard link from source file to destination file.
	Create a directory entry to destination directory with given name and source files inode number.
*/
void create_link(FILE* file_system,struct directory_to_inode* dir,char* source_path,char *dest_path){

	int directory_size = sb.block_size / sizeof(struct directory_to_inode);
	struct directory_to_inode dest_dir[directory_size];
	memcpy(dest_dir,dir, sizeof(struct directory_to_inode)*directory_size);

	int c = 0;
	for (int i = 0; i < strlen(source_path); ++i)
		if (source_path[i] == '/')
			c = i;
	source_path[c] = '\0';
	change_directory(file_system,source_path,dir,1);

	int inode_num = -1;
	for (int i = 0; i < directory_size; ++i){

		if (strcmp(dir[i].name,&source_path[c+1]) == 0){
			inode_num = dir[i].inode_num;
			break;
		}
	}
	if (inode_num == -1){
		fprintf(stderr, "Error: %s file does not exist.\n",&source_path[c+1]);
		exit(-1);
	}

	c = 0;
	for (int i = 0; i < strlen(dest_path); ++i)
		if (dest_path[i] == '/')
			c = i;
	dest_path[c] = '\0';
	change_directory(file_system,dest_path,dest_dir,1);

	// if file name is empty or it's lenght is more than maximum lenght(14) print error message and exit.
	if (strlen(&dest_path[c+1]) == 0 || strlen(&dest_path[c+1]) >= MAX_LEN){
		fprintf(stderr, "Error: Invalid file name.\n");
		exit(-1);
	}

	// update data block
	// Add links name and inode number to source directory
	for (int i = 0; i < directory_size; ++i){

		if (dest_dir[i].inode_num == -1){
			strcpy(dest_dir[i].name,&dest_path[c+1]);
	    	dest_dir[i].inode_num = inode_num;
	    	break;	
		}	
    	else if(strcmp(dest_dir[i].name,&dest_path[c+1]) == 0){
    		fprintf(stderr, "%s file already exist.\n",&dest_path[c+1]);
    		exit(-1);
    	}
    	else if (i == directory_size -1){
    		fprintf(stderr, "Error: Directory full.\n");
    		exit(-1);
    	}
	}

    fseek(file_system, inodes[dest_dir[0].inode_num].address[0] * sb.block_size, SEEK_SET);
	fwrite(dest_dir,sizeof(struct directory_to_inode),directory_size,file_system);

	// update inode block
	fsm.free_inode_blocks[inode_num]--;

	time_t t = time(NULL);
  	inodes[dest_dir[0].inode_num].mtime = *localtime(&t); // update directory time
  	fseek(file_system, sb.starting_adress_of_inode_block * sb.block_size, SEEK_SET);
	fwrite(inodes,sizeof(struct inode),sb.number_of_inodes,file_system);


	// update free space management block
	fseek(file_system, sb.starting_adress_of_fsm_block * sb.block_size, SEEK_SET);
    fwrite(fsm.free_data_blocks, sizeof(int8_t), DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block, file_system);
    fwrite(fsm.free_inode_blocks, sizeof(int8_t), sb.number_of_inodes, file_system);
}
/*
	Create symbolic link from source file to destination file.
	Symbolic link has different inode number from source file.
	It's data block keep source file path.
*/
void create_symlink(FILE* file_system,struct directory_to_inode* dir,char* path,char* name){
	
	int directory_size = sb.block_size / sizeof(struct directory_to_inode);
	int inode_num = -1;

	if (strlen(name) == 0 || strlen(name) >= MAX_LEN){
		fprintf(stderr, "Error: Invalid file name.\n");
		exit(-1);
	}
	for (int i = 0; i < directory_size; ++i){

		if (dir[i].inode_num == -1)
    		break;
    	else if(strcmp(dir[i].name,name) == 0){
    		fprintf(stderr, "%s file already exist.\n",name);
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
	
	int file_block = -1;
	
	for (int i = 0; i < DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block; ++i){
		if (fsm.free_data_blocks[i] == 1){
			file_block = i;
			fsm.free_data_blocks[i] = 0;
			break;
		}
	}
	if (file_block == -1){
		fprintf(stderr, "Error: Data blocks full.\n");
		exit(-1);
	}
	
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

	// update inodes 
	inodes[inode_num].type = symbolic_link;
	strcpy(inodes[inode_num].name,name);
	inodes[inode_num].address[0] = sb.starting_adress_of_data_block + file_block;
	
	time_t t = time(NULL);
  	inodes[inode_num].mtime = *localtime(&t);
  	inodes[dir[0].inode_num].mtime = *localtime(&t); // update directory time
  	inodes[inode_num].size = sizeof(path);
  	fseek(file_system, sb.starting_adress_of_inode_block * sb.block_size, SEEK_SET);
	fwrite(inodes,sizeof(struct inode),sb.number_of_inodes,file_system);

	// update free space management
	fseek(file_system, sb.starting_adress_of_fsm_block * sb.block_size, SEEK_SET);
    fwrite(fsm.free_data_blocks, sizeof(int8_t), DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block, file_system);
    fwrite(fsm.free_inode_blocks, sizeof(int8_t), sb.number_of_inodes, file_system);

	// store file content
	char* buffer = (char*) malloc(sb.block_size);
	memset(buffer, 0, sb.block_size);
	strcpy(buffer,path);

	fseek(file_system, (sb.starting_adress_of_data_block + file_block) * sb.block_size, SEEK_SET);
	fwrite(buffer,1,sb.block_size,file_system);

	free(buffer);
}

/*
	Simple fsck operation.
	For I-nodes:
		compare free space management information and I-nodes contents
		I-nodes in use array calculated from I-nodes block (if I-node type is empty this node is not using)
		Free I-nodes array is free space management information

	For Data blocks:
		Find used data blocks from I-nodes and compare them with free space management
		Block in use calculated from I-nodes block (Address information in using I-nodes)
		Free blocks array is free space management indormation
*/
void fsck(FILE* file_system){

	int8_t fsck_data_blocks[DISK_SIZE / sb.block_size];
	int8_t fsck_inode_blocks[sb.number_of_inodes]; 

	int size = sb.number_of_inodes;
	for (int i = 0; i < sb.number_of_inodes; ++i)
	{
		if (inodes[i].type == empty)
			fsck_inode_blocks[i] = 0;
		else{
			fsck_inode_blocks[i] = 1;
		}
	}
	int x = 20;
	for (int i = 0; size > 0; i+= x,size -=x){
		printf("%15s","I-node number: ");
		for (int j = 0; j <  (size >= x ? x : size); ++j)
			printf("%3d ",i+j);
		printf("\n%15s","I-node in use: ");
		for (int j = 0; j < (size >= x ? x : size); ++j)
			printf("%3d ",fsck_inode_blocks[i + j]);
		printf("\n%15s","Free I-node: ");
		for (int j = 0; j < (size >= x ? x : size); ++j)
			printf("%3d ",fsm.free_inode_blocks[i+j]);
		printf("\n");
		for (int i = 0; i < 15 + x*4; ++i)
			printf("-");
		printf("\n");
		
	}	

	printf("\n");
	
	for (int i = 0; i < DISK_SIZE / sb.block_size; ++i)
		if (i < sb.starting_adress_of_data_block)
			fsck_data_blocks[i] = 1;
		else
			fsck_data_blocks[i] = 0;

	int blocks[DISK_SIZE/sb.block_size];
	for(int i = 0;i<sb.number_of_inodes;++i){
		if (inodes[i].type == empty)
			continue;
		int r = blocks_for_inode(file_system,i,blocks);
		for (int j = 0; j < r; ++j)
			fsck_data_blocks[blocks[j]] = 1;
	}
	
	size = DISK_SIZE / sb.block_size;
	for (int i = 0; size > 0; i+= x,size -=x){
		printf("%15s","Block number: ");
		for (int j = 0; j <  (size >= x ? x : size); ++j)
			printf("%3d ",i+j);
		printf("\n%15s","Block in use: ");
		for (int j = 0; j < (size >= x ? x : size); ++j)
			printf("%3d ",fsck_data_blocks[i + j]);
		printf("\n%15s","Free block: ");
		for (int j = 0; j < (size >= x ? x : size); ++j)
			printf("%3d ",(i+j < sb.starting_adress_of_data_block ? 0 : fsm.free_data_blocks[i+j-sb.starting_adress_of_data_block]));
		printf("\n");
		for (int i = 0; i < 15 + x*4; ++i)
			printf("-");
		printf("\n");
	}	

	int nfree_inodes = 0;
	for (int i = 0; i < sb.number_of_inodes; ++i)
		if (fsm.free_inode_blocks[i] != 1)
			nfree_inodes++;

	int nfree_data_count = sb.starting_adress_of_data_block;
	for (int i = 0; i < DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block; ++i)
		if (fsm.free_data_blocks[i] != 1)
			nfree_data_count++;

	int f1 =0,f2=0;
	for (int i = 0; i < sb.number_of_inodes; ++i)
		if (fsm.free_inode_blocks[i] == fsck_inode_blocks[i])
			f1 = 1;
	for (int i = 0; i < DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block; ++i)
		if (fsck_data_blocks[i + sb.starting_adress_of_data_block] == fsm.free_data_blocks[i])
			f2 = 1;

	if (f1 == 0 && f2 == 0)
		printf("clean, %d/%d inodes, %d/%d blocks\n",nfree_inodes,sb.number_of_inodes,nfree_data_count,DISK_SIZE/sb.block_size);
	if(f1 == 1)
		printf("I-node block corruption!!\n");
	if (f2 == 1)
		printf("Data block corruption!!\n");
}
/*
	Find using blocks from given I-node number.
	Blocks array has block numbers
	Return total count of blocks
*/
int blocks_for_inode(FILE* file_system,int inode_num,int* blocks){
	int result_index = 0;
	int size = inodes[inode_num].size; 
	    
	int direct = 10;
	int single_indirect = (sb.block_size / sizeof(int16_t)); // max size can contain from single indirection
	int double_indirect = single_indirect * single_indirect; // max size can contain from double indirection

	if (size > (direct * sb.block_size)){

		size -= (sb.block_size * direct);
		
		for (int i = 0; i < direct; ++i)
			blocks[result_index++] = inodes[inode_num].address[i];

		if (size > (single_indirect * sb.block_size)){

			size -= (sb.block_size * single_indirect);
			
			int16_t indirect[single_indirect];
			fseek(file_system, inodes[inode_num].address[direct] * sb.block_size, SEEK_SET);
			fread(indirect,sizeof(int16_t),single_indirect,file_system);

			for (int i = 0; i < single_indirect; ++i)
				blocks[result_index++] = indirect[i];
			blocks[result_index++] = inodes[inode_num].address[direct];

			if (size > (double_indirect * sb.block_size)){

				size -= (sb.block_size * double_indirect);

				int16_t indirect2[single_indirect];
				int index = 0;
				int index2 = 0;
				
				fseek(file_system, inodes[inode_num].address[direct + 1] * sb.block_size, SEEK_SET);
				fread(indirect2,sizeof(int16_t),single_indirect,file_system);

				fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
				fread(indirect,sizeof(int16_t),single_indirect,file_system);

				blocks[result_index++] = indirect2[index2];
				index2++;

				for (int i = 0; i < double_indirect; ++i){

					blocks[result_index++] = indirect[index];
					index++;
					if (index == single_indirect){
						fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
						fread(indirect,sizeof(int16_t),single_indirect,file_system);
						blocks[result_index++] = indirect2[index2];
						index2++;
						index = 0;
					}
				}
				blocks[result_index++] = inodes[inode_num].address[direct + 1];		
				
				int16_t indirect3[single_indirect];
				index = 0;
				index2 = 0;
				int index3 = 0;

				fseek(file_system, inodes[inode_num].address[direct + 2] * sb.block_size, SEEK_SET);
				fread(indirect3,sizeof(int16_t),single_indirect,file_system);

				fseek(file_system, indirect3[index3] * sb.block_size, SEEK_SET);
				fread(indirect2,sizeof(int16_t),single_indirect,file_system);
				blocks[result_index++] = indirect3[index3];
				index3++;

				fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
				fread(indirect,sizeof(int16_t),single_indirect,file_system);
				blocks[result_index++] = indirect2[index2];
				index2++;

				for (int i = 0; i < ceil((float)size / sb.block_size); ++i){

					blocks[result_index++] = indirect[index];
					index++;

					if (index == single_indirect){
						fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
						fread(indirect,sizeof(int16_t),single_indirect,file_system);
						blocks[result_index++] = indirect2[index2];
						index2++;
						index = 0;
					}
					if (index2 == single_indirect){
						fseek(file_system, indirect3[index3] * sb.block_size, SEEK_SET);
						fread(indirect2,sizeof(int16_t),single_indirect,file_system);
						blocks[result_index++] = indirect3[index3];
						index3++;
						index2 = 0;
					}
				}
				blocks[result_index++] = inodes[inode_num].address[direct + 2];
			}
			else{

				int16_t indirect2[single_indirect];
				int index = 0;
				int index2 = 0;
				
				fseek(file_system, inodes[inode_num].address[direct + 1] * sb.block_size, SEEK_SET);
				fread(indirect2,sizeof(int16_t),single_indirect,file_system);

				fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
				fread(indirect,sizeof(int16_t),single_indirect,file_system);

				blocks[result_index++] = indirect2[index2];
				index2++;

				for (int i = 0; i < ceil((float)size / sb.block_size); ++i){

					blocks[result_index++] = indirect[index];

					index++;
					if (index == single_indirect){
						fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
						fread(indirect,sizeof(int16_t),single_indirect,file_system);
						blocks[result_index++] = indirect2[index2];
						index2++;
						index = 0;
					}
				}
				blocks[result_index++] = inodes[inode_num].address[direct + 1];
			}
		}
		else{
			int16_t indirect[single_indirect];
			fseek(file_system, inodes[inode_num].address[direct] * sb.block_size, SEEK_SET);
			fread(indirect,sizeof(int16_t),single_indirect,file_system);

			for (int i = 0; i < ceil((float)size / sb.block_size); ++i)
				blocks[result_index++] = indirect[i];

			blocks[result_index++] = inodes[inode_num].address[direct];
		}

	}
	else{
		for (int i = 0; i < ceil((float)size / sb.block_size); ++i){
			blocks[result_index++] = inodes[inode_num].address[i];
		}
	}
	return result_index;
}
/*
	Print file system informations.
	Print files and directories in file system with data blocks they use.
*/
void dumpe2fs(FILE* file_system){

	int free_data_count = 0;
	int free_inode_count = 0;
	int files = 0;
	int directories = 0;
	int sym = 0;

	for (int i = 0; i < DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block; ++i)
		if (fsm.free_data_blocks[i] == 1)
			free_data_count++;
	for (int i = 0; i < sb.number_of_inodes; ++i)
		if (fsm.free_inode_blocks[i] == 1)
			free_inode_count++;

	printf("Block Size : 			%d\n",sb.block_size);
	printf("Number of blocks : 		%d\n",DISK_SIZE / sb.block_size);
	printf("Free data blocks : 		%d\n",free_data_count);
	printf("Number of I-nodes : 		%d\n",sb.number_of_inodes);
	printf("Free I-nodes : 			%d\n",free_inode_count);

	for (int i = 0; i < sb.number_of_inodes; ++i){
		if (inodes[i].type == file)
			files++;
		else if(inodes[i].type == directory)
			directories++;
		else if(inodes[i].type == symbolic_link)
			sym++;
	}
	printf("Number of files : 		%d\n",files);
	printf("Number of directories :		%d\n",directories);
	printf("Number of symbolic links : 	%d\n",sym);

	for (int j = 0; j < 90; ++j)
			printf("-");
		printf("\n");
	int blocks[DISK_SIZE/sb.block_size];
	for(int i = 0;i<sb.number_of_inodes;++i){
		if (inodes[i].type == empty)
			continue;

		int r = blocks_for_inode(file_system,i,blocks);
		printf("I-node number: %-4d 	Name : %-15s Type : %s \n",i,inodes[i].name,
									(inodes[i].type == file ? "file" : (inodes[i].type == directory ? "directory" : "symbolic link")) );
		printf("Data Blocks : ");
		for (int j = 0; j < r; ++j)
			printf("%d ",blocks[j]);
		printf("\n");
		for (int j = 0; j < 90; ++j)
			printf("-");
		printf("\n");
	}
}