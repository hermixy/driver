#include <stdio.h> 		//putchar
#include <iosLib.h> 	//iosTOOL
#include <errnoLib.h>	//errnoSet
//pour afficher dans le shell : printErrno()
#include <string.h> 	//memset
#include <stdlib.h>
#include <taskLib.h> 	//taskIdSelf
#include <semLib.h>
#include <msgQLib.h>
#include <time.h>
#include "driver.h"
#include "fifo.h"

typedef struct myDev
{
	DEV_HDR devHdr;
	struct myDev * next; 	//pointer to next device in device linked-list (one per device)
	SEM_ID semMData; 		//semaphore protecting multiple acces to the following data :
	int mode; 				//nothing useful.
	int openned; 			//allows us to prevent deletion of an open device.
	int devNumber; 			//number corresponding to the physical device id.
	struct fifo * firstFifo;//pointer to fifo linked-list, one per app. 
} myDev;

myDev * first = NULL; //entry point to the device list (protected by semMAdmin)
int numPilote = -1; //useful to test driver installation (protected by semMAdmin)
int tMsgDispatchID; //dispatch task id, needed to delete it at driver removal time
MSG_Q_ID isrmq; 	//msg queue used by isr to communicate with dispatch task 
SEM_ID semMAdmin = 0; 	//Allowing multiple acces to the admin API makes no sense,
					//and can be dangerous in numerous occasions, therefore we forbid it. 

/* 
 * ios/user functions API : open,close,read 
 */
int devOpen(myDev * dev, char * name, int mode)
{
	fifo * i = 0;
	//Create a new fifo for this app and initialize it :
	fifo * newFifo = malloc(sizeof(fifo));
	if(newFifo==NULL)
	{
		errnoSet(MEM_OVERFLOW);
		return -1;
	}
	initFifo(newFifo);
	newFifo->taskId = taskIdSelf();
	//From now on we'll acces the device data, let's require the sem :
	if(semTake(dev->semMData,WAIT_FOREVER)!=OK)
	{
		free(newFifo);
		errnoSet(SEM_ERR);
		return -1;
	}
	dev->openned++;		//keep track of the number of tasks that have openned this device
	dev->mode = mode;	//this seems useless, todo : check
	//Add fifo at end of listeFifo :
	if (dev->firstFifo==NULL)
	{
		dev->firstFifo = newFifo;
	} else {
		for (i=dev->firstFifo;i->nextFifo!=NULL;i=i->nextFifo);
		i->nextFifo=newFifo;
	}
	semGive(dev->semMData);
	return (int)dev; //pointer returned is used as arg in devClose!
}
int devClose(myDev * dev)
{
	fifo * i = NULL; 
	fifo * temp = NULL;
	if(dev==NULL)
	{
		errnoSet(NOT_OPEN);
		return -1;
	}
	//Prevent anyone from accessing this device while we're modifying it :
	if(semTake(dev->semMData,WAIT_FOREVER)==-1)
	{
		errnoSet(SEM_ERR);
		return -1;
	}
	if (dev->openned == 0)
	{
		errnoSet(NOT_OPEN);
		return -1;
	} else {
		//Find fifo corresponding to this task in drv->listFifo and deallocate it
		if (dev->firstFifo->taskId==taskIdSelf())
		{
			temp=dev->firstFifo;
			dev->firstFifo=temp->nextFifo;
			deleteFifo(temp);
			free(temp);
		} else {
			for (i=dev->firstFifo;i->nextFifo->taskId==taskIdSelf();i=i->nextFifo);
			temp=i->nextFifo;
			i->nextFifo=temp->nextFifo;
			deleteFifo(temp);
			free(temp);
		}
		dev->openned--;
	}
	semGive(dev->semMData);
	return 0;
}
int devRead(myDev * dev, char * data, size_t dataSize)
{
	int taskId;
	fifo * i;
	msg * buff = (msg*)data;
	int buffSize = dataSize/sizeof(msg); //I can't see any reason why sizeof(msg)
										//should change, but let's not take any chances
	int msgsRead = 0;
	printf("1\n");
	taskId = taskIdSelf();
	printf("2\n");
	if (dev==NULL)
	{
		errnoSet(NOT_OPEN);
		return -1;
	}
	//we'll only send whole messages, thus if dataSize<sizeof(msg),
	//we can't send anything :
	if (dataSize<sizeof(msg))
	{
		errnoSet(SHORT_BUFF);
		return -1;
	}
	printf("3\n");
	if(semTake(dev->semMData,WAIT_FOREVER)==-1)
	{
		errnoSet(SEM_ERR);
		return -1;
	}
	printf("4\n");
	//find out wich fifo is our own:
	for (i=dev->firstFifo; i!=NULL && i->taskId!=taskId; i=i->nextFifo);
	if (i==NULL)
	{
		errnoSet(NOT_OPEN);
		return -1;
	}
	printf("5\n");
	semGive(dev->semMData);
	printf("6\n");
	if (i==NULL)
	{
		//We have nothing to write on? This shouldn't happen either...
		errnoSet(NOT_REGISTERED);
		return -1;
	}
	printf("7\n");
	readFifo(i,&(buff[0]), WAIT_FOREVER); //First call block if fifo empty, cf specs.
	printf("8\n");
	msgsRead++;
	printf("9, bufferSize=%d, msgsRead=%d\n",buffSize,msgsRead);
	while (msgsRead<buffSize && readFifo(i,&(buff[msgsRead]),NO_WAIT)==0)
	{
		//Let's read messages until fifo is empty or until the data buffer is 
		//to full to take whole messages
		msgsRead++;
		printf("9.1\n");
	}
	printf("10\n");
	return msgsRead*sizeof(msg);
	
}
/* ioctl : unused so far
int devIoctl(myDev * drv, int function, void * args )
{
	int result = -1000;
	switch (function){
	case TEST:
		break;
	default :
		errnoSet(UNKNOWN_FUNCTION);
		return -1;
	}
}
*/
/*
 * Sensors handling functions : isr and msgDispatch
 */
