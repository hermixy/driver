#include <stdio.h> //putchar
#include <iosLib.h> //iosTOOL
#include <errnoLib.h> //errnoSet
//pour afficher dans le shell : printErrno()
#include <string.h> //memset
#include <stdlib.h> //malloc
#include "driver.h"

typedef struct myDev
{
	DEV_HDR devHdr;
	int mode;
	int opened;
	char data[tailleMax];
	int devNumber;
	struct myDev * next;
} myDev;
myDev * first = NULL;
int numPilote = -1;
int fakeReg;

// fonctions standards : open,close,write

int devOpen(myDev * drv)
{
	if (drv->opened == 1)
	{
		errnoSet(ALREADY_OPEN);
		return -1;
	} else 
	{
		drv->opened = 1;
		memset(drv->data,0,tailleMax);
		return (int)drv;
	}
}
int devClose(myDev * drv)
{
	if (drv->opened == 1)
	{
		drv->opened =0;
		return 0;
	} else {
		errnoSet(NOT_OPEN);
		return -1;
	}
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

int devWriteZarb(myDev * drv, char * data, int dataSize)
{
	int i = 0;
	if (drv->opened != 1)
	{
		errnoSet(NOT_OPEN);
		return -1;
	}
	for (i=0; i<dataSize && data[i] != '\n'; i++)
	{
		putchar(data[i]);
		putchar(data[i]);
	}
	return i;
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
		dev->devNumber = devNb;
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

void handler()
{

}
/*TEST*/
/*void hello_world()
{
	printf("Hello World\n");
}*/
