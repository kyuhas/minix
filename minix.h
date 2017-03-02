#ifndef MINIX_HEADER_FILE
#define MINIX_HEADER_FILE

#define BLOCK_SIZE 1024
#define INODE_SIZE 32
#define DIR_ENT_SIZE 32

struct minix_super_block {
	unsigned short s_ninodes;
	unsigned short s_nzones;
	unsigned short s_imap_blocks;
	unsigned short s_zmap_blocks;
	unsigned short s_firstdatazone;
	unsigned short s_log_zone_size;
	unsigned int s_max_size;
	unsigned short s_magic;
	unsigned short s_state;
	unsigned int s_zones;
};

struct minix_inode {
	unsigned short i_mode;
	unsigned short i_uid;
	unsigned int i_size;
	unsigned int i_time;
	unsigned char i_gid;
	unsigned char i_nlinks;
	unsigned short i_nzone[9];
};

struct minix_dir_entry {
	unsigned short inode;
	char name[0]; //this is the same as char *name
};

void readCommands(char *commands);
char **parseCommands(char *commands, int *args);
int isImageFile(char *file);
int determineCommand(int argc, char **argv, char *fileName);

#endif