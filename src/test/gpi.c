#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <time.h>
#include "beaglebone_gpio.h"

#define __TIMELINE_NANOSLEEP    388
#define __SET_OFFSET            389
#define __PRINT_TIMELINE        390
#define TIMELINE_ID_SIZE 32

#define NSEC_PER_SEC 1000000000

struct timespec ns_to_timespec(long long nsec)
{
    struct timespec ts;
    if (!nsec)
        return (struct timespec) {0, 0};
    ts.tv_nsec = (long) nsec % NSEC_PER_SEC;
    ts.tv_sec = nsec / NSEC_PER_SEC;
    return ts;
}

int main(int argc, char *argv[]) {
    volatile void *gpio_addr = NULL;
    volatile unsigned int *gpio_oe_addr = NULL;
    volatile unsigned int *gpio_setdataout_addr = NULL;
    volatile unsigned int *gpio_cleardataout_addr = NULL;

    struct timespec t_now, t_next;
    unsigned long t, count, t_nsec;
    char tlid[TIMELINE_ID_SIZE] = "local_time";
    unsigned int reg;
    if(argc > 1) {
        if(sscanf(argv[1], "%lu", &t) != 1 || t<0) t = 2;              // no non-zero values allowed
        if(sscanf(argv[2], "%lu", &t_nsec) != 1 || (t<=0 && t_nsec <= 0)) t_nsec = 0;              // no non-zero values allowed
        
    } else t = 2;
    count = 0;

    printf("[sleeper] PID: %d sleeping for %ld secs and %ld nsec on timeline \"%s\"\n", getpid(), t, t_nsec, tlid);

    int fd = open("/dev/mem", O_RDWR);

    printf("Mapping %X - %X (size: %X)\n", GPIO1_START_ADDR,  GPIO1_END_ADDR, GPIO1_SIZE);

    gpio_addr = mmap(0, GPIO1_SIZE, PROT_READ | PROT_WRITE,  MAP_SHARED, fd, GPIO1_START_ADDR);

    gpio_oe_addr = gpio_addr + GPIO_OE;
    gpio_setdataout_addr = gpio_addr + GPIO_SETDATAOUT;
    gpio_cleardataout_addr = gpio_addr + GPIO_CLEARDATAOUT;

    if(gpio_addr == MAP_FAILED) {
        printf("Unable to map GPIO\n");
        exit(1);
    }
    printf("GPIO mapped to %p\n", gpio_addr);
    printf("GPIO OE mapped to %p\n", gpio_oe_addr);
    printf("GPIO SETDATAOUTADDR mapped to %p\n", gpio_setdataout_addr);
    printf("GPIO CLEARDATAOUT mapped to %p\n", gpio_cleardataout_addr);

    reg = *gpio_oe_addr;
    printf("GPIO1 configuration: %X\n", reg);
    reg = reg & (0xFFFFFFFF - PIN);
    *gpio_oe_addr = reg;
    printf("GPIO1 configuration: %X\n", reg);

    printf("Start toggling PIN \n");
    while(1) {
        
        *gpio_setdataout_addr= PIN;
        clock_gettime(CLOCK_REALTIME, &t_now);      // get current time
        t_next.tv_sec = t_now.tv_sec + t;           // start at next sec
        t_next.tv_nsec = 0 + t_nsec;
        count++;
        //printf("[sleeper] (%lu) CLOCK_REALTIME, current start: %ld.%09lu, next start: %ld.%09lu\n", count, t_now.tv_sec, t_now.tv_nsec, t_next.tv_sec, t_next.tv_nsec);
        if(syscall(__TIMELINE_NANOSLEEP, tlid, &t_next) == -52) {
            printf("[sleeper] missed deadline by > 1msec\n");
        }
        *gpio_cleardataout_addr = PIN;
        clock_gettime(CLOCK_REALTIME, &t_now);      // get current time
        t_next.tv_sec = t_now.tv_sec + t;           // start at next sec
        t_next.tv_nsec = 0 + t_nsec;
        count++;
        //printf("[sleeper] (%lu) CLOCK_REALTIME, current start: %ld.%09lu, next start: %ld.%09lu\n", count, t_now.tv_sec, t_now.tv_nsec, t_next.tv_sec, t_next.tv_nsec);
        if(syscall(__TIMELINE_NANOSLEEP, tlid, &t_next) == -52) {
            printf("[sleeper] missed deadline by > 1msec\n");
        }
      }

    close(fd);
    return 0;
}
