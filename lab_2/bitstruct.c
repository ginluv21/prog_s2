#include "bitstruct.h"


const int display_price[] = {1000, 1500, 3000, 2500};
const int cpu_price[] = {1000, 1500, 3000, 2500};
const int memory_price[] = {1000, 1500, 3000, 2500};
const int water_price[] = {1000, 1500, 3000, 2500};

const char* display_names[] = {"LCD", "LED", "OLED", "E-Ink"};
const char* cpu_names[] = {"ARM", "AVR", "M-16pro", "x86"};
const char* memory_names[] = {"16KB", "32KB", "64KB", "128KB"};
const char* water_names[] = {"Нет", "IP67", "IP68"};

device* dev_create (void) {
    device* d = malloc(sizeof(device));
    if (d == NULL) {
        return NULL;
    }
    d->data = 0;
    return d;
}

void dev_destroy (device* d) {
    free(d);
    //d = NULL;
}

void dev_set_display (device* d, uint16_t val) {
    if( d == NULL) return;
    if(val > 7) return; // нельзя больше 7

    d->data = (d->data & ~DIS_MASK) | (val << DIS_SHIFT); // запись значения и сдвиг
}

void dev_set_brightness(device* d, uint16_t value) {
    if (d == NULL) return;
    if (value > 15) return;

    d->data = (d->data & ~BRI_MASK) | (value << BRI_SHIFT);
}

void dev_set_time_format(device* d, uint16_t value) {
    if (d == NULL) return;
    if (value > 1) return;

    d->data = (d->data & ~TIM_MASK) | (value << TIM_SHIFT);
}

void dev_set_alarm(device* d, uint16_t value) {
    if (d == NULL) return;
    if (value > 1) return;

    d->data = (d->data & ~ALA_MASK) | (value << ALA_SHIFT);
}

void dev_set_memory(device* d, uint16_t value) {
    if (d == NULL) return;
    if (value > 3) return;

    d->data = (d->data & ~MEM_MASK) | (value << MEM_SHIFT);
} 

void dev_set_cpu(device* d, uint16_t value) {
    if (d == NULL) return;
    if (value > 7) return; 

    d->data = (d->data & ~CPU_MASK) | (value << CPU_SHIFT);
}

void dev_set_water(device* d, uint16_t value) {
    if (d == NULL) return;
    if (value > 3) return;

    d->data = (d->data & ~WAT_MASK) | (value << WAT_SHIFT); 
}

uint16_t dev_get_display (device* d) {
    if (d == NULL) return 0;

    return (d->data & DIS_MASK) >> DIS_SHIFT; // чтение значения и сдвиг
}

uint16_t dev_get_brightness (device* d) {
    if (d == NULL) return 0;

    return (d->data & BRI_MASK) >> BRI_SHIFT;
}

uint16_t dev_get_time_format (device* d) {
    if (d == NULL) return 0;

    return (d->data & TIM_MASK) >> TIM_SHIFT;
}

uint16_t dev_get_alarm (device* d) {
    if (d == NULL) return 0;

    return (d->data & ALA_MASK) >> ALA_SHIFT;
}

uint16_t dev_get_memory (device* d) {
    if (d == NULL) return 0;    

    return (d->data & MEM_MASK) >> MEM_SHIFT;
}

uint16_t dev_get_cpu (device* d) {
    if (d == NULL) return 0;

    return (d->data & CPU_MASK) >> CPU_SHIFT;
}

uint16_t dev_get_water (device* d) {
    if (d == NULL) return 0;

    return (d->data & WAT_MASK) >> WAT_SHIFT;
}

int get_choice(uint16_t min, uint16_t max) {
    uint16_t val;
    int res;

    while (1) {
        res = scanf("%hu", &val);

        if (res != 1) {
            while (getchar() != '\n'); // очистка ввода
            printf("Ошибка ввода. Попробуйте снова: ");
            continue;
        }

        if (val < min || val > max) {
            printf("Недопустимое значение. Попробуйте снова: ");
            continue;
        }

        return val;
    }
}
