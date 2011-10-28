#ifndef __LIBFIFOH__
#define __LIBFIFOH__

#include <types.h>
#include <semLib.h>
#define SIZE_FIFO 20
typedef struct msg
{
	unsigned int mdata;
	unsigned int mnb;
	unsigned long int mtimestamp;
}msg;
typedef struct fifo
{
	unsigned int start;
	unsigned int stop;
	unsigned int count;
	msg mem[SIZE_FIFO+1];
	int initialized;
	SEM_ID semCCount; //Can be used to keep a count of actual elements present in fifo
	SEM_ID semMData;
	struct fifo * nextFifo;
	int taskId;
} fifo;
int initFifo(fifo * iFifo);
void deleteFifo(fifo * dFifo);
int writeFifo(fifo * wFifo, msg data );
int readFifo(fifo * rFifo, msg * data, int timeout );
#endif
