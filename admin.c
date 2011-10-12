#include "driver.h"
#include <stdio.h>
#include <iosLib.h>
int test()
{
	int fd = 0;
	int retInstall = 1;
	int retAdd = 1;
	int retDelete = 1;
	printf("Driver install test :\n");
	retInstall = drvInstall();
	printf("drvInstall returned %d, ",retInstall);
	if (retInstall==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL!\n");
		return -1;
	}
	retInstall = drvInstall();
	printf("drvInstall overwrite returned %d, ",retInstall);
	if (retInstall==-1)
	{
		printf("OK!\n");
	} else {
		printf("FAIL!\n");
		return -1;
	}
	getchar();
	printf("Device add test(lol):\n");
	retAdd = devAdd("lol",1);
	printf("devAdd returned %d\n, ",retAdd);
	if (retAdd==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL!\n");
		return -2;
	}
	
	printf("Add other devices test(lol1):\n");
	retAdd = devAdd("lol1",2);
	printf("devAdd returned %d, ",retAdd);
	if (retAdd==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL!\n");
		return -2;
	}
	printf("Add other devices test(lol2):\n");
	retAdd = devAdd("lol2",1);
	printf("devAdd returned %d,",retAdd);
	if (retAdd==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL!\n");
		return -2;
	}
	
	printf("Add other devices test(lol3):\n");
	retAdd = devAdd("lol3",1);
	printf("devAdd returned %d, ",retAdd);
	if (retAdd==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL!\n");
		return -2;
	}
	iosDevShow();
	getchar();
	
	printf("Test deletion of device in the midle of linked list\n");
	retDelete = devDelete("lol2");
	printf("devDelete returned %d, ",retDelete);
	if (retAdd==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL!\n");
		return -3;
	}
	printf("Test deletion of device in front of linked list\n");
	retDelete = devDelete("lol");
	printf("devDelete returned %d, ",retDelete);
	if (retAdd==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL!\n");
		return -3;
	}
	printf("Test deletion of device at end of linked list\n");
	retDelete = devDelete("lol3");
	printf("devDelete returned %d,",retDelete);
	if (retAdd==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL!\n");
		return -3;
	}	
	iosDevShow();
	return 0;
}
