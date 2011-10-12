#include <ioLib.h>
#include <iosLib.h>
#include <stdio.h>


int userTest()
{
	int fd = 0;
	iosFdShow();
	fd = open("lol",0,0);
	printf("File descriptor :%d\n",fd);
	iosFdShow();

}
