#ifndef DATATIME_H
#define DATATIME_H

#include <stdio.h>
#include <stdlib.h>
#include "bitstruct.h"

extern const int month_lengths[12];

typedef struct {
    int day;
    int month;
    int year;
    int hour;
    int minute;

    device* dev;
} datatime;

datatime* datatime_create(int d, int m, int y, int h, int min);
void datatime_destroy(datatime *dt);
void datatime_print(const datatime *dt);
int datatime_compare(const datatime *dt1, const datatime *dt2);
void datatime_min_increment(datatime *dt);
void datatime_hour_increment(datatime *dt);
void datatime_day_increment(datatime *dt);
void datatime_month_increment(datatime *dt);
void datatime_year_set(datatime *dt, int y);
void datatime_month_set(datatime *dt, int m);
void datatime_day_set(datatime *dt, int d);
void datatime_hour_set(datatime *dt, int h);
void datatime_min_set(datatime *dt, int min);
void datatime_min_decrement(datatime *dt);
void datatime_hour_decrement(datatime *dt);
void datatime_day_decrement(datatime *dt);
void datatime_month_decrement(datatime *dt);
void datatime_year_decrement(datatime *dt);
char* datatime_to_string(const datatime *dt);
datatime* datatime_from_string(const char *str);
void datatime_print_diff(const datatime *dt1, const datatime *dt2);
unsigned long long datatime_to_minutes(const datatime *dt);
datatime* datatime_from_minutes(unsigned long long total_minutes);
unsigned long long datatime_diff_in_seconds_from_unix(const datatime *dt);
datatime* create_empty_datatime();
void copy_datatime(datatime *a, const datatime *b);

#endif // DATATIME_H