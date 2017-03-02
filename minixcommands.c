#include <sys/time.h>
#include "minix.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "minixcommands.h"
#include <ctype.h>
#include <time.h>

int copyToMemory(char *fileName) {
	char buf[BLOCK_SIZE];
	int fd = open(fileName, O_RDONLY); //open the image file
	if(fd == -1) {
		printf("File error: %s -- could not be opened", fileName);
		return 0; 
	}
	else {
		lseek(fd, BLOCK_SIZE, SEEK_SET); //pointer now points to super block
		read(fd, buf, BLOCK_SIZE);
		struct minix_super_block *p = (struct minix_super_block *)buf; //now a struct pointer
		
		superBlock.s_ninodes = p->s_ninodes;
		superBlock.s_nzones = p->s_nzones;
		superBlock.s_imap_blocks = p->s_imap_blocks;
		superBlock.s_zmap_blocks = p->s_zmap_blocks;
		superBlock.s_firstdatazone = p->s_firstdatazone;
		superBlock.s_log_zone_size = p->s_log_zone_size;
		superBlock.s_max_size = p->s_max_size;
		superBlock.s_magic = p->s_magic;
		superBlock.s_state = p->s_state;
		superBlock.s_zones = p->s_zones;
		
		lastdatazone = p->s_firstdatazone + p->s_nzones;
		firstImap = 2;
		firstZmap = 2 + p->s_imap_blocks;
		firstInode = 2 + p->s_imap_blocks + p->s_zmap_blocks;
	}
	close(fd);
	return 1;
}


//this method prints out all of the available commands for the minix system
void help() {
	printf("\n");
	printf("AVAILABLE COMMANDS:\n");
	printf("help\n");
	printf("minimount argument\n");
	printf("miniumount\n");
	printf("showsuper\n");
	printf("traverse [-l]\n");
	printf("showzone [zone number]\n"); 
	printf("showfile [file name]\n"); 
	printf("quit\n");
	printf("\n");
}

//this method returns a boolean that stops the infinite loop in the main method
int quit() {
	printf("Now leaving minix shell...\n");
	int boolean = 0;
	return boolean;
}


//this method goes to the super block and prints out its values
void showsuper(char *fileName) {
	printf("\n");
	printf("# inodes:        %d\n", superBlock.s_ninodes);
	printf("# zones:         %d\n", superBlock.s_nzones);
	printf("# imap blocks:   %d\n", superBlock.s_imap_blocks);
	printf("# zmap blocks:   %d\n", superBlock.s_zmap_blocks);
	printf("First data zone: %d\n", superBlock.s_firstdatazone);
	printf("Log zone size:   %d\n", superBlock.s_log_zone_size);
	printf("Max size:        %d\n", superBlock.s_max_size);
	printf("Magic:           %d\n", superBlock.s_magic);
	printf("State:           %d\n", superBlock.s_state);
	printf("Zones:           %d\n", superBlock.s_zones);
	printf("\n");
}

