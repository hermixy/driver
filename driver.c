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
SEM_ID semMAdmin; 	//Allowing multiple acces to the admin API makes no sense,
					//and can be dangerous in numerous occasions, therefore we forbid it. 

/* 
 * ios/user functions API : open,close,read 
 */
int devOpen(myDev * drv, char * name, int mode)
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
	if(semTake(drv->semMData,WAIT_FOREVER)!=OK)
	{
		free(newFifo);
		errnoSet(SEM_ERR);
		return -1;
	}
	drv->openned++;		//keep track of the number of tasks that have openned this device
	drv->mode = mode;	//this seems useless, todo : check
	//Add fifo at end of listeFifo :
	if (drv->firstFifo==NULL)
	{
		drv->firstFifo = newFifo;
	} else {
		for (i=drv->firstFifo;i->nextFifo!=NULL;i=i->nextFifo);
		i->nextFifo=newFifo;
	}
	semGive(drv->semMData);
	return (int)drv; //pointer returned is used as arg in devClose!
}
int devClose(myDev * drv)
{
	fifo * i = NULL; 
	fifo * temp = NULL;
	if(drv==NULL)
	{
		errnoSet(NOT_OPEN);
		return -1;
	}
	//Prevent anyone from accessing this device while we're modifying it :
	if(semTake(drv->semMData,WAIT_FOREVER)==-1)
	{
		errnoSet(SEM_ERR);
		return -1;
	}
	if (drv->openned == 0)
	{
		errnoSet(NOT_OPEN);
		return -1;
	} else {
		//Find fifo corresponding to this task in drv->listFifo and deallocate it
		if (drv->firstFifo->taskId==taskIdSelf())
		{
			temp=drv->firstFifo;
			drv->firstFifo=temp->nextFifo;
			deleteFifo(temp);
			free(temp);
		} else {
			for (i=drv->firstFifo;i->nextFifo->taskId==taskIdSelf();i=i->nextFifo);
			temp=i->nextFifo;
			i->nextFifo=temp->nextFifo;
			deleteFifo(temp);
			free(temp);
		}
		drv->openned--;
	}
	semGive(drv->semMData);
	return 0;
}
int devRead(myDev * drv, char * data, size_t dataSize)
{
	int taskId;
	fifo * i;
	msg * buff = (msg*)data;
	int buffSize = dataSize/sizeof(msg); //I can't see any reason why sizeof(msg)
										//should change, but let's not take any chances
	int msgsRead = 0;
	taskId = taskIdSelf();
	//we'll only send whole messages, thus if dataSize<sizeof(msg),
	//we can't send anything :
	if (dataSize<sizeof(msg))
	{
		errnoSet(SHORT_BUFF);
		return -1;
	}
	//find out wich fifo is our own:
	for (i=drv->firstFifo; i->taskId=taskId; i=i->nextFifo);
	if (i==NULL)
	{
		//We have nothing to write on? This shouldn't happen either...
		errnoSet(NOT_REGISTERED);
		return -1;
	}
	readFifo(i,&(buff[0]), WAIT_FOREVER); //First call block if fifo empty, cf specs.
	msgsRead++;
	while (msgsRead<buffSize && readFifo(i,&(buff[msgsRead]),NO_WAIT)!=0)
	{
		//Let's read messages until fifo is empty or until the data buffer is 
		//to full to take whole messages
		msgsRead++;
	}
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
	char value[4]; 			//Local storage for isr message
	int sensorNb;
	struct timespec timev;	//Used to store timestamp
	msg message;	
	for (;;)
	{
		if(msgQReceive(isrmq,value,4,WAIT_FOREVER)==ERROR)//get isr message
		{
			sleep(1);
			continue;
		}
		memcpy(&sensorNb,&value,2); 			//get sensor number
		memcpy(&(message.mdata),&value+2,2);	//get sensor value and store it in message
		semTake(semMAdmin,WAIT_FOREVER);
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
				//dispatch message to all apps listening to this device :
				for(j=i->firstFifo; j!=NULL; j=j->nextFifo)
				{
					message.mnb=j->count;
					j->count++;
					clock_gettime(CLOCK_REALTIME, &timev);
					message.mtimestamp=timev.tv_sec*(long)1000+timev.tv_nsec/(long)1000000;
					writeFifo(j,message);
					//We do not care whether the fifo is full or not,
					//as we can't really do anything from here if it is. 
					//Therefore there is no return code test.
				}
				//There is only one logical device per sensor and we have found it,
				//it is then useless to keep on looping onto devices, let's exit isr.
				break;
			}
		}
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
	if (semTake(semMAdmin,NO_WAIT)==-1 && errnoGet()==S_objLib_OBJ_ID_ERROR)
	{//The semaphore hasn't been created yet : the driver has not been installed yet,
	 //let's do it
		semMAdmin = semMCreate(SEM_INVERSION_SAFE|SEM_Q_PRIORITY);
		if(semMAdmin==0)
		{
			errnoSet(SEM_ERR);
			return -1;
		}
	} else {
		semGive(semMAdmin);//If semTake has previously succeded we must now release it
		//This driver has already been installed, return now.
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
	tMsgDispatchID = taskSpawn("tMsgDispatch",1,0,300,msgDispatch,0,0,0,0,0,0,0,0,0,0);
	//This task will dispatch a msg received by the isr and sleep the rest of the time.
	//It needs to be fast to prevent isr msg queue to fill up, hence the high priority
	if (tMsgDispatchID==-1)
	{
		msgQDelete(isrmq);
		errnoSet(TASK_ERR);
		return -1;
	}
	numPilote=iosDrvInstall(0,0,devOpen,devClose,0,0,0); //Register device in ios
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
	dev->semMData = semMCreate(SEM_INVERSION_SAFE|SEM_Q_PRIORITY);
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
	myDev * drv = (myDev*) iosDevFind (devName,&pNameTail);
	if ((*pNameTail)!='\0' || drv==NULL)
	//If pNameTail is not '\0', either we don't have an exact match
	//or we have no match at all
	{
		errnoSet(UNKNOWN_DEVICE);
		semGive(semMAdmin);
		return -1;
	}
	if(semTake(semMAdmin,WAIT_FOREVER)==-1)
	{//The driver probably hasn't been created yet.
		errnoSet(SEM_ERR);
		return -1;
	}
	semTake(drv->semMData,WAIT_FOREVER);
	if (drv->openned != 0)
	{
		//There are still openned file descriptors on this device,
		//we can't delete it, give back semaphores and leave.
		semGive(drv->semMData);
		errnoSet(OPENNED_DEV);
		semGive(semMAdmin);
		return -1;
	}
	iosDevDelete((DEV_HDR*)drv); //This only prevents further oppenings. 
	//Find and delete the device in device list :
	if (drv==first)
	{
		first=drv->next;
	} else {
		for (i=first;i->next!=drv;i=i->next);
		i->next=drv->next;
	}
	semTake(drv->semMData,WAIT_FOREVER); //Let pending ops on this dev finish
	semDelete(drv->semMData); //We don't need to release it to delete it
	free(drv);
	semGive(semMAdmin);
	return 0;
}

int drvRemove()
{
	myDev * i = first;
	myDev * drv;
	if (semTake(semMAdmin,WAIT_FOREVER)==-1)
	{
		errnoSet(SEM_ERR);
		return -1;
	}
	if (numPilote == -1)
	{
		errnoSet(NOT_INSTALLED);
		semGive(semMAdmin);
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
	semDelete(semMAdmin);
	return 0;
}