void isr()
{
	char value[4];
	memcpy(value,&reg,4);//copy register to local variable
	msgQSend(isrmq,value,4,NO_WAIT,MSG_PRI_NORMAL); //queue msg
	//If msgQSend fails it probably means that the msg queue hasn't been created
	//yet. But we don't really care in here whether there is someone to listen to
	//us or we just shouting in the wind, thus no return code test.
}
int msgDispatch(void)
//This function runs in an application-independant task, and is woken up periodically
//by the isr, it's role is to process msgs sent by the isr and to put them into the 
//right devices fifos
{
	myDev * i;  
	fifo * j;
	//This previous two variables are used to search through all devices.
	char value[] = {0,0,0,0}; 			//Local storage for isr message
	short int sensorNb = 0;
	struct timespec timev;	//Used to store timestamp
	msg message;	
	for (;;)
	{
		if(msgQReceive(isrmq,value,4,WAIT_FOREVER)==ERROR)//get isr message
		{
			sleep(1);
			continue;
		}
		printf("message received!\n");
		memcpy(&sensorNb,value,2); 			//get sensor number
		memcpy(&(message.mdata),value+2,2);	//get sensor value and store it in message
		printf("sensor : %d, value : %d\n",sensorNb,message.mdata);
		semTake(semMAdmin,WAIT_FOREVER);
		printf("looking for devices...\n");
		//We can't afford to have an acces to the device linked list while we
		//are looking trough it
		
		/*N.B. : if msgQReceive or semTake goes wrong it means there is a serious
		 * issue. Either the obj has been deleted (i can't see why it would be), or
		 * somme other inprobable event has happened. We definitly can not ignore it
		 * as it would mean possible concurential acces to the device linked list
		 * if semTake fails, and, far worse, a infinite loop with nothing to stop the
		 * task if msgQReceive stops blocking. 
		 * That would basically mean a deadlock system since this task's priority is 1.
		 * However if we just killed the task we could as well uninstall the whole driver
		 * as it would become pretty useless, so we just make it sleep and hope it will
		 * work better next time... 
		 */
		for (i=first;i!=NULL;i=i->next)
		{//go through all installed devices
			if (i->devNumber == sensorNb)
			{//this is the logical device corresponding to the sender's id :
				semGive(semMAdmin);//From this point on, we'll only acces the data. 
				printf("found device : %s\n",i->devHdr.name);
				//dispatch message to all apps listening to this device :
				semTake(i->semMData,WAIT_FOREVER);
				for(j=i->firstFifo; j!=NULL; j=j->nextFifo)
				{
					semGive(i->semMData);
					message.mnb=j->count;
					j->count++;
					clock_gettime(CLOCK_REALTIME, &timev);
					message.mtimestamp=timev.tv_sec*(long)1000+timev.tv_nsec/(long)1000000;
					writeFifo(j,message);
					//We do not care whether the fifo is full or not,
					//as we can't really do anything from here if it is. 
					//Therefore there is no return code test.
					semTake(i->semMData,WAIT_FOREVER);
					printf("message written in fifo \n");
				}
				semGive(i->semMData);
				//There is only one logical device per sensor and we have found it,
				//it is then useless to keep on looping onto devices, let's exit isr.
				break;
			}
		}
		semGive(semMAdmin);
	}
	return 0; //just to avoid compiler warnings
}

