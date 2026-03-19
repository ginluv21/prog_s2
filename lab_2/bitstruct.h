#ifndef BITSTRUCT_H
#define BITSTRUCT_H

/* 
Б 0–2 - display "3 - LCD, 4 - LED, 5 - OLED, 6 - E-Ink" 3 бита
Б 3–6 - brightness - от 0 до 15 4 бита
Б 7 - time format - "12h, 24h" 1 бит
Б 8 - alarm - "вкл, выкл" 1 бит
Б 9–10 - memory "16KB, 32KB, 64KB, 128KB" 2 бита
Б 11–13 - cpu "ARM, AVR, RISC-V, x86" 3 бита
Б 14–15 - water "Нет, IP67, IP68" 2 бита
*/

#include <stdint.h> // для uint16_t
#include <stdio.h>
#include <stdlib.h>

#define DIS_SHIFT 0 // сдвиг влево на 0
#define BRI_SHIFT 3 
#define TIM_SHIFT 7
#define ALA_SHIFT 8 
#define MEM_SHIFT 9
#define CPU_SHIFT 11 
#define WAT_SHIFT 14 


#define DIS_MASK (0b111 << DIS_SHIFT) //  0000 0000 0000 0111
#define BRI_MASK (0b1111 << BRI_SHIFT) // 0000 0000 0111 1000
#define TIM_MASK (0b1 << TIM_SHIFT)  //   0000 0000 1000 0000
#define ALA_MASK (0b1 << ALA_SHIFT) //    0000 0001 0000 0000
#define MEM_MASK (0b11 << MEM_SHIFT) //   0000 1100 0000 0000
#define CPU_MASK (0b111 << CPU_SHIFT) //  0011 0000 0000 0000
#define WAT_MASK (0b11 << WAT_SHIFT) //   1100 0000 0000 0000

typedef struct { 
    uint16_t data; // поле данных
} device;

void dev_set_display(device* d, uint16_t val); // запись значения
void dev_set_brightness(device* d, uint16_t value);
void dev_set_time_format(device* d, uint16_t value);
void dev_set_alarm(device* d, uint16_t value);
void dev_set_memory(device* d, uint16_t value);
void dev_set_cpu(device* d, uint16_t value);
void dev_set_water(device* d, uint16_t value);

uint16_t dev_get_display(device* d); // чтение значения
uint16_t dev_get_brightness(device* d);
uint16_t dev_get_time_format(device* d);
uint16_t dev_get_alarm(device* d);
uint16_t dev_get_memory(device* d);
uint16_t dev_get_cpu(device* d);
uint16_t dev_get_water(device* d);

void dev_destroy(device* d);
device* dev_create(void);

int get_choice(uint16_t min, uint16_t max);


extern const int display_price[];
extern const int cpu_price[];
extern const int memory_price[];
extern const int water_price[];

extern const char* display_names[]; 
extern const char* cpu_names[];
extern const char* memory_names[];
extern const char* water_names[];


#endif /* BITSTRUCT_H */