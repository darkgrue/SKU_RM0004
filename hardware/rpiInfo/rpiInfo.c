#include "rpiInfo.h"
#include <stdio.h>
#include <math.h>
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
 * @brief Get free RAM.
 *
 * @return Free RAM in percent.
 */
uint8_t GetMemory(void)
{
  struct sysinfo s_info;
  unsigned char buffer[100] = {0};
  unsigned char key[100] = {0};
  unsigned int value = 0;
  uint32_t memTotal = 0;
  uint32_t memFree = 0;

  if (sysinfo(&s_info) == 0) // Get memory information
  {
    FILE *fd = fopen("/proc/meminfo", "r");
    if (fd == NULL)
    {
      fprintf(stderr, "rpiInfo: Unable to open /proc/meminfo file.\n");
      return 0;
    }

    while (fgets(buffer, sizeof(buffer), fd))
    {
      if (sscanf(buffer, "%s%u", key, &value) != 2)
      {
        continue;
      }
      if (strcmp(key, "MemTotal:") == 0)
      {
        memTotal = value;
      }
      else if (strcmp(key, "MemFree:") == 0)
      {
        memFree = value;
      }
    }
    fclose(fd);
  }
  float ramPct = (float)(memTotal - memFree) / memTotal * 100.0;

  /*
  fprintf(stderr, "MemTotal: %lu, MemFree: %lu (%f %%)\n", memTotal, memFree, ramPct);
  */

  return round(ramPct);
}

/**
 * @brief Get mounted filesystem information.
 *
 * @return Used amount of FS storage in percent.
 */
uint8_t GetFSMemoryStatfs(void)
{
  char *mnt_dir = "/";
  struct statfs fs;
  float pctFree = 0;

  if (statfs(mnt_dir, &fs) == 0) // Get mounted filesystem information
  {
    unsigned long long blockSize = fs.f_bsize;                  // The number of bytes per block
    unsigned long long totalSize = blockSize * fs.f_blocks;     // Total data blocks in filesystem
    unsigned long long availableSize = blockSize * fs.f_bavail; // Free blocks available to unprivileged user
    totalSize = totalSize >> 30;
    availableSize = availableSize >> 30;
    pctFree = (float)(totalSize - availableSize) / totalSize * 100.0;

    /*
    fprintf(stderr, "(statfs) f_bsize: %u\n", blockSize);
    fprintf(stderr, "(statfs) Disk Free: %llu GiB, Disk Used: %llu GiB (%f %%), Total: %llu GiB\n", availableSize, totalSize - availableSize, pctFree, totalSize);
    fprintf(stderr, "(statfs) Disk Used: %u %%\n", *fsUsed);
    */
  }
  else
  {
    fprintf(stderr, "rpiInfo: Unable to stat %s filesystem.\n", mnt_dir);
    return 0;
  }

  return round(pctFree);
}

/**
 * @brief Get mounted filesystem information using df/awk (better portability).
 *
 * @return Used amount of FS storage in percent.
 */
uint8_t GetFSMemoryDf(void)
{
  char *mnt_dir = "/";
  FILE *fd;
  char buffer[32] = {0};
  uint32_t totalSize = 0;
  uint32_t usedSize = 0;
  uint32_t availableSize = 0;
  uint8_t pctFree = 0;

  fd = popen("df -k / | awk '{if (NR==2) {print $2\" \"$3\" \"$4\" \"$5+0}}'", "r");
  if (fd == NULL)
  {
    fprintf(stderr, "rpiInfo: Unable to stat %s filesystem.\n", mnt_dir);
    return 0;
  }

  fgets(buffer, sizeof(buffer), fd);

  pclose(fd);

  // Parse buffer
  sscanf(buffer, "%lu %lu %lu %u", &totalSize, &usedSize, &availableSize, &pctFree);

  /*
  fprintf(stderr, "(df) Disk Free: %lu B, Disk Used: %lu B (%u %%), Total: %lu B\n", availableSize, usedSize, pctFree, totalSize);
  */

  return pctFree;
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
  uint16_t temp;

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

  return (TEMPERATURE_TYPE == FAHRENHEIT) ? temp / 1000 * 1.8 + 32 : temp / 1000;
}

