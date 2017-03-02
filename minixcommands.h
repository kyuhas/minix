#ifndef MINIXCOMMANDS_HEADER_FILE
#define MINIXCOMMANDS_HEADER_FILE

#define OWNER_MASK 0x01c0
#define GROUP_MASK 0x0038
#define OTHER_MASK 0x0007

struct minix_super_block superBlock;
unsigned short lastdatazone;
unsigned short firstImap;
unsigned short firstZmap;
unsigned short firstInode;

void help();
int quit();
void showsuper(char *fileName);
void showzone(char *fileName, int zoneNum);
void traverse(char *fileName, int longTraverse);
void getLongTraversalInfo(char *file, unsigned short inum, unsigned short rootI);
char getType(unsigned short mode);
char *getPermissions(unsigned short mode, int user);
void getTime(unsigned int seconds);
int checkBitMap(char *fileName, int isInode, unsigned short iNum, unsigned short zoneNum) ;
void hexDump(char *fileName, unsigned short zoneNumber) ;
void showfile(char *fileName, char *file);
void getContents(char *fileName, unsigned short Inumber);
int copyToMemory(char *fileName);

#endif