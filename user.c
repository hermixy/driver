#include <ioLib.h>
#include <iosLib.h>
#include <stdio.h>
#include <string.h>
#include <errnoLib.h>
#include "driver.h"
#include "sensors.h"
//Aliases
void fds()
{
	iosFdShow();
}
void drvs()
{
	iosDrvShow();
}
//Test
int testUser()
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
int testSURead()
{
	msg buff[30];
	int ret;
	int i;
	short int devNb = 1;
	short int message = 41;
	int fd;
	memcpy(reg+2,&message,2);
	memcpy(reg,&devNb,2);
	printf("register set.\n");
	isr();
	fd = open("lol1",0,0);
	printf("device openned(lol1), fd =%d\n",fd);
	message++;
	memcpy(reg+2,&message,2);
	printf("register set.\n");
	isr();
	printf("isr called\n");
	
	printf("Test read with buffer to short:\n");
	ret = read(fd,(char*)buff,5);
	printf("read returned : %d\n",ret);
	if (ret == sizeof(msg))
	{
		printf("Fail!\n");
		printf("sensor message : %d\n",buff[0].mdata);
		printf("message count : %d\n",buff[0].mnb);
		printf("message timestamp : %d\n",buff[0].mtimestamp);
	}else if(ret ==-1 )
	{
		printf("OK : %d\n",errnoGet());
	}
	getchar();
	printf("Test read with buffer big enough for one msg:\n");
	ret = read(fd,(char*)buff,sizeof(msg));
	printf("read returned : %d\n",ret);
	if (ret == sizeof(msg))
	{
		printf("sensor message : %d\n",buff[0].mdata);
		printf("message count : %d\n",buff[0].mnb);
		printf("message timestamp : %d\n",buff[0].mtimestamp);
		printf("Ok!\n");
	}else if(ret ==-1 )
	{
		printf("Fail : %d\n",errnoGet());
	}
	getchar();
	message=43;
	memcpy(reg+2,&message,2);
	isr();
	message=44;
	memcpy(reg+2,&message,2);
	isr();
	message=45;
	memcpy(reg+2,&message,2);
	isr();
	message=46;
	memcpy(reg+2,&message,2);
	isr();
	printf("4 new messages sent.\n");
	
	printf("Test read with buffer bigger than one msg:\n");
	ret = read(fd,(char*)buff,sizeof(msg)+5);
	printf("read returned : %d\n",ret);
	if (ret == sizeof(msg))
	{
		printf("sensor message : %d\n",buff[0].mdata);
		printf("message count : %d\n",buff[0].mnb);
		printf("message timestamp : %d\n",buff[0].mtimestamp);
		printf("Ok!\n");
	}else if(ret ==-1 )
	{
		printf("Fail : %d\n",errnoGet());
	}
	getchar();
	printf("Test read with buffer far to big");	
	ret = read(fd,(char*)buff,10*sizeof(msg));
	printf("read returned : %d\n",ret);
	if(ret ==-1 )
	{
		printf("Fail : %d\n",errnoGet());
	}else{
		for(i=1; i*sizeof(msg)<=ret; i++)
		{
			printf("sensor message : %d\n",buff[i-1].mdata);
			printf("message count : %d\n",buff[i-1].mnb);
			printf("message timestamp : %d\n",buff[i-1].mtimestamp);
		}
		printf("Ok!\n");
	}
	
	getchar();
	for (i=0; i<22; i++)
	{
		message=20+i;
		memcpy(reg+2,&message,2);
		isr();
	}
	printf("22 new messages sent.\n");
	printf("Test read more than 20 messages\n");	
	ret = read(fd,(char*)buff,25*sizeof(msg));
	printf("read returned : %d\n",ret);
	if(ret ==-1 )
	{
		printf("Fail : %d\n",errnoGet());
	}else{
		for(i=1; i*sizeof(msg)<=ret; i++)
		{
			printf("sensor message : %d\n",buff[i-1].mdata);
			printf("message count : %d\n",buff[i-1].mnb);
			printf("message timestamp : %d\n",buff[i-1].mtimestamp);
		}
		printf("Ok!\n");
	}
	close(fd);
	return 0;
}