/**
 * @brief Get CPU usage using top/grep/awk.
 *
 * @return CPU utilization in percent.
 */
uint8_t GetCPUUsageTop(void)
{
  FILE *fd;
  char buffer[16] = {0};

  fd = popen("top -b -n2 -d1 | grep -m 1 'Cpu(s)' | awk '{print $2 + $4 + $6 + $13 + $15}'", "r");
  if (fd == NULL)
  {
    fprintf(stderr, "rpiInfo: Unable to query top for CPU usage.\n");
    return 0;
  }

  fgets(buffer, sizeof(buffer), fd);

  pclose(fd);

  /*
  fprintf(stderr, "top CPU %f%%\n", atof(buffer));
  */

  return round(atof(buffer));
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
    fprintf(stderr, "rpiInfo: Unable to open /proc/stat pseudofile.\n");
    return 0;
  }

  fscanf(fd, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
         &prev.t_user, &prev.t_nice, &prev.t_system, &prev.t_idle, &prev.t_iowait, &prev.t_irq, &prev.t_softirq, &prev.t_steal, &prev.t_guest, &prev.t_guestnice);

  fclose(fd);

  unsigned long long int prev_total = prev.t_user + prev.t_nice + prev.t_system + prev.t_idle + prev.t_iowait + prev.t_irq + prev.t_softirq + prev.t_steal;
  unsigned long long int prev_util = prev.t_user + prev.t_nice + prev.t_system + prev.t_irq + prev.t_softirq;

  // Wait for one second to collect data to calculate delta
  sleep(1);

  fd = fopen("/proc/stat", "r");
  if (fd == NULL)
  {
    fprintf(stderr, "rpiInfo: Unable to open /proc/stat pseudofile.\n");
    return 0;
  }

  fscanf(fd, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
         &cur.t_user, &cur.t_nice, &cur.t_system, &cur.t_idle, &cur.t_iowait, &cur.t_irq, &cur.t_softirq, &cur.t_steal, &cur.t_guest, &cur.t_guestnice);

  fclose(fd);

  unsigned long long int cur_total = cur.t_user + cur.t_nice + cur.t_system + cur.t_idle + cur.t_iowait + cur.t_irq + cur.t_softirq + cur.t_steal;
  unsigned long long int cur_util = cur.t_user + cur.t_nice + cur.t_system + cur.t_irq + cur.t_softirq;
  unsigned long int total_d = cur_total - prev_total;

  float cpuPct = (float)(cur_util - prev_util) / total_d * 100.0;

  /*
  fprintf(stderr, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n", prev.t_user, prev.t_nice, prev.t_system, prev.t_idle, prev.t_iowait, prev.t_irq, prev.t_softirq, prev.t_steal, prev.t_guest, prev.t_guestnice);

  fprintf(stderr, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n", cur.t_user, cur.t_nice, cur.t_system, cur.t_idle, cur.t_iowait, cur.t_irq, cur.t_softirq, cur.t_steal, cur.t_guest, cur.t_guestnice);

  fprintf(stderr, "%lu us, %lu ni, %lu sy, %lu hi %lu si = %lu\n",
          (cur.t_user - prev.t_user),
          (cur.t_nice - prev.t_nice),
          (cur.t_system - prev.t_system),
          (cur.t_irq - prev.t_irq),
          (cur.t_softirq - prev.t_softirq),
          total_d);

  fprintf(stderr, "%f us, %f ni, %f sy, %f hi %f si\n",
          (float)(cur.t_user - prev.t_user) / total_d * 100.0,
          (float)(cur.t_nice - prev.t_nice) / total_d * 100.0,
          (float)(cur.t_system - prev.t_system) / total_d * 100.0,
          (float)(cur.t_irq - prev.t_irq) / total_d * 100.0,
          (float)(cur.t_softirq - prev.t_softirq) / total_d * 100.0);

  fprintf(stderr, "%f %%\n", (float)(cur_util - prev_util) / total_d * 100.0);

  fprintf(stderr, "pstat CPU %f%%\n", cpuPct);
  */

  return round(cpuPct);
}
