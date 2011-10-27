#ifndef __LIBFIFOH__
#define __LIBFIFOH__

#include <types.h>
#include <semLib.h>
#define SIZE_FIFO 20
typedef struct fifo
{
	int start;
	int stop;
	uint16_t mem[SIZE_FIFO+1];
	int initialized;
	SEM_ID semCCount; //Can be used to keep a count of actual elements present in fifo
	SEM_ID semMData;
	struct fifo * nextFifo;
	int taskId;
} fifo;
int initFifo(fifo * iFifo);
void deleteFifo(fifo * dFifo);
int writeFifo(fifo * wFifo, uint16_t data );
int readFifo(fifo * rFifo, uint16_t * data );
#endif
