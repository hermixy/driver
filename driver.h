#ifndef __DRIVER_H
#define __DRIVER_H

#define ALREADY_OPEN 2
#define NOT_OPEN 3
#define ALREADY_INSTALLED 4
#define NOT_INSTALLED 5
#define MEM_OVERFLOW 6
#define INSTALL_ERR 7
#define DEV_ADD_ERR 8
#define UNKNOWN_FUNCTION 9
#define UNKNOWN_DEVICE 10
#define REMOVE_ERROR 11
#define DEV_ALREADY_CREATED 12
#define SEM_ERR 13
#define INVALID_ARG 14
#define OPENNED_DEV 15
#define TASK_ERR 16
#define MSGQ_ERR 17

#define SIZE_FIFO 20

/*OS FUNCTIONS FOR IOCTL*/
#define TEST 100

int drvInstall();
int drvRemove();
int devAdd(char * devName,int devNb);
int devDelete(char * devName);

#endif
