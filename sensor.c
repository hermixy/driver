#include "driver.h" //for call to isr() and acces to register.
#include <types.h>
#include <taskLib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
int tSensorID = 0;

int sensor()
{
	uint16_t sensors[] = {1,2,3,4,5};
	unsigned int sensorsSize = 5;
	unsigned int i = 0;
	uint16_t value;
	for(;;)
	{
		sleep(1);
		value = (i+2)%sensorsSize;
		i++;
		i = i%sensorsSize;
		memcpy(reg,&(sensors[i]),2);
		memcpy(&reg+2,&value,2);
		isr(); //fake interrupt
	}
}
int startSensor()
{
	if (tSensorID != 0)
	{
		printf("Sensors already started.\n");
		return 0;
	}
	tSensorID = taskSpawn("tSensor",1,0,1000,sensor,0,0,0,0,0,0,0,0,0,0);
	return tSensorID;
}
void stopSensor()
{
	if (tSensorID == 0)
	{
		printf("Sensors are not running.\n");
		return;
	}
	taskDelete(tSensorID);
	tSensorID=0;
}
