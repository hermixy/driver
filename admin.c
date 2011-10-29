#include "driver.h"
#include <stdio.h>
#include <iosLib.h>
#include <errnoLib.h>
int testStart()
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
		printf("FAIL : %d!\n",errnoGet());
		return -1;
	}
	retInstall = drvInstall();
	printf("drvInstall overwrite returned %d, ",retInstall);
	if (retInstall==-1)
	{
		printf("OK! : %d\n",errnoGet());
	} else {
		printf("FAIL!\n");
		return -1;
	}
	iosDrvShow();
	getchar();
	
	printf("Device add test(lol,1):\n");
	retAdd = devAdd("lol",1);
	printf("devAdd returned %d, ",retAdd);
	if (retAdd==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL : %d!\n",errnoGet());
		return -2;
	}
	printf("Same name device add test(lol,2):\n");
	retAdd = devAdd("lol",2);
	if (retAdd==-1)
	{
		printf("OK! : %d\n",errnoGet());
	} else {
		printf("FAIL!\n");
		return -2;
	}
	printf("Same number device add test(lol1,1):\n");
	retAdd = devAdd("lol1",1);
	if (retAdd==-1)
	{
		printf("OK! : %d\n",errnoGet());
	} else {
		printf("FAIL!\n");
		return -2;
	}
	printf("Add other devices test(lol1,2):\n");
	retAdd = devAdd("lol1",2);
	printf("devAdd returned %d, ",retAdd);
	if (retAdd==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL : %d!\n",errnoGet());
		return -2;
	}
	printf("Add other devices test(lol2,3):\n");
	retAdd = devAdd("lol2",3);
	printf("devAdd returned %d,",retAdd);
	if (retAdd==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL : %d!\n",errnoGet());
		return -2;
	}
	
	printf("Add other devices test(lol3,4):\n");
	retAdd = devAdd("lol3",4);
	printf("devAdd returned %d, ",retAdd);
	if (retAdd==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL : %d!\n",errnoGet());
		return -2;
	}
	printf("Add other devices test(lol4,5):\n");
	retAdd = devAdd("lol4",5);
	printf("devAdd returned %d, ",retAdd);
	if (retAdd==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL : %d!\n",errnoGet());
		return -2;
	}
	iosDevShow();
	getchar();

	printf("Test deletion of innexistant device\n");
	retDelete = devDelete("lol42");
	printf("devDelete returned %d, ",retDelete);
	if (retDelete==-1)
	{
		printf("OK! : %d\n",errnoGet());
	} else {
		printf("FAIL!\n");
		return -3;
	}
	
	printf("Test deletion of device in the midle of linked list\n");
	retDelete = devDelete("lol2");
	printf("devDelete returned %d, ",retDelete);
	if (retDelete==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL : %d!\n",errnoGet());
		return -3;
	}
	printf("Test deletion of device in front of linked list\n");
	retDelete = devDelete("lol");
	printf("devDelete returned %d, ",retDelete);
	if (retDelete==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL : %d!\n",errnoGet());
		return -3;
	}
	printf("Test deletion of device at end of linked list\n");
	retDelete = devDelete("lol4");
	printf("devDelete returned %d,",retDelete);
	if (retDelete==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL : %d!\n",errnoGet());
		return -3;
	}	
	iosDevShow();
	return 0;
}
int testStop()
{
	int retRemove;
	printf("Test brutal removal of driver, with open devices\n");
	retRemove = drvRemove();
	printf("drvRemove returned %d,",retRemove);
	if (retRemove==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL : %d!\n",errnoGet());
		return -4;
	}	
	iosFdShow();
	iosDevShow();
	iosDrvShow();
	return 0;
}
int testClose()
{
	int ret;
	printf("Test removal of openned devices(lol1)\n");
	ret = devDelete("lol1");
	printf("drvDelete returned %d,",ret);
	if (ret==-1)
	{
		printf("OK : %d\n",errnoGet());
	} else {
		printf("FAIL!\n");
		return -4;
	}	
	printf("Test removal of openned devices(lol3)\n");
	ret = devDelete("lol3");
	printf("drvDelete returned %d,",ret);
	if (ret==-1)
	{
		printf("OK : %d\n",errnoGet());
	} else {
		printf("FAIL!\n");
		return -4;
	}	
	return 0;
	printf("Test removal of closed devices(lol2)\n");
	devAdd("lol2",42);
	ret = devDelete("lol2");
	printf("drvDelete returned %d,",ret);
	if (ret==0)
	{
		printf("OK!\n");
	} else {
		printf("FAIL : %d\n",errnoGet());
		return -4;
	}	
}
