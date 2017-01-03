#ifndef _BEAGLEBONE_GPIO_H_
#define _BEAGLEBONE_GPIO_H_

// GPIO 1 Beaglebone Parameters
#define GPIO1_START_ADDR 0x4804C000
#define GPIO1_END_ADDR 0x4804DFFF
#define GPIO1_SIZE (GPIO1_END_ADDR - GPIO1_START_ADDR)
#define GPIO_OE 0x134
#define GPIO_SETDATAOUT 0x194
#define GPIO_CLEARDATAOUT 0x190


#define PIN19 (1<<19) 
#define PIN28 (1<<28)
#define PIN (1<<28)

// Definitions for Writing out an 8-Bit Word
#define PIN_0 (1<<12)
#define PIN_1 (1<<13)
#define PIN_2 (1<<14)
#define PIN_3 (1<<15)
#define PIN_4 (1<<16)
#define PIN_5 (1<<17)
#define PIN_6 (1<<19)
#define PIN_7 (1<<28)

#endif
