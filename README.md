# minix
This program allows users to access a locally mounted floppy disk.

Author: Kaylee Yuhas

Project Description: This project allows a user to access a minix disk locally mounted onto 
a computer. It allows to use the following commands: help, minimount argument, miniumount, 
showsuper, traverse [-l], showzone [zone number], showfile [file name], and quit. When minimount 
is called, the fileName is initialized to the image file name (i.e. imagefile.img). From here,
any other commands that require an image file gets passed the image file name, which opens a file
descriptor. Also when minimount is called, the superblock is loaded into memory.