//this method goes to a zone block and checks to see if the data zone is a zone. If so, 
//it prints out all of the contents of the data zone as characters (not hexadecimal)
void showzone(char *fileName, int zoneNum) {
	char buf[BLOCK_SIZE];
	int fd = open(fileName, O_RDONLY); //open the image file
	if(fd == -1) {
		printf("File error.\n");
		return;
	}
	else {
		if(zoneNum >= superBlock.s_firstdatazone && zoneNum < lastdatazone && checkBitMap(fileName, 0, 0, zoneNum)) {
			//the zone is valid
			char zoneContents[BLOCK_SIZE];
			lseek(fd, zoneNum*BLOCK_SIZE, SEEK_SET);
			read(fd, zoneContents, BLOCK_SIZE);

			//start printing!
			printf("    0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
			int rowNum = 0;
			while(rowNum < 64) {
				printf("%-3x", rowNum*16);
				int colNum = 0;
				while(colNum < 16) {
					int value = 16*rowNum + colNum;
					char c = zoneContents[value];
					if(isprint(c)) {
						//it is ascii character
						printf(" %c ", c);
					}
					else {
						//not ascii character
						printf("   ");
					}
					colNum++;
				}
				printf("\n");
				rowNum++;
			}	
		}
		else {
			//the integer was not within the data zone range
			printf("Not a data zone.\n");
		}
	}
	close(fd);
}

//this method takes a zone as a parameter and prints out the contents of the file one byte at 
//a time -- in hexadecimal notation
//we also know that this is a valid data zone 
void hexDump(char *fileName, unsigned short zoneNumber) {
	int fd = open(fileName, O_RDONLY); //open the image file
	if(fd == -1) {
		printf("File error.\n");
		return;
	}
	else {
		if(zoneNumber >= superBlock.s_firstdatazone && zoneNumber < lastdatazone) {
			//go the the zone
			char zoneContents[BLOCK_SIZE];
			lseek(fd, zoneNumber*BLOCK_SIZE, SEEK_SET);
			read(fd, zoneContents, BLOCK_SIZE);

			//start printing!
			int rowNum = 0;
			while(rowNum < 64) {
				int colNum = 0;
				while(colNum < 16) {
					int value = 16*rowNum + colNum;
					unsigned char c = zoneContents[value];
					printf("   %02x", c);
					colNum++;
				}
				printf("\n");
				rowNum++;
			}
		}	
		printf("\n");
	}
	close(fd);
}


//this method goes to each data zones used by the root directory and either prints out 
//just the names of the files, or the names along with other detailed information, 
//depending on the user input
void traverse(char *fileName, int longTraverse) {
	int fd = open(fileName, O_RDONLY); //open the image file
	if(fd == -1) {
		printf("File error.\n");
		return;
	}
	else {
		//we're going to root directory, which is in the firstInode block
		lseek(fd, firstInode*BLOCK_SIZE, SEEK_SET);
		char inode[INODE_SIZE];
		read(fd, inode, INODE_SIZE);
		struct minix_inode *ip = (struct minix_inode *)inode;
	
		//from here, we can determine where the data blocks are for the root directory
		//we only need to consider the i_nzone[0] through i_nzone[6]
		unsigned short zoneIDs[7];
		int i = 0;
		while(i < 7) {
			zoneIDs[i] = ip->i_nzone[i];
			i++;
		}
	
		i = 0;
		while(i < 7) {
			if(zoneIDs[i] < lastdatazone && zoneIDs[i] >= superBlock.s_firstdatazone && checkBitMap(fileName, 0, 0, zoneIDs[i])) {
				char zoneData[BLOCK_SIZE]; //this will hold file name
				lseek(fd, zoneIDs[i]*BLOCK_SIZE, SEEK_SET); //pointing to zone block
		
				//now, we need to get all the dir_entries
				int k = 0;
				while(k < DIR_ENT_SIZE) {
					char dirent[DIR_ENT_SIZE];
					read(fd, dirent, DIR_ENT_SIZE);
					struct minix_dir_entry *d = (struct minix_dir_entry *)dirent;
					if(strcmp(d->name, "") != 0 && strcmp(d->name, "." ) != 0 && strcmp(d->name, "..") != 0) {
						if(longTraverse) {
							if(checkBitMap(fileName, 1, d->inode, 0)) {
								//this is a valid entry in the inode bit map!
								getLongTraversalInfo(fileName, d->inode, firstInode);
								printf("%s\n", d->name);
							}
						}
						else {
							if(checkBitMap(fileName, 1, d->inode, 0)){
								//this is a valid entry in the inode bit map!
								printf("%s\n", d->name);
							}
						}
					}
					k++;
				}
			}
			i++;
		}
	}
	close(fd);
}

//this method prints out the information on the file (we know at this point that the 
//file is valid in the inode table)
void getLongTraversalInfo(char *file, unsigned short inum, unsigned short rootI) {
	int filed = open(file, O_RDONLY);
	if(filed == -1) {
		printf("File error.\n");
		return;
	}
	else {
		//now we need to figure out which inode block the file is contained in
		//we know that the first inode is rootInode
		unsigned short block_num = inum/INODE_SIZE; // inum/32
		unsigned short inodeBlock = rootI + block_num; //this is the block ID of the inode we're looking for
		unsigned int whichInode = inum%INODE_SIZE; 
		
		lseek(filed, inodeBlock*BLOCK_SIZE, SEEK_SET);
		//we are now pointed at the start of the block 
		lseek(filed, inodeBlock*BLOCK_SIZE + (whichInode - 1)*INODE_SIZE, SEEK_SET);
		//we are now pointed at the specific inode in the correct inode block
		//you need the -1 because if it is the 33rd node, whichInode is 1, but it's really
		//the 0th node in the second block
		
		char inode[INODE_SIZE];
		read(filed, inode, INODE_SIZE);
		struct minix_inode *mi = (struct minix_inode *)inode;
		printf("%c%s%s%s %d %d ", getType(mi->i_mode), getPermissions(mi->i_mode, 0), getPermissions(mi->i_mode, 1), getPermissions(mi->i_mode, 2), mi->i_uid, mi->i_size); 
		getTime(mi->i_time);
	}
	close(filed);
}

//this method gets the type of the file
char getType(unsigned short mode) {
	if(mode >= 32768) {
		//it is a regular file
		return '-';
	}
	else if(mode >= 16384 && mode < 32768) {
		//it is a directory
		return 'd';
	}
	else {
		//it is a different type of file 
		//(we don't have to worry about this part for this project, but i'll add it anyway)
		return ' ';
	}
}

//this method gets the permission bits of the file
char *getPermissions(unsigned short mode, int user) {
	unsigned short permissions;
	if(user == 0) {
		//get owner bits
		permissions = (mode & OWNER_MASK) >> 6; 
	}
	else if(user == 1) {
		//get group bits
		permissions = (mode & GROUP_MASK) >> 3;
	}
	else {
		//get other bits
		permissions = (mode & OTHER_MASK);
	}
	//now we have the permissions bits (a number between 0 and 7)
	switch(permissions) {
		case 0:
			return "---";
		case 1: 
			return "--x";
		case 2: 
			return "-w-";
		case 3:
			return "-wx";
		case 4:
			return "r--";
		case 5:
			return "r-x";
		case 6:
			return "rw-";
		case 7:
			return "rwx";
		default:
			return "   ";
	
	}
}

//this method gets the date of a file based on the number of seconds since the epoch
void getTime(unsigned int seconds) {
	char theTime[100];
	time_t num_seconds = (time_t)seconds;
	struct tm *time = localtime(&num_seconds);
	strftime(theTime, sizeof(theTime), "%b %d %Y",time);
	printf("%s ", theTime);
}

//this method checks whether or not the inode or data zone is valid
//if isInode == 1, we check the imap. else, if isInode == 0, we check the zmap
int checkBitMap(char *fileName, int isInode, unsigned short iNum, unsigned short zoneNum) {
	int fileDes = open(fileName, O_RDONLY);
	//get last imap block
	unsigned short lastIMap = firstImap + superBlock.s_imap_blocks - 1; 
	//the first zmap block is after the last imao
	unsigned short firstZMap = lastIMap + 1;
	//get last zmap block
	unsigned short lastZMap = firstZMap + superBlock.s_zmap_blocks - 1; 
	
	if(isInode == 1) {
		//we are checking to see if the INODE is valid or not
		if(iNum == 1) {
			//we are checking to see if the ROOT inode is valid
			//this is index 1 in the first inode bit map -- go to firstIMap
			char iMapCheck[1];
			lseek(fileDes, firstImap*BLOCK_SIZE, SEEK_SET);
			read(fileDes, iMapCheck, 1); //read first 8 bits from imap
			unsigned short imaps = iMapCheck[0] >> 6; 
			//the bit we want to check is now the least significant bit
			if(imaps % 2 == 0) {
				//even -- so not set and therefore not valid
				return 0;
			}
			else {
				//that bit is set
				return 1;
			}
		}
		else {
			//we are checking the validity of an inode OTHER than the root directory inode
			//find what imap block this inode bit is in
			
			unsigned short imap_block_number = iNum/8192; 
			unsigned short imap_block_ID = firstImap + imap_block_number;
			unsigned short imap_byte_num = iNum/8; 
			
			char bitmap_byte[1];
			lseek(fileDes, imap_block_ID*BLOCK_SIZE + imap_byte_num, SEEK_SET);
			read(fileDes, bitmap_byte, 1); //byte now in bitmap_byte[0]
			
			//shift to get the appropriate bit in last location
			unsigned short num_shifts = 7 - (imap_byte_num % 8);
			unsigned short after_shifting = bitmap_byte[0] >> num_shifts;
			
			if(after_shifting % 2 == 0) {
				//this is an even number, so bit not set
				return 0;
			}
			else {
				//bit set
				return 1;
			}

		}
	}
	else {
		//we are checking to see if the ZONE is valid or not
		unsigned short zone = zoneNum - superBlock.s_firstdatazone;
		unsigned short zmap_block_number = zone/8192;
		unsigned short zmap_block_ID = firstZMap + zmap_block_number;
		//printf("\n");
		//printf("zone: %d zmap: %d\n", zone, zmap_block_ID);
		unsigned short zmap_byte_num = zone/8;
		
		char zbitmap_byte[1];
		lseek(fileDes, zmap_block_ID*BLOCK_SIZE + zmap_byte_num, SEEK_SET);
		read(fileDes, zbitmap_byte, 1); //byte now in zbitmap_byte[0]
		
		//shift to get the appropriate bit in last location
		unsigned short znum_shifts = 7 - (zmap_byte_num % 8);
		unsigned short zafter_shifting = zbitmap_byte[0] >> znum_shifts;
			
		if(zafter_shifting % 2 == 0) {
			//this is an even number, so bit not set
			return 0;
		}
		else {
			//bit set
			return 1;
		}
		
	}
	close(fileDes);
}

//this method determines which inode to go to by going through all of the directory entries 
//in the root directory data zone and finding which one matches the desired file name
//if the file name is not found, it prints "file not found"
void showfile(char *fileName, char *file) {
	int fileDescriptor = open(fileName, O_RDONLY);
	if(fileDescriptor == -1) {
		printf("File error.\n");
		return;
	}
	else {
		char rootINode[INODE_SIZE];
		lseek(fileDescriptor, firstInode*BLOCK_SIZE, SEEK_SET); //now at inode block 
		read(fileDescriptor, rootINode, INODE_SIZE);
		
		struct minix_inode *inode = (struct minix_inode *)rootINode;
		
		//determine data zones to find the data 
		unsigned short zoneID[7];
		int i = 0;
		while(i < 7) {
			zoneID[i] = inode->i_nzone[i];
			i++;
		}
		
		//go through each data zone for the root directory
		i = 0;
		while(i < 7) {
			if(zoneID[i] < (superBlock.s_firstdatazone + superBlock.s_nzones) && zoneID[i] >= superBlock.s_firstdatazone && checkBitMap(fileName, 0, 0, zoneID[i])) {
				char zoneData[BLOCK_SIZE]; 
				lseek(fileDescriptor, zoneID[i]*BLOCK_SIZE, SEEK_SET); //pointing to zone block
		
				int boolean = 0;
				//now, we need to get all the dir_entries
				int k = 0;
				while(k < DIR_ENT_SIZE) {
					char dirent[DIR_ENT_SIZE];
					read(fileDescriptor, dirent, DIR_ENT_SIZE);
					struct minix_dir_entry *direntry = (struct minix_dir_entry *)dirent;
					//printf("%s %s\n", direntry->name, file);
					if(strcmp(direntry->name, file) == 0 && checkBitMap(fileName, 1, direntry->inode, 0) == 1) {
						//this is the file that we want to print AND it is valid!
						//printf("Entered\n");
						boolean = 1;
						unsigned short dirInum = direntry->inode;
						getContents(fileName, dirInum);
					}
					k++;
				}
				if(boolean == 0) {
					//we never found the file
					printf("File not found.\n");
				}
			}
			i++;
		}
	}
	close(fileDescriptor);
}


//this method goes through all of the data zones associated with the file name (based on 
//its inumber) and then prints out those zones using the hex dump method.
//also, at this point, we know that the inode itself is valid
void getContents(char *fileName, unsigned short Inumber) {
	int fd = open(fileName, O_RDONLY);
	//we need to see how large the file is and how many zones it takes up
	unsigned short inumBlockNumber = Inumber/INODE_SIZE;
	//if it is less than 1, it will be in the first inum block. 
	//if it is greater than 1, it will be in a different inum block.
	
	unsigned short inumBlock = firstInode + inumBlockNumber;
	
	//now figure out which one we are pointing to
	unsigned short inodeNumber = Inumber%INODE_SIZE;
	lseek(fd, inumBlock*BLOCK_SIZE + (inodeNumber-1)*INODE_SIZE, SEEK_SET);
	//now, we are pointing at the correct inode
	
	char inodeContents[INODE_SIZE];
	read(fd, inodeContents, INODE_SIZE);
	
	struct minix_inode *theInode = (struct minix_inode *)inodeContents;
	
	unsigned short zoneBlocks[7];
	int i = 0;
	while(i < 7) {
		zoneBlocks[i] = theInode->i_nzone[i];
		i++;
	}
	i = 0;
	while(i < 7) {
		//printf("Data zone is: %d\n", zoneBlocks[i]);
		if(zoneBlocks[i] < (superBlock.s_firstdatazone + superBlock.s_nzones) && zoneBlocks[i] >= superBlock.s_firstdatazone && checkBitMap(fileName, 0, 0, zoneBlocks[i])) {
			//print out the contents
			//printf("%d is valid!\n", zoneBlocks[i]);
			hexDump(fileName, zoneBlocks[i]);
		}
		i++;
	}
	close(fd);
}

