#include <ioLib.h>
#include <iosLib.h>
#include <stdio.h>


int userTest()
{
	int i = 0;
	int ret = 0;
	int fd[6] = 0;
	printf("Open tests:\n");
	for (i=0; i<4; i++)
	{
		fd[i]=open("lol1",0,0);
		printf("open returned : %d,",fd[i]);
		if (fd[i]==0)
		{
			printf("Fail.\n");
		} else {
			printf("Ok.\n");
		}
	}
	for (i=4; i<6; i++)
	{
		fd[i]=open("lol3",0,0);
		printf("open returned : %d,",fd[i]);
		if (fd[i]==0)
		{
			printf("Fail.\n");
		} else {
			printf("Ok.\n");
		}
	}
		
	iosFdShow();
	getchar();
	
	printf("Close tests:\n");
	printf("Test close first elem of linked list:\n");
	ret = close(fd[4]);
	printf("Close returned : %d, ", ret);
	if (ret==-1)
	{
		printf("Fail.\n");
	} else {
		printf("Ok.\n");
	}
	
	printf("Test close last elem of linked list:\n");
	ret = close(fd[3]);
	printf("Close returned : %d, ", ret);
	if (ret==-1)
	{
		printf("Fail.\n");
	} else {
		printf("Ok.\n");
	}
	
	printf("Test close elem in linked list:\n");
	ret = close(fd[1]);
	printf("Close returned : %d, ", ret);
	if (ret==-1)
	{
		printf("Fail.\n");
	} else {
		printf("Ok.\n");
	}
	
	iosFdShow();

}
