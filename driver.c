#include <stdio.h> //putchar
#include <iosLib.h> //iosTOOL
#include <errnoLib.h> //errnoSet
//pour afficher dans le shell : printErrno()
#include <string.h> //memset
#include <stdlib.h> //malloc
#include <taskLib.h> //taskIdSelf
#include <semLib.h>
#include "driver.h"

typedef struct fifo
{
	int message;
	struct fifo * next
}fifo;
typedef struct listeFifo
{
	 fifo * first;
	 fifo * last;
	 int taskId;
	 struct listeFifo * next;
} listeFifo;
typedef struct myDev
{
	DEV_HDR devHdr;
	int mode;
	int opened;
	int devNumber;
	struct listeFifo * firstFifo;  
	SEM_ID semFirst;
	struct myDev * next;
} myDev;
myDev * first = NULL;
char reg[4];
int numPilote = -1;

// fonctions standards : open,close,write

int devOpen(myDev * drv, char * name, int mode)
{
	listeFifo * i = 0;
	listeFifo * fifo = malloc(sizeof(listeFifo));
	drv->opened++;
	drv->mode = mode;
	memset(fifo->pointeurFifo,0,SIZE_FIFO);
	fifo->next=NULL;
	fifo->taskId = taskIdSelf();
	semMTake(drv->semFirst,NO_WAIT);
	if (drv->firstFifo==NULL)
	{
		drv->firstFifo = fifo;
	} else {
		//Add fifo at end of listeFifo
		for (i=drv->firstFifo;i->next!=NULL;i=i->next);
		i->next=fifo;
	}
	semMGive(drv->semFirst);
	return (int)drv; //pointer returned is used as arg in devClose!
}
int devClose(myDev * drv)
{
	listeFifo * i = NULL; 
	if(drv==NULL)
	{
		errnoSet(NOT_OPEN);
		return -1;
	}
	if (drv->opened == 0)
	{
		errnoSet(NOT_OPEN);
		return -1;
	} else {
		semMTake(drv->semFirst,NO_WAIT);
		//Find fifo in drv->listFifo
		if (drv->firstFifo->taskId==taskIdSelf())
		{
			drv->firstFifo=drv->firstFifo->next;
		} else {
			for (i=drv->firstFifo;i->next->taskId==taskIdSelf();i=i->next);
			free(i->next);
			i->next=i->next->next;
		}
		semMGive(drv->semFirst);
		drv->opened--;
	}
	return 0;
}
int devRead(myDev * drv, char * data, int dataSize)
{
	
}
/*
int devWrite(myDev * drv, char * data, int dataSize)
{
	int i = 0;
	if (drv->opened != 1)
	{
		errnoSet(NOT_OPEN);
		return -1;
	}
	//printf("You wrote :\n");
	for (i=0; i<dataSize && data[i] != '\n'; i++)
	//warning, 2 letters can be submitted
	{
		putchar(data[i]);
	}
	putchar('\n');
	//printf("\nCongrats!!\n");
	return i;
}
*/
void isr()
{
	
}

int devIoctl(myDev * drv, int function, void * args )
{
	int result = -1000;
	typedef struct{
		myDev * drv;
		char * data;
		int dataSize;
	} zarbArgs;
	switch (function){
	//TODO: Simple function that prints "lol", ten times
	case TEST:
		break;
	//TODO: TEST !!!
	/*case ZARB:
		zarbArgs * newArgs = malloc(sizeof(zarbArgs));
		memcpy(newArgs,args,sizeof(zarbArgs));
		result = devWriteZarb(newArgs->drv, newArgs->data,newArgs->dataSize);
		free(newArgs);
		return result;
		break;
	*/
	default :
		errnoSet(UNKNOWN_FUNCTION);
		return -1;
	}
}

//fonctions maintenance
int drvInstall()
{
	if (numPilote != -1)
	{
		errnoSet(ALREADY_INSTALLED);
		return -1;
	}
	numPilote=iosDrvInstall(0,0,devOpen,devClose,0,0,devIoctl);
	if (numPilote == -1)
	{
		errnoSet(INSTALL_ERR);
		return -1;
	}else{
		return 0;
	}
	return 0;
//	return numPilote == -1 ? -1 : 0;
}

int devAdd(char * devName,int devNb)
{
	myDev * dev;
	myDev * i;
	int ret = 0;
	dev = malloc(sizeof(myDev));
	dev->next = NULL;
	if (dev==NULL)
	{
		errnoSet(NOT_ENOUGH_MEMORY);
		return -1;
	}
	if (numPilote == -1)
	{
		errnoSet(NOT_INSTALLED);
		return -1;
	}
	ret = iosDevAdd((DEV_HDR*)dev,devName,numPilote);
	if (ret == -1)
	{
		errnoSet(DEV_ADD_ERR);
		return -1;
	}else{
		dev->semFirst = semMCreate(SEM_INVERSION_SAFE|SEM_Q_PRIORITY);
		printf("%d\n",dev->semFirst);
		if(dev->semFirst==0)
		{
			free(dev);
			errnoSet(INSTALL_ERR);
			return -1;
		}
		//SEM_INVERSION_SAFE prevents devOpen from being interupted
		//while adding fifo to list
		dev->devNumber = devNb;
		dev->firstFifo = NULL;
		if (first==NULL)
		{
			first = dev;
		}else{
			for (i=first;i->next!=NULL;i=i->next);
			i->next = dev;
		}
		return 0;
	}
	return 0;
}

int devDelete(char* devName)
{
	myDev * i = first;
	myDev* drv = (myDev*) iosDevFind (devName,NULL);
	if (drv==NULL)
	{
		errnoSet(UNKNOWN_DEVICE);
		return -1;
	}
	if (drv==first)
	{
		first=drv->next;
	} else {
		for (i=first;i->next!=drv;i=i->next);
		i->next=drv->next;
	}
	iosDevDelete((DEV_HDR*)drv);
	semDelete(drv->semFirst);
	free(drv);
	return 0;
}

int drvRemove()
{
	//TODO: Fix the bug
	int status;
	//For the TP, test when a person forget to remove all devices
	//THE SOLUCE IS IN THE FUNCTION REMOVE
	if (numPilote == -1)
	{
		errnoSet(NOT_INSTALLED);
		return -1;
	}
	iosDrvRemove(numPilote,1);
	if (status == -1)
	{
		errnoSet(REMOVE_ERROR);
		return -1;
	}
	numPilote = -1;
	return 0;
//	return numPilote == -1 ? -1 : 0;
}

/*TEST*/
/*void hello_world()
{
	printf("Hello World\n");
}*/
