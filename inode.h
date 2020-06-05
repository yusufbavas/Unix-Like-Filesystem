#include <stdint.h>
#include <time.h>

#define DISK_SIZE 1048576 // 1MB
#define KB_TO_B 1024
#define MAX_LEN 14

#define RED "\033[1;31m"
#define BLUE "\033[1;34m"
#define GREEN "\033[1;32m"
#define BACKGROUND "\033[42m" 
#define DEFAULT "\033[0m" 

enum type{file,directory,symbolic_link,empty}; 

struct inode // 104 byte
{
    enum type type;
    int size;
    struct tm mtime;
    char name[MAX_LEN];
    int16_t address[13]; // 10 direct - 1 single indirect - 1 double indirect - 1 triple indirect
};                             

struct directory_to_inode // 16 byte directory entry
{
    int16_t inode_num;
    char name[MAX_LEN];
};

struct super_block // 20 byte
{
    int block_size;
    int number_of_inodes;
    int starting_adress_of_fsm_block;
    int starting_adress_of_data_block; // and address of root directory
    int starting_adress_of_inode_block;
    
};

struct free_space_management
{
   int8_t *free_data_blocks;
   int8_t *free_inode_blocks;  
};

struct inode* change_directory(FILE* file_system,char* path,struct directory_to_inode* dir,int error);

void print_directory(FILE* file_system,struct directory_to_inode* dir);

void create_directory(FILE* file_system,char* name,struct directory_to_inode* dir);

void remove_directory(FILE* file_system,struct directory_to_inode* dir);

int next_free_data(int block_index);

void write_file(FILE* file_system,char* name,struct directory_to_inode* dir,const char* o_file);

void read_file(FILE* file_system,struct inode* n,const char* name);

void delete_file(FILE* file_system,char* name,struct directory_to_inode* dir);

void create_link(FILE* file_system,struct directory_to_inode* dir,char* source_path,char *dest_path);

void create_symlink(FILE* file_system,struct directory_to_inode* dir,char* path,char* name);

void fsck(FILE* file_system);

int blocks_for_inode(FILE* file_system,int inode_num,int* blocks);

void dumpe2fs(FILE* file_system);