/*
 * Admin functions API: install of driver and then of devices.
 *						uninstall of devices and then of driver
 *						direct driver uninstall is also supported
 */
int drvInstall()
{
	if (semMAdmin==0)
	{//The semaphore hasn't been created yet : the driver has not been installed yet,
	 //let's do it
		semMAdmin = semMCreate(SEM_Q_FIFO);
		if(semMAdmin==0)
		{
			errnoSet(SEM_ERR);
			return -1;
		}
	} else {
		errnoSet(ALREADY_INSTALLED);
		return -1;
	}
	isrmq = msgQCreate(10,4,0); //Create a msg queue with 10 msg max,
								//4 byte per msg max, and msgs filled up in fifo order
	if (isrmq == NULL)
	{
		errnoSet(MSGQ_ERR);
		return -1;
	}
	tMsgDispatchID = taskSpawn("tMsgDispatch",0,0,1000,msgDispatch,(int)semMAdmin,0,0,0,0,0,0,0,0,0);
	//This task will dispatch a msg received by the isr and sleep the rest of the time.
	//It needs to be fast to prevent isr msg queue to fill up, hence the high priority
	//It's also nice for tests if his priority is superior than the shell's one, which
	//is set to 1
	if (tMsgDispatchID==-1)
	{
		msgQDelete(isrmq);
		errnoSet(TASK_ERR);
		return -1;
	}
	numPilote=iosDrvInstall(0,0,devOpen,devClose,devRead,0,0); //Register device in ios
	if (numPilote == -1)
	{
		msgQDelete(isrmq);
		taskDelete(tMsgDispatchID);
		errnoSet(INSTALL_ERR);
		return -1;
	}
	return 0;
}

