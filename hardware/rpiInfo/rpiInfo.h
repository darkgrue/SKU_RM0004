#ifndef __RPIINFO_H
#define __RPIINFO_H

#include <stdint.h>

/**********Select display temperature type**************/
#define CELSIUS 0
#define FAHRENHEIT 1
#define TEMPERATURE_TYPE CELSIUS
/**********Select display temperature type**************/

/************************Control the display of IP address. Can customize the display****************/
#define DISPLAY_IP_ADDR false

// Set empty string to display hostname.
// Maximum of 20 characters for 8x16 font.
#define CUSTOM_DISPLAY ""
/************************Turn off the IP display. Can customize the display****************/

char *GetIPAddress(void);
uint8_t GetMemory(void);
uint8_t GetFSMemoryStatfs(void);
uint8_t GetFSMemoryDf(void);
uint8_t GetCPUTemperature(void);
uint8_t GetCPUUsageTop(void);
uint8_t GetCPUUsagePstat(void);

#endif /*__RPIINFO_H*/