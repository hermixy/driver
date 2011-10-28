#include "fifo.h"
#include <types.h>
#include <stdio.h>
#include <semLib.h>
int initFifo(fifo * iFifo)
{
	if (iFifo->initialized==1)
	{
		return -1;
	}
	iFifo->semMData = semMCreate(SEM_INVERSION_SAFE|SEM_Q_PRIORITY);
	if (iFifo->semMData==NULL)
	{
		return -1;
	}
	iFifo->semCCount = semCCreate(SEM_Q_PRIORITY,0);
	if (iFifo->semCCount==NULL)
	{
		semDelete(iFifo->semMData);
		return -1;
	} else {
		iFifo->start = 0;
		iFifo->stop = 0;
		iFifo->count = 0;
		iFifo->nextFifo = NULL;
		iFifo->initialized = 1;
	}
}
void deleteFifo(fifo * dFifo)
{
	semTake(dFifo->semMData,WAIT_FOREVER);
	semDelete(dFifo->semCCount);
	semDelete(dFifo->semMData);
	dFifo->initialized=0;
}
/*
 * In both write and read functions we never take boths mutex and count sems at 
 * the same time, therefore we're sure to avoid race conditions.
 */
int writeFifo(fifo * wFifo, msg data )
{
	//write data at end of fifo
	semTake(wFifo->semMData,WAIT_FOREVER);
	wFifo->mem[wFifo->stop]=data;
	//inc stop pointer
	wFifo->stop++;
	wFifo->stop=(wFifo->stop)%(SIZE_FIFO+1);
	if (wFifo->stop == wFifo->start)
	{
		//fifo full, discard first message : inc start pointer
		//and do not add a token into the count sem.
		wFifo->start++;
		wFifo->start=(wFifo->start)%(SIZE_FIFO+1);
		semGive(wFifo->semMData);
		return 1;
	}
	semGive(wFifo->semMData);
	semGive(wFifo->semCCount);
	return 0;
}
int readFifo(fifo * rFifo, msg * data, int timeout)
{
	int ret = 0;
	semTake(rFifo->semCCount, timeout);
	//Wait until there is something to read or timeout expire
	if (ret==ERROR)
	{
		//errno has been set by semTake
		return -1;
	}
	semTake(rFifo->semMData,WAIT_FOREVER);
	if (rFifo->start == rFifo->stop)
	{
		//fifo empty, nothing to read : exit and cry
		//since we added the semTake(semCount) call, this should NEVER happen
		//if it does something is very wrong
		semGive(rFifo->semMData);
		return -1;
	}//otherwise :
	//Read data
	*data = rFifo->mem[rFifo->start];
	rFifo->start++;
	rFifo->start=(rFifo->start)%(SIZE_FIFO+1);
	semGive(rFifo->semMData);
	return 0;
}
