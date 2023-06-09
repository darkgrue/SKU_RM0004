#include "rpiInfo.h"
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include "st7735.h"
#include <stdlib.h>

/**
 * @brief Get the IP address of the default interface.
 *
 * @return Pointer to char of IP address in dotted-quad notation.
 */
char *GetIPAddress(void)
{
  FILE *fd;
  char line[100], *p, *c;

  fd = fopen("/proc/net/route", "r");
  if (fd == NULL)
  {
    fprintf(stderr, "rpiInfo: Unable to open /proc/net/route pseudofile.\n");
    return '\0';
  }

  while (fgets(line, 100, fd))
  {
    p = strtok(line, " \t");
    c = strtok(NULL, " \t");

    if (p != NULL && c != NULL)
    {
      if (strcmp(c, "00000000") == 0)
      {
        break;
      }
    }
  }

  fclose(fd);

  // Default to eth0 interface
  if (p == NULL)
  {
    fprintf(stderr, "rpiInfo: Unable to determine default interface. Defaulting to eth0.\n");
    p = "eth0";
  }

  int sockfd;
  struct ifreq ifr;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  // Type of address to retrieve - IPv4 IP address
  ifr.ifr_addr.sa_family = AF_INET;

  // Copy the interface name in the ifreq structure
  strncpy(ifr.ifr_name, p, IFNAMSIZ - 1);

  ioctl(sockfd, SIOCGIFADDR, &ifr);

  close(sockfd);

  return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}

/**
 * @brief Get total and free RAM.
 *
 * @param MemTotal Total amount of RAM.
 * @param MemFree Free amount of RAM.
 * @return void
 */
void GetMemory(float *MemTotal, float *MemFree)
{
  struct sysinfo s_info;

  unsigned int value = 0;
  unsigned char buffer[100] = {0};
  unsigned char famer[100] = {0};

  if (sysinfo(&s_info) == 0) // Get memory information
  {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fd == NULL)
    {
      fprintf(stderr, "rpiInfo: Unable to open /proc/meminfo file.\n");
      return 0;
    }

    while (fgets(buffer, sizeof(buffer), fp))
    {
      if (sscanf(buffer, "%s%u", famer, &value) != 2)
      {
        continue;
      }
      if (strcmp(famer, "MemTotal:") == 0)
      {
        *MemTotal = value / 1000.0 / 1000.0;
      }
      else if (strcmp(famer, "MemFree:") == 0)
      {
        *MemFree = value / 1000.0 / 1000.0;
      }
    }
    fclose(fp);
  }
}

/**
 * @brief Get mounted filesystem information.
 *
 * @param MemSize Total amount of FS storage.
 * @param freesize Free amount of FS storage.
 * @return void
 */
void GetFSMemoryStatfs(uint32_t *MemSize, uint32_t *freesize)
{
  char *mnt_dir = "/";
  struct statfs fs;

  if (statfs(mnt_dir, &fs) == 0) // Get mounted filesystem information
  {
    unsigned long long blocksize = fs.f_bsize;       // The number of bytes per block
    unsigned long long totalsize = fs * fs.f_blocks; // Total number of bytes
    *MemSize = (unsigned int)(totalsize >> 30);

    unsigned long long size = blocksize * fs.f_bfree; // Now let's figure out how much space we have left
    *freesize = size >> 30;
    *freesize = *MemSize - *freesize;

    fprintf(stderr, "f_bsize: %u\n", fs.blocksize);
    fprintf(stderr, "Disk Free: %f GiB\tTotal: %f GiB\n", freesize, MemSize);
  }
  else
  {
    fprintf(stderr, "rpiInfo: Unable to stat %s filesystem.\n", mnt_dir);
  }
}

/**
 * @brief Get mounted filesystem information using df/awk (better portability).
 *
 * @param MemSize Total amount of FS storage in gibibytes.
 * @param freesize Free amount of FS storage in gibibytes.
 * @return void
 */
void GetFSMemoryDf(uint32_t *MemSize, uint32_t *freesize)
{
  char *mnt_dir = "/";
  FILE *fp;
  char buffer[32] = {0};
  float usedsize = 0;
  float totalsize = 0;

  fp = popen("df -k / | awk '{if (NR==2) {print $3 / 1048576\" \"$2 / 1048576}}'", "r");
  if (fp == NULL)
  {
    fprintf(stderr, "rpiInfo: Unable to stat %s filesystem.\n", mnt_dir);
    return;
  }

  fgets(buffer, sizeof(buffer), fp);

  pclose(fp);

  // Parse buffer
  sscanf(buffer, "%f %f", &usedsize, &totalsize);
  *MemSize = atof(totalsize);
  *freesize = *MemSize - atof(usedsize);

  fprintf(stderr, "Disk Free: %f\tTotal: %f\n", usedsize, totalsize);
  fprintf(stderr, "Disk Free: %f GiB\tTotal: %f GiB\n", freesize, MemSize);
}

/**
 * @brief Get the CPU temperature.
 *
 * @return CPU temperature in Celsius or Fahrenheit (according to TEMPERATURE_TYPE setting).
 */
uint8_t GetCPUTemperature(void)
{
  FILE *fd;
  char buffer[150] = {0};
  unsigned int temp;

  fd = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
  if (fd == NULL)
  {
    fprintf(stderr, "rpiInfo: Unable to open /sys/class/thermal/thermal_zone0/temp pseudofile.\n");
    return 0;
  }

  fgets(buffer, sizeof(buffer), fd);

  fclose(fd);

  // Parse buffer
  sscanf(buffer, "%u", &temp);

  return atoi((TEMPERATURE_TYPE == FAHRENHEIT) ? temp / 1000 * 1.8 + 32 : temp / 1000);
}

