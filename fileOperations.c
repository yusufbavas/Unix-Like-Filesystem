#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include "inode.h"

extern struct super_block sb;
extern struct free_space_management fsm;
extern struct inode *inodes;
/* 
	Find free data blocks according to given index
	Helper function for write_file function
*/
int next_free_data(int block_index){
	
	while(fsm.free_data_blocks[block_index] == 0){
		block_index++;

		if (block_index ==  DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block){
			fprintf(stderr, "Error: Data blocks full.\n");
			exit(-1);
		}
	}
	return block_index;
}
/*
	Create a file under the given directory and fill it with given file content
	If file is a symbolic link, find target file and write into it.
	Fill file using single, double, triple indirections if necessary
*/
void write_file(FILE* file_system,char* name,struct directory_to_inode* dir,const char* o_file){

	int directory_size = sb.block_size / sizeof(struct directory_to_inode);
	int inode_num = -1;
	struct inode* n = NULL;

	if (strlen(name) == 0 || strlen(name) >= MAX_LEN){
		fprintf(stderr, "Error: Invalid file name.\n");
		exit(-1);
	}
	for (int i = 0; i < directory_size; ++i){

		if (dir[i].inode_num == -1)
    		break;
    	else if(strcmp(dir[i].name,name) == 0){ // if file exist delete it
    		if (inodes[dir[i].inode_num].type == symbolic_link){
    			
    			n = change_directory(file_system,name,dir,0);
    			
	    		for (int j = 0; j < directory_size; ++j){
	    			
	    			if(strcmp(dir[j].name,n->name) == 0){

	    				inode_num = dir[j].inode_num;
			    		int temp = fsm.free_inode_blocks[inode_num];
			    		fsm.free_inode_blocks[inode_num] = 0;
			    		delete_file(file_system,n->name,dir);
			    		fsm.free_inode_blocks[inode_num] = temp;
			    		name = n->name;
			    		break;
	    			}
	    		}
    		}
    		else{
    			inode_num = dir[i].inode_num;
	    		int temp = fsm.free_inode_blocks[inode_num];
	    		fsm.free_inode_blocks[inode_num] = 0;
	    		delete_file(file_system,name,dir);
	    		fsm.free_inode_blocks[inode_num] = temp;
	    		break;	
    		}
    		
    	}
	}
	if (inode_num == -1){
		
		for (int i = 0; i < sb.number_of_inodes; ++i){
			if (fsm.free_inode_blocks[i] == 1){
				inode_num = i;
				fsm.free_inode_blocks[i] = 0;
				break;
			}
		}
	}
	
	if (inode_num == -1){
		fprintf(stderr, "Error: Inode blocks full.\n");
		exit(-1);
	}
	
	FILE* f = fopen(o_file, "rb");
	if (f == NULL){
		fprintf(stderr, "Error: %s file does not exist\n",o_file);
		exit(-1);
	}

	int fd = fileno(f);
	struct stat buf;
	fstat(fd, &buf);
	int size = buf.st_size;
	
	int direct = 10;
	int single_indirect = (sb.block_size / sizeof(int16_t)); // max size can contain from single indirection
	int double_indirect = single_indirect * single_indirect; // max size can contain from double indirection
	int block_index = 0;

	char* buffer = (char*) malloc(sb.block_size);
	memset(buffer, 0, sb.block_size);

	block_index = next_free_data(block_index);
	
	if (size > (direct * sb.block_size)){

		size -= (sb.block_size * direct);
		
		for (int i = 0; i < direct; ++i){

			fread(buffer,1,sb.block_size,f);
			fseek(file_system, (sb.starting_adress_of_data_block + block_index) * sb.block_size, SEEK_SET);
			fwrite(buffer,1,sb.block_size,file_system);
			inodes[inode_num].address[i] = sb.starting_adress_of_data_block + block_index;
			fsm.free_data_blocks[block_index] = 0;
			block_index = next_free_data(block_index);
		}

		if (size > (single_indirect * sb.block_size)){

			size -= (sb.block_size * single_indirect);
			int16_t indirect[single_indirect];

			for (int i = 0; i < single_indirect; ++i){

				fread(buffer,1,sb.block_size,f);
				fseek(file_system, (sb.starting_adress_of_data_block + block_index) * sb.block_size, SEEK_SET);
				fwrite(buffer,1,sb.block_size,file_system);
				indirect[i] = sb.starting_adress_of_data_block +  block_index;
				fsm.free_data_blocks[block_index] = 0;
				block_index = next_free_data(block_index);
			}
			fseek(file_system, (sb.starting_adress_of_data_block +block_index) * sb.block_size, SEEK_SET);
			fwrite(indirect,sizeof(int16_t),single_indirect,file_system);
			inodes[inode_num].address[direct] = sb.starting_adress_of_data_block + block_index;
			fsm.free_data_blocks[block_index] = 0;
			block_index = next_free_data(block_index);

			if (size > (double_indirect * sb.block_size)){

				size -= (sb.block_size * double_indirect);

				int16_t indirect2[single_indirect];
				int index = 0;
				int index2 = 0;
				for (int i = 0; i < double_indirect; ++i){

					fread(buffer,1,sb.block_size,f);
					fseek(file_system, (sb.starting_adress_of_data_block + block_index) * sb.block_size, SEEK_SET);
					fwrite(buffer,1,sb.block_size,file_system);
					indirect[index] = sb.starting_adress_of_data_block +  block_index;
					fsm.free_data_blocks[block_index] = 0;
					block_index = next_free_data(block_index);
					index++;
					if (index == single_indirect){
						fseek(file_system, (sb.starting_adress_of_data_block +block_index) * sb.block_size, SEEK_SET);
						fwrite(indirect,sizeof(int16_t),single_indirect,file_system);
						indirect2[index2] = sb.starting_adress_of_data_block +  block_index;
						fsm.free_data_blocks[block_index] = 0;
						block_index = next_free_data(block_index);
						index = 0;
						index2++;
					}
				}
				fseek(file_system, (sb.starting_adress_of_data_block + block_index) * sb.block_size, SEEK_SET);
				fwrite(indirect2,sizeof(int16_t),single_indirect,file_system);
				inodes[inode_num].address[direct + 1] = sb.starting_adress_of_data_block + block_index;
				fsm.free_data_blocks[block_index] = 0;
				block_index = next_free_data(block_index);

				// triple indirection
				int16_t indirect3[single_indirect];
				index = 0;
				index2 = 0;
				int index3 = 0;

				for (int i = 0; i < ceil((float)size / sb.block_size); ++i){

					fread(buffer,1,sb.block_size,f);
					fseek(file_system, (sb.starting_adress_of_data_block + block_index) * sb.block_size, SEEK_SET);
					fwrite(buffer,1,sb.block_size,file_system);
					indirect[index] = sb.starting_adress_of_data_block +  block_index;
					fsm.free_data_blocks[block_index] = 0;
					block_index = next_free_data(block_index);
					index++;
					if (index == single_indirect || i == ceil((float)size / sb.block_size) -1){
						fseek(file_system, (sb.starting_adress_of_data_block +block_index) * sb.block_size, SEEK_SET);
						fwrite(indirect,sizeof(int16_t),single_indirect,file_system);
						indirect2[index2] = sb.starting_adress_of_data_block +  block_index;
						fsm.free_data_blocks[block_index] = 0;
						block_index = next_free_data(block_index);
						index = 0;
						index2++;
					}
					if(index2 == single_indirect || i == ceil((float)size / sb.block_size) -1){
						fseek(file_system, (sb.starting_adress_of_data_block +block_index) * sb.block_size, SEEK_SET);
						fwrite(indirect2,sizeof(int16_t),single_indirect,file_system);
						indirect3[index3] = sb.starting_adress_of_data_block +  block_index;
						fsm.free_data_blocks[block_index] = 0;
						block_index = next_free_data(block_index);
						index2 = 0;
						index3++;
					}
				}
				fseek(file_system, (sb.starting_adress_of_data_block + block_index) * sb.block_size, SEEK_SET);
				fwrite(indirect3,sizeof(int16_t),single_indirect,file_system);
				inodes[inode_num].address[direct + 2] = sb.starting_adress_of_data_block + block_index;
				fsm.free_data_blocks[block_index] = 0;
				block_index = next_free_data(block_index);				
			}
			else{
				// double indirection
				int16_t indirect2[single_indirect];
				int index = 0;
				int index2 = 0;
				for (int i = 0; i < ceil((float)size / sb.block_size); ++i){

					fread(buffer,1,sb.block_size,f);
					fseek(file_system, (sb.starting_adress_of_data_block + block_index) * sb.block_size, SEEK_SET);
					fwrite(buffer,1,sb.block_size,file_system);
					indirect[index] = sb.starting_adress_of_data_block +  block_index;
					fsm.free_data_blocks[block_index] = 0;
					block_index = next_free_data(block_index);
					index++;
					if (index == single_indirect || i == ceil((float)size / sb.block_size) -1){
						fseek(file_system, (sb.starting_adress_of_data_block +block_index) * sb.block_size, SEEK_SET);
						fwrite(indirect,sizeof(int16_t),single_indirect,file_system);
						indirect2[index2] = sb.starting_adress_of_data_block +  block_index;
						fsm.free_data_blocks[block_index] = 0;
						block_index = next_free_data(block_index);
						index = 0;
						index2++;
					}
				}
				fseek(file_system, (sb.starting_adress_of_data_block + block_index) * sb.block_size, SEEK_SET);
				fwrite(indirect2,sizeof(int16_t),single_indirect,file_system);
				inodes[inode_num].address[direct + 1] = sb.starting_adress_of_data_block + block_index;
				fsm.free_data_blocks[block_index] = 0;
				block_index = next_free_data(block_index);
			}
		}
		else{
			// single indirection
			int16_t indirect[single_indirect];

			for (int i = 0; i < ceil((float)size / sb.block_size); ++i){

				fread(buffer,1,sb.block_size,f);
				fseek(file_system, (sb.starting_adress_of_data_block + block_index) * sb.block_size, SEEK_SET);
				fwrite(buffer,1,sb.block_size,file_system);
				indirect[i] = sb.starting_adress_of_data_block +  block_index;
				fsm.free_data_blocks[block_index] = 0;
				block_index = next_free_data(block_index);
			}
			fseek(file_system, (sb.starting_adress_of_data_block +block_index) * sb.block_size, SEEK_SET);
			fwrite(indirect,sizeof(int16_t),single_indirect,file_system);
			inodes[inode_num].address[direct] = sb.starting_adress_of_data_block + block_index;
			fsm.free_data_blocks[block_index] = 0;
			block_index = next_free_data(block_index);
		}

	}
	else{
		// direct addressing
		for (int i = 0; i < ceil((float)size / sb.block_size); ++i){

			fread(buffer,1,sb.block_size,f);
			fseek(file_system, (sb.starting_adress_of_data_block + block_index) * sb.block_size, SEEK_SET);
			fwrite(buffer,1,sb.block_size,file_system);
			inodes[inode_num].address[i] = sb.starting_adress_of_data_block + block_index;
			fsm.free_data_blocks[block_index] = 0;
			block_index = next_free_data(block_index);
		}
		
	}

	inodes[inode_num].type = file;
	strcpy(inodes[inode_num].name,name);
	time_t t = time(NULL);
  	inodes[inode_num].mtime = *localtime(&t);
  	inodes[dir[0].inode_num].mtime = inodes[inode_num].mtime = *localtime(&t);
  	inodes[inode_num].size = buf.st_size;
  	fseek(file_system, sb.starting_adress_of_inode_block * sb.block_size, SEEK_SET);
	fwrite(inodes,sizeof(struct inode),sb.number_of_inodes,file_system);

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

	// update free space management
	fseek(file_system, sb.starting_adress_of_fsm_block * sb.block_size, SEEK_SET);
    fwrite(fsm.free_data_blocks, sizeof(int8_t), DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block, file_system);
    fwrite(fsm.free_inode_blocks, sizeof(int8_t), sb.number_of_inodes, file_system);


	fclose(f);
	free(buffer);
}
/*
	Read file in file system and write it content to the given file
	Read file using single, double, triple indirections if necessary
*/
void read_file(FILE* file_system,struct inode* n,const char* name){

    FILE* f = fopen(name, "wb");

    int size = n->size; 
    int s = n->size % sb.block_size;

	int direct = 10;
	int single_indirect = (sb.block_size / sizeof(int16_t)); // max size can contain from single indirection
	int double_indirect = single_indirect * single_indirect; // max size can contain from double indirection

	char* buffer = (char*) malloc(sb.block_size);
	memset(buffer, 0, sb.block_size);

	if (size > (direct * sb.block_size)){

		size -= (sb.block_size * direct);
		
		for (int i = 0; i < direct; ++i){

			fseek(file_system, n->address[i] * sb.block_size, SEEK_SET);
			fread(buffer,1,sb.block_size,file_system);
			
			fwrite(buffer,1,sb.block_size,f);
		}

		if (size > (single_indirect * sb.block_size)){

			size -= (sb.block_size * single_indirect);
			
			int16_t indirect[single_indirect];
			fseek(file_system, n->address[direct] * sb.block_size, SEEK_SET);
			fread(indirect,sizeof(int16_t),single_indirect,file_system);

			for (int i = 0; i < single_indirect; ++i){

				fseek(file_system, indirect[i] * sb.block_size, SEEK_SET);
				fread(buffer,1,sb.block_size,file_system);
	
				fwrite(buffer,1,sb.block_size,f);
			}

			if (size > (double_indirect * sb.block_size)){
				// triple indirection
				size -= (sb.block_size * double_indirect);

				int16_t indirect2[single_indirect];
				int index = 0;
				int index2 = 0;
				
				fseek(file_system, n->address[direct + 1] * sb.block_size, SEEK_SET);
				fread(indirect2,sizeof(int16_t),single_indirect,file_system);

				fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
				fread(indirect,sizeof(int16_t),single_indirect,file_system);
				index2++;

				for (int i = 0; i < double_indirect; ++i){

					fseek(file_system, indirect[index] * sb.block_size, SEEK_SET);
					fread(buffer,1,sb.block_size,file_system);

					fwrite(buffer,1,sb.block_size,f);
					
					if (index == single_indirect){
						fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
						fread(indirect,sizeof(int16_t),single_indirect,file_system);
						index2++;
						index = 0;
					}
				}				
				
				int16_t indirect3[single_indirect];
				index = 0;
				index2 = 0;
				int index3 = 0;

				fseek(file_system, n->address[direct + 2] * sb.block_size, SEEK_SET);
				fread(indirect3,sizeof(int16_t),single_indirect,file_system);

				fseek(file_system, indirect3[index3] * sb.block_size, SEEK_SET);
				fread(indirect2,sizeof(int16_t),single_indirect,file_system);
				index3++;

				fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
				fread(indirect,sizeof(int16_t),single_indirect,file_system);
				index2++;

				for (int i = 0; i < ceil((float)size / sb.block_size); ++i){

					fseek(file_system, indirect[index] * sb.block_size, SEEK_SET);
					fread(buffer,1,sb.block_size,file_system);

					if (i == ceil((float)size / sb.block_size)-1)
						fwrite(buffer,1,s,f);
					else
						fwrite(buffer,1,sb.block_size,f);
					index++;
					if (index == single_indirect){
						fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
						fread(indirect,sizeof(int16_t),single_indirect,file_system);
						index2++;
						index = 0;
					}
					if (index2 == single_indirect){
						fseek(file_system, indirect3[index3] * sb.block_size, SEEK_SET);
						fread(indirect2,sizeof(int16_t),single_indirect,file_system);
						index3++;
						index2 = 0;
					}
				}				
			}
			else{
				// double indirection
				int16_t indirect2[single_indirect];
				int index = 0;
				int index2 = 0;
				
				fseek(file_system, n->address[direct + 1] * sb.block_size, SEEK_SET);
				fread(indirect2,sizeof(int16_t),single_indirect,file_system);

				fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
				fread(indirect,sizeof(int16_t),single_indirect,file_system);
				index2++;

				for (int i = 0; i < ceil((float)size / sb.block_size); ++i){

					fseek(file_system, indirect[index] * sb.block_size, SEEK_SET);
					fread(buffer,1,sb.block_size,file_system);

					if (i == ceil((float)size / sb.block_size)-1)
						fwrite(buffer,1,s,f);
					else
						fwrite(buffer,1,sb.block_size,f);
					index++;
					if (index == single_indirect){
						fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
						fread(indirect,sizeof(int16_t),single_indirect,file_system);
						index2++;
						index = 0;
					}
				}
			}
		}
		else{
			// single indirection
			int16_t indirect[single_indirect];
			fseek(file_system, n->address[direct] * sb.block_size, SEEK_SET);
			fread(indirect,sizeof(int16_t),single_indirect,file_system);

			for (int i = 0; i < ceil((float)size / sb.block_size); ++i){

				fseek(file_system, indirect[i] * sb.block_size, SEEK_SET);
				fread(buffer,1,sb.block_size,file_system);
				
				if (i == ceil((float)size / sb.block_size)-1)
					fwrite(buffer,1,s,f);
				else
					fwrite(buffer,1,sb.block_size,f);
			}
		}

	}
	else{
		// direct addressing
		for (int i = 0; i < ceil((float)size / sb.block_size); ++i){
			
			fseek(file_system, n->address[i] * sb.block_size, SEEK_SET);
			fread(buffer,1,sb.block_size,file_system);
			
			if (i == ceil((float)size / sb.block_size)-1)
				fwrite(buffer,1,s,f);
			else
				fwrite(buffer,1,sb.block_size,f);
		}		
	}

    free(buffer);
    fclose(f);
}
/*
	Delete file in the file system
	If that file has a hard link, don't delete its data blocks and I-node
*/
void delete_file(FILE* file_system,char* name,struct directory_to_inode* dir){

	int directory_size = sb.block_size / sizeof(struct directory_to_inode);

	int inode_num = -1;
	int deleted_index = -1;

	if (strlen(name) == 0){
		fprintf(stderr, "Error: Invalid file name.\n");
		exit(-1);
	}

	for (int i = 0; i < directory_size; ++i){

		if (dir[i].inode_num == -1){

			if (deleted_index == -1){
				fprintf(stderr, "Error: %s file does not exist.\n",name);
				exit(-1);
			}
			else{
				dir[deleted_index].inode_num = dir[i-1].inode_num;
				dir[i-1].inode_num = -1;
				strcpy(dir[deleted_index].name,dir[i-1].name);
				break;
			}
		}
		if (strcmp(dir[i].name,name) == 0){
			if (inodes[dir[i].inode_num].type == directory){

				fprintf(stderr, "Error: %s is not a file.\n",name);
				exit(-1);
			}
			inode_num = dir[i].inode_num;
			deleted_index = i;
		}
	}
	if (fsm.free_inode_blocks[inode_num] == 0){

		int size = inodes[inode_num].size; 
	    
		int direct = 10;
		int single_indirect = (sb.block_size / sizeof(int16_t)); // max size can contain from single indirection
		int double_indirect = single_indirect * single_indirect; // max size can contain from double indirection

		if (size > (direct * sb.block_size)){

			size -= (sb.block_size * direct);
			
			for (int i = 0; i < direct; ++i)
				fsm.free_data_blocks[inodes[inode_num].address[i] - sb.starting_adress_of_data_block] = 1;

			if (size > (single_indirect * sb.block_size)){

				size -= (sb.block_size * single_indirect);
				
				int16_t indirect[single_indirect];
				fseek(file_system, inodes[inode_num].address[direct] * sb.block_size, SEEK_SET);
				fread(indirect,sizeof(int16_t),single_indirect,file_system);

				for (int i = 0; i < single_indirect; ++i)
					fsm.free_data_blocks[indirect[i] - sb.starting_adress_of_data_block] = 1;

				fsm.free_data_blocks[inodes[inode_num].address[direct] - sb.starting_adress_of_data_block] = 1;

				if (size > (double_indirect * sb.block_size)){
					// triple indirection
					size -= (sb.block_size * double_indirect);

					int16_t indirect2[single_indirect];
					int index = 0;
					int index2 = 0;
					
					fseek(file_system, inodes[inode_num].address[direct + 1] * sb.block_size, SEEK_SET);
					fread(indirect2,sizeof(int16_t),single_indirect,file_system);

					fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
					fread(indirect,sizeof(int16_t),single_indirect,file_system);

					fsm.free_data_blocks[indirect2[index2] - sb.starting_adress_of_data_block] = 1;
					index2++;

					for (int i = 0; i < double_indirect; ++i){

						fsm.free_data_blocks[indirect[index] - sb.starting_adress_of_data_block] = 1;
	
						index++;
						if (index == single_indirect){
							fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
							fread(indirect,sizeof(int16_t),single_indirect,file_system);
							fsm.free_data_blocks[indirect2[index2] - sb.starting_adress_of_data_block] = 1;
							index2++;
							index = 0;
						}
					}
					fsm.free_data_blocks[inodes[inode_num].address[direct + 1] - sb.starting_adress_of_data_block] = 1;				
					
					int16_t indirect3[single_indirect];
					index = 0;
					index2 = 0;
					int index3 = 0;

					fseek(file_system, inodes[inode_num].address[direct + 2] * sb.block_size, SEEK_SET);
					fread(indirect3,sizeof(int16_t),single_indirect,file_system);

					fseek(file_system, indirect3[index3] * sb.block_size, SEEK_SET);
					fread(indirect2,sizeof(int16_t),single_indirect,file_system);
					fsm.free_data_blocks[indirect3[index3] - sb.starting_adress_of_data_block] = 1;
					index3++;

					fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
					fread(indirect,sizeof(int16_t),single_indirect,file_system);
					fsm.free_data_blocks[indirect2[index2] - sb.starting_adress_of_data_block] = 1;
					index2++;

					for (int i = 0; i < ceil((float)size / sb.block_size); ++i){

						fsm.free_data_blocks[indirect[index] - sb.starting_adress_of_data_block] = 1;
						index++;

						if (index == single_indirect){
							fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
							fread(indirect,sizeof(int16_t),single_indirect,file_system);
							fsm.free_data_blocks[indirect2[index2] - sb.starting_adress_of_data_block] = 1;
							index2++;
							index = 0;
						}
						if (index2 == single_indirect){
							fseek(file_system, indirect3[index3] * sb.block_size, SEEK_SET);
							fread(indirect2,sizeof(int16_t),single_indirect,file_system);
							fsm.free_data_blocks[indirect3[index3] - sb.starting_adress_of_data_block] = 1;
							index3++;
							index2 = 0;
						}
					}
					fsm.free_data_blocks[inodes[inode_num].address[direct + 2] - sb.starting_adress_of_data_block] = 1;				
				}
				else{
					// double indirection
					int16_t indirect2[single_indirect];
					int index = 0;
					int index2 = 0;
					
					fseek(file_system, inodes[inode_num].address[direct + 1] * sb.block_size, SEEK_SET);
					fread(indirect2,sizeof(int16_t),single_indirect,file_system);

					fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
					fread(indirect,sizeof(int16_t),single_indirect,file_system);

					fsm.free_data_blocks[indirect2[index2] - sb.starting_adress_of_data_block] = 1;
					index2++;

					for (int i = 0; i < ceil((float)size / sb.block_size); ++i){

						fsm.free_data_blocks[indirect[index] - sb.starting_adress_of_data_block] = 1;
	
						index++;
						if (index == single_indirect){
							fseek(file_system, indirect2[index2] * sb.block_size, SEEK_SET);
							fread(indirect,sizeof(int16_t),single_indirect,file_system);
							fsm.free_data_blocks[indirect2[index2] - sb.starting_adress_of_data_block] = 1;
							index2++;
							index = 0;
						}
					}
					fsm.free_data_blocks[inodes[inode_num].address[direct + 1] - sb.starting_adress_of_data_block] = 1;
				}
			}
			else{
				// single indirection
				int16_t indirect[single_indirect];
				fseek(file_system, inodes[inode_num].address[direct] * sb.block_size, SEEK_SET);
				fread(indirect,sizeof(int16_t),single_indirect,file_system);

				for (int i = 0; i < ceil((float)size / sb.block_size); ++i)
					fsm.free_data_blocks[indirect[i] - sb.starting_adress_of_data_block] = 1;	

				fsm.free_data_blocks[inodes[inode_num].address[direct] - sb.starting_adress_of_data_block] = 1;
			}

		}
		else{
			// direct addressing
			for (int i = 0; i < ceil((float)size / sb.block_size); ++i)
				fsm.free_data_blocks[inodes[inode_num].address[i] - sb.starting_adress_of_data_block] = 1;	
		}

	    inodes[inode_num].type = empty;
	    time_t t = time(NULL);
	    inodes[dir[0].inode_num].mtime = *localtime(&t); // update directory time
		fseek(file_system, sb.starting_adress_of_inode_block * sb.block_size, SEEK_SET);
		fwrite(inodes,sizeof(struct inode),sb.number_of_inodes,file_system);
	}
	
	fsm.free_inode_blocks[inode_num]++;

	// update free space management block
	fseek(file_system, sb.starting_adress_of_fsm_block * sb.block_size, SEEK_SET);
    fwrite(fsm.free_data_blocks, sizeof(int8_t), DISK_SIZE / sb.block_size - sb.starting_adress_of_data_block, file_system);
    fwrite(fsm.free_inode_blocks, sizeof(int8_t), sb.number_of_inodes, file_system);

	// update data block
	fseek(file_system, inodes[dir[0].inode_num].address[0] * sb.block_size, SEEK_SET);
	fwrite(dir,sizeof(struct directory_to_inode),directory_size,file_system);
}