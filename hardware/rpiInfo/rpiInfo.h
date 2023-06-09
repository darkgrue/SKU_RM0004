#ifndef __RPIINFO_H
#define __RPIINFO_H

#include <stdint.h>

/**********Select display temperature type**************/
#define CELSIUS 0
#define FAHRENHEIT 1
#define TEMPERATURE_TYPE CELSIUS
/**********Select display temperature type**************/

/************************Control the display of IP address. Can customize the display****************/
#define DISPLAY_IP_ADDR true

#define CUSTOM_DISPLAY "UCTRONICS"
/************************Turn off the IP display. Can customize the display****************/

char *GetIPAddress(void);
void GetMemory(float *MemTotal, float *MemFree);
void GetFSMemoryStatfs(uint32_t *MemSize, uint32_t *freesize);
void GetFSMemoryDf(uint32_t *MemSize, uint32_t *freesize);
float GetCPUTemperature(void);
uint8_t GetCPUUsageTop(void);
uint8_t GetCPUUsagePstat(void);

#endif /*__RPIINFO_H*/