int devAdd(char * devName,int devNb)
{
	myDev * dev;
	myDev * i;
	int ret = 0;
	if (devNb == 0)
	{//No sensor have this number, do not add device. 
	 //This test is not very useful, since we can't initiate the device,
	 //we can't make sure a corresponding physical device exist anyway. 
		errnoSet(INVALID_ARG);
		return -1;
	}
	dev = malloc(sizeof(myDev));
	//myDev has to be dynamically allocated since it will be needed 'till devDelete
	//where it'll then be deallocated.
	if (dev==NULL)
	{//we can't allocate memory for drv struct => we can't add driver
		errnoSet(MEM_OVERFLOW);
		return -1;
	}
	dev->next = NULL; //it's the last one in linked list
	dev->devNumber = devNb;
	dev->firstFifo = NULL; //no one has openned this device just yet
	dev->openned = 0;
	dev->semMData = semMCreate(SEM_Q_FIFO);
	//semMDATA will prevent multiple acces to device members used by apps during
	//open, read and close
	if(dev->semMData==0)
	{
		free(dev);
		errnoSet(SEM_ERR);
		return -1;
	}
	if(semTake(semMAdmin,WAIT_FOREVER)==-1)
	{
		semDelete(dev->semMData);//semaphore
		free(dev);
		errnoSet(SEM_ERR);
		return -1;
	}
	if (numPilote == -1)
	{
		semDelete(dev->semMData);//semaphore
		free(dev);
		errnoSet(NOT_INSTALLED);
		return -1;
	}
	//Let's queue this device to the driver device-linked-list.
	if (first==NULL)
	{//This is very first device added to this driver
		first = dev;
	}else{
		//Let's look for the end of linked list and make sure there is no other
		//device with the same number already installed
		for (i=first;i->next!=NULL;i=i->next)
		{
			if (i->devNumber == devNb)
			{//There is already a device with this number, cancel this one's installation
				semGive(semMAdmin);
				semDelete(dev->semMData);//semaphore
				free(dev);
				errnoSet(DEV_ALREADY_CREATED);
				return -1;
			}
		}
		if (i->devNumber == devNb)
		{//The last device already has this number, same thing than above
			semGive(semMAdmin);
			semDelete(dev->semMData);//semaphore
			free(dev);
			errnoSet(DEV_ALREADY_CREATED);
			return -1;
		}
		i->next = dev; //queue this device to linked list
		//We might simplify this one by using a while, or a flag to indicate last element.
	}
	ret = iosDevAdd((DEV_HDR*)dev,devName,numPilote);
	if (ret == -1)
	{
		//We couldn't register device, let's free allocated ressources:
		if (first == dev) //linked list
		{
			first=NULL;
		}else{
			i->next=NULL;
		}
		semGive(semMAdmin);
		semDelete(dev->semMData);//semaphore
		free(dev); //memory
		errnoSet(DEV_ADD_ERR);
		return -1;
	}
	semGive(semMAdmin);
	return 0;
}

int devDelete(char* devName)
{
	myDev * i = first;
	char * pNameTail;
	myDev * dev = (myDev*) iosDevFind (devName,&pNameTail);
	if ((*pNameTail)!='\0' || dev==NULL)
	//If pNameTail is not '\0', either we don't have an exact match
	//or we have no match at all
	{
		errnoSet(UNKNOWN_DEVICE);
		semGive(semMAdmin);
		return -1;
	}
	if(semTake(semMAdmin,WAIT_FOREVER)==-1)
	{
		errnoSet(SEM_ERR);
		return -1;
	}
	semTake(dev->semMData,WAIT_FOREVER);
	if (dev->openned != 0)
	{
		//There are still openned file descriptors on this device,
		//we can't delete it, give back semaphores and leave.
		semGive(dev->semMData);
		errnoSet(OPENNED_DEV);
		semGive(semMAdmin);
		return -1;
	}
	iosDevDelete((DEV_HDR*)dev); //This only prevents further oppenings. 
	//Find and delete the device in device list :
	if (dev==first)
	{
		first=dev->next;
	} else {
		for (i=first;i->next!=dev;i=i->next);
		i->next=dev->next;
	}
	semTake(dev->semMData,WAIT_FOREVER); //Let pending ops on this dev finish
	semDelete(dev->semMData); //We don't need to release it to delete it
	free(dev);
	semGive(semMAdmin);
	return 0;
}

int drvRemove()
{
	myDev * i = first;
	myDev * drv;
	if (semMAdmin == 0)
	{
		errnoSet(NOT_INSTALLED);
		return -1;
	}
	if (semTake(semMAdmin,WAIT_FOREVER)==-1)
	{
		errnoSet(SEM_ERR);
		return -1;
	}
	if (iosDrvRemove(numPilote,1) == -1) //And force closure of open files
	{
		errnoSet(REMOVE_ERROR);
		semGive(semMAdmin);
		return -1;
	}
	taskDelete(tMsgDispatchID);
	msgQDelete(isrmq);
	//Delete all devices  : 
	while (i!=NULL)
	{
		drv = i;
		i = drv->next;
		iosDevDelete((DEV_HDR*)drv);
		semTake(drv->semMData,WAIT_FOREVER); //Let pending ops finish
		semDelete(drv->semMData);
		free(drv);
	}
	numPilote = -1;
	first = NULL;
	semDelete(semMAdmin);
	semMAdmin=0;
	return 0;
}