/**
 * @brief Get CPU usage using top/grep/awk.
 *
 * @return CPU utilization in percent.
 */
uint8_t GetCPUUsageTop(void)
{
  FILE *fp;
  char buffer[16] = {0};

  fp = popen("env CPULOOP=1 top -bn 1 | grep -m 1 'Cpu(s)' | awk '{print 100 - $8}'", "r");
  if (fp == NULL)
  {
    fprintf(stderr, "rpiInfo: Unable to query top for CPU usage.\n");
    return 0;
  }

  fgets(buffer, sizeof(buffer), fp);

  pclose(fp);

  ///*
    fprintf(stderr, "top CPU %f%%\n", atof(buffer));
  //*/

  return atoi(buffer);
}

struct cpustat
{
  unsigned long int t_user;
  unsigned long int t_nice;
  unsigned long int t_system;
  unsigned long int t_idle;
  unsigned long int t_iowait;
  unsigned long int t_irq;
  unsigned long int t_softirq;
  unsigned long int t_steal;
  unsigned long int t_guest;
  unsigned long int t_guestnice;
};

/**
 * @brief Get CPU usage using pstat.
 *
 * @return Float of CPU utilization in percent.
 */
uint8_t GetCPUUsagePstat(void)
{
  FILE *fd;
  struct cpustat prev, cur;

  fd = fopen("/proc/stat", "r");
  if (fd == NULL)
  {
    fprintf(stderr, "rpiInfo: Unable to open /proc/stat file.\n");
    return 0;
  }

  fscanf(fd, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
         &prev.t_user, &prev.t_nice, &prev.t_system, &prev.t_idle, &prev.t_iowait, &prev.t_irq, &prev.t_softirq, &prev.t_steal, &prev.t_guest, &prev.t_guestnice);

  fclose(fd);

  unsigned long long int prev_total = prev.t_idle + prev.t_iowait + prev.t_user + prev.t_nice + prev.t_system + prev.t_irq + prev.t_softirq + prev.t_steal;

  /*
    fprintf(stderr, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n", prev.t_user, prev.t_nice, prev.t_system, prev.t_idle, prev.t_iowait, prev.t_irq, prev.t_softirq, prev.t_steal, prev.t_guest, prev.t_guestnice);
    unsigned long long int prev_idle = prev.t_idle + prev.t_iowait;
    unsigned long long int prev_nonidle = prev.t_user + prev.t_nice + prev.t_system + prev.t_irq + prev.t_softirq + prev.t_steal;
    //unsigned long long int prev_total = prev_idle + prev_nonidle;
  */

  // Wait for one second to collect data to calculate delta
  sleep(3);

  fd = fopen("/proc/stat", "r");
  if (fd == NULL)
  {
    fprintf(stderr, "rpiInfo: Unable to open /proc/stat peudofile.\n");
    return 0;
  }

  fscanf(fd, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
         &cur.t_user, &cur.t_nice, &cur.t_system, &cur.t_idle, &cur.t_iowait, &cur.t_irq, &cur.t_softirq, &cur.t_steal, &cur.t_guest, &cur.t_guestnice);

  fclose(fd);

  unsigned long long int cur_total = cur.t_idle + cur.t_iowait + cur.t_user + cur.t_nice + cur.t_system + cur.t_irq + cur.t_softirq + cur.t_steal;

  /*
    fprintf(stderr, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n", cur.t_user, cur.t_nice, cur.t_system, cur.t_idle, cur.t_iowait, cur.t_irq, cur.t_softirq, cur.t_steal, cur.t_guest, cur.t_guestnice);
    unsigned long long int cur_idle = cur.t_idle + cur.t_iowait;
    unsigned long long int cur_nonidle = cur.t_user + cur.t_nice + cur.t_system + cur.t_irq + cur.t_softirq + cur.t_steal;
    //unsigned long long int cur_total = cur_idle + cur_nonidle;

    fprintf(stderr, "PREV: idle:%llu\tnonidle: %llu\ttotal: %llu\n", prev_idle, prev_nonidle, prev_total);
    fprintf(stderr, "CUR: idle:%llu\tnonidle: %llu\ttotal: %llu\n", cur_idle, cur_nonidle, cur_total);

    // Calculate delta values
    unsigned long int total_d = cur_total - prev_total;
    unsigned long int idle_d = cur_idle - prev_idle;
    unsigned long int used_d = total_d - idle_d;
    //float cpu_perc = (float)used_d / (float)total_d * 100.0;
    fprintf(stderr, "TOTAL prev: %llu\tcurr: %llu\tdelta: %lu\n", prev_total, cur_total, total_d);
    fprintf(stderr, "IDLE  prev: %llu\tcurr: %llu\tdelta: %lu\n", prev_idle, cur_idle, idle_d);
    fprintf(stderr, "USED  delta: %lu\n", used_d);
  */

  float cpu_perc = (float)(cur_total - prev_total - cur.t_idle - cur.t_iowait + prev.t_idle + prev.t_iowait) / (float)(cur_total - prev_total) * 100.0;

  ///*
    fprintf(stderr, "pstat CPU %f%%\n", cpu_perc);
  //*/

  return atoi(cpu_perc);
}
