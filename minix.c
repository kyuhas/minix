#include "minix.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "minixcommands.h"

int main() {
	//boolean will be false once the user types in "quit"
	int boolean = 1;
	char fileName[100]; //can hold up to 100 characters
	fileName[0] = '\0'; //initialize the first value of fileName
	
	while(boolean) {
		char buffer[7] = "minix: ";
		write(1, buffer, 7);
		
		char *commands = (char *)malloc(sizeof(char) * 1024); 
		readCommands(commands);
		//now, commands holds the user input, null terminated
		
		int argc = 0;
		int *args = &argc;
		char **argv;
		argv = parseCommands(commands, args);
		
		boolean = determineCommand(argc, argv, fileName);
		
	}
	
	return 0;
}

//this method parses through the commands input by the user and adds a null terminator to
//the end, so that it can be read
void readCommands(char *commands) {
	//get input from user -- commands now holds the input
	ssize_t bytesRead = read(0, commands, 1024);
	
	//check to see where the actual input ends in commands
	int index = 0;
	while(index < 1024) {
		if(commands[index] == EOF || commands[index] == '\n') {
			//end of user input -- replace with '\0'
			commands[index] = '\0';
			return;
		}	
		index++;
	}
	//at this point, if it is not reached, that means the user input something that took up
	//the entire array
	commands[1023] = '\0';
	return;
}

//this method parses through the commands input by the user and determines places each 
//word (separated by white space) into an argument array (called argv)
char **parseCommands(char *commands, int *args) {
	//argv can hold up to 100 different arguments
	char **argv = (char **)malloc(sizeof(char *) * 100);
	char *token;
	
	int index = 0;
	token = strtok(commands, " \t\r\n");

	while(token != NULL) {
		//put argument number into argv array
		argv[index] = token; 
		index++;
		token = strtok(NULL, " \t\r\n");
	} 
	
	//at this point, all the tokens have been made
	*args = index;
	if(index < 100) {
		argv[index] = NULL;
	}
	return argv;
}

//this method determines if a certain file is a .img file or not -- if value == 0, it is
//an incorrect image file. Else, if value == 1, it WAS an image file.
int isImageFile(char *file) {
	int value;
	char *extension;
	extension = strrchr(file, '.');
	if(extension == NULL) {
		//there was NOT an extension in this file
		value = 0;
		printf("Invalid minix image file.\n");
	}
	else {
		//there WAS a dot in the file
		if(strcmp(extension, ".img") == 0) {
			//it is a .img file
			value = 1;
		}
		else {
			value = 0;
			printf("Invalid minix image file.\n");
		}
	}
	return value;
}

//this method takes the user input commands and determines what the user wants the program
//to do. For this project, the maximum number of arguments passed should be 2 -- therefore, 
//if argc > 2, there will be an error output.
int determineCommand(int argc, char **argv, char *fileName) {
	int boolean = 1;
		if(argc == 1 || argc == 2) {
			//QUIT
			if(strcmp(argv[0], "quit") == 0) {
				if(argc != 1) {
					printf("Too many arguments.\n");
				}
				else {
					boolean = quit();				
				}
			}
			//HELP
			else if(strcmp(argv[0], "help") == 0) {
				if(argc != 1) {
					printf("Too many arguments.\n");
				}
				else {
					help();				
				}
			}
			//SHOWSUPER
			else if(strcmp(argv[0], "showsuper") == 0) {
				if(argc != 1) {
					printf("Too many arguments.\n");
				}
				else {
					if(fileName[0] == '\0') {
						printf("You need to mount the image file first!\n");
					}
					else { 
						//it is mounted already and therefore available
						showsuper(fileName); 
					}			
				}
			}
			//TRAVERSE
			else if(strcmp(argv[0], "traverse") == 0) {
				if(fileName[0] == '\0') {	
					printf("You need to mount the image file first!\n");
				}
				else {
					if(argc == 2) {
						if(strcmp(argv[1], "-l") == 0) {
							//do long list traversal
							printf("\n");
							traverse(fileName, 1);
							printf("\n");
						}
						else {
							printf("Invalid input\n");
						}
					}
					else {
						//do short list traversal
						printf("\n");
						traverse(fileName, 0);
						printf("\n");
					}
				}
			}
			//MINIMOUNT
			else if(strcmp(argv[0], "minimount") == 0) {
				if(argc != 2) {
					printf("Invalid input. Match format: 'minimount argument'.\n");
				}
				else {	
					//they included an argument--check to see if it is a .img file
					int img = isImageFile(argv[1]);
					if(img == 1) {
						//the argument was a .img file
						strcpy(fileName, argv[1]);
						int copied = copyToMemory(fileName);
						if(copied == 1) {
							printf("The file %s has been successfully mounted.\n", fileName);
						}
						else {
							//file error -- prints error message in other method
							strcpy(fileName, "\0");
						}
					}
					else {
						//the argument was not a .img file
						printf("Invalid input.\n");
					}
				}
			}
			//MINIUMOUNT
			else if(strcmp(argv[0], "miniumount") == 0) {
				if(fileName[0] == '\0') {
					printf("No image file is mounted.\n");
				}
				else {
					if(argc != 1) {
						printf("Too many arguments.\n");
					}
					else {
						printf("%s has been successfully unmounted.\n", fileName);
						strcpy(fileName, "\0");
					}
				}
			}
			//SHOWZONE
			else if(strcmp(argv[0], "showzone") == 0) {
				if(argc == 1) {
					printf("Invalid format.\n");
				}
				else {
					//there IS a second argument
					if(fileName[0] == '\0') {
						printf("You need to mount the image file first!\n");
					}
					else {
						//check to see if second argument is a number
						int zoneNum = atoi(argv[1]);
						if(zoneNum == 0 && argv[1][0] != '0') {
							//the number the user gave was NOT an integer
							printf("Invalid format. Please enter 'showzone' followed by an integer.\n");
						}
						else {
							//the number the user gave WAS an integer
							printf("\n");
							showzone(fileName, zoneNum);
							printf("\n");
						}
					}
				}
			}
			//SHOWFILE 
			else if(strcmp(argv[0], "showfile") == 0) {
				if(argc == 1) {
					printf("Invalid format.\n");
				}
				else {
					//there is a second command!
					if(fileName[0] == '\0') {
						printf("You need to mount the image file first!\n");
					}
					else {
						printf("\n");
						showfile(fileName, argv[1]);
						printf("\n");
					}
				}
			}
			//OTHER COMMAND THAT IS NOT VALID 
			else {
				printf("Invalid command.\n");
			}
		}
		
		//the user typed in either 0 arguments or more than 2 arguments
		else {
			printf("Incorrect number of arguments.\n");
		}
		
	return boolean;
}
