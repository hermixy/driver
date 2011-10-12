#ifndef __DRIVER_H
#define __DRIVER_H

#define ALREADY_OPEN 2
#define NOT_OPEN 3
#define ALREADY_INSTALLED 4
#define NOT_INSTALLED 5
#define NOT_ENOUGH_MEMORY 6
#define INSTALL_ERR 7
#define DEV_ADD_ERR 8
#define UNKNOWN_FUNCTION 9
#define UNKNOWN_DEVICE 10
#define REMOVE_ERROR 11

#define SIZE_FIFO 10

/*OS FUNCTIONS FOR IOCTL*/
#define TEST 100
#define ZARB 101


int drvInstall();
int drvRemove();
int devAdd(char * devName,int devNb);
int devDelete(char * devName);

#endif
