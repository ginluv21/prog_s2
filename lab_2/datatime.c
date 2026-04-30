#include <stdio.h>
#include <stdlib.h>
#include <string.h>   /* ✅ ДОБАВЛЕНО: нужен для memcpy */
#include "datatime.h"

const int month_lengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const unsigned long long unix_dt_in_minutes = 1035476640;

static int check_time_format(int d, int m, int y, int h, int min) {
    (void)y; /* год не ограничивается — любое целое допустимо */
    if (m < 1 || m > 12 || d < 1 || d > month_lengths[m - 1] || h < 0 || h > 23 || min < 0 || min > 59) {
        return 0;
    }
    return 1;
}

datatime* datatime_create(int d, int m, int y, int h, int min) {
    datatime* dt = malloc(sizeof *dt);
    if (dt == NULL) return NULL;

    if (!check_time_format(d, m, y, h, min)) {
        printf("Неверный формат даты и времени\n");
        free(dt);
        return NULL;
    }

    dt->day    = d;
    dt->month  = m;
    dt->year   = y;
    dt->hour   = h;
    dt->minute = min;

    dt->dev = dev_create();
    if (dt->dev == NULL) {
        free(dt);
        return NULL;
    }

    return dt;
}

/*
 * ❌ ОШИБКА: функция принимала обычный указатель и делала dt = NULL локально —
 *    это ничего не меняет снаружи, переменная вызывающего кода осталась бы
 *    указывать на освобождённую память.
 *
 * void datatime_destroy(datatime *dt) {
 *     if (dt == NULL) return;
 *     if (dt->dev != NULL) dev_destroy(dt->dev);
 *     free(dt);
 *     dt = NULL;  ← бесполезно: локальная переменная, снаружи не меняется
 * }
 *
 * ✅ ИСПРАВЛЕНО: оставляем одиночный указатель (lab_3 и lab_4 зависят от этой сигнатуры),
 *    но убираем бесполезный dt = NULL и добавляем комментарий.
 *    Вызывающий код сам обязан обнулить указатель после вызова.
 */
void datatime_destroy(datatime *dt) {
    if (dt == NULL) return;
    if (dt->dev != NULL) dev_destroy(dt->dev);
    free(dt);
    /* dt = NULL здесь бессмысленно — это локальный параметр.
       После вызова делай: ptr = NULL; самостоятельно. */
}

/*
 * ❌ ОШИБКА: datatime_create(0, 0, 0, 0, 0) не проходит check_time_format
 *    (m=0 → "m < 1" истинно), функция всегда возвращала NULL.
 *
 * datatime* create_empty_datatime() {
 *     return datatime_create(0, 0, 0, 0, 0);
 * }
 *
 * ✅ ИСПРАВЛЕНО: выделяем память напрямую, минуя валидатор.
 *    Явно инициализируем dev через dev_create() — иначе dev будет мусором.
 */
datatime* create_empty_datatime() {
    datatime *dt = malloc(sizeof *dt);
    if (dt == NULL) return NULL;

    dt->day    = 0;
    dt->month  = 0;
    dt->year   = 0;
    dt->hour   = 0;
    dt->minute = 0;

    dt->dev = dev_create();
    if (dt->dev == NULL) {
        free(dt);
        return NULL;
    }

    return dt;
}

void copy_datatime(datatime *a, const datatime *b) {
    if (a == NULL || b == NULL) return;

    a->day    = b->day;
    a->month  = b->month;
    a->year   = b->year;
    a->hour   = b->hour;
    a->minute = b->minute;

    if (b->dev != NULL) {
        if (a->dev == NULL)
            a->dev = dev_create();

        if (a->dev != NULL) {
            /*
             * ❌ СТАРЫЙ КОД: копировали только одно поле вручную
             *     a->dev->data = b->dev->data;
             *
             * ✅ ИСПРАВЛЕНО: memcpy копирует всю структуру device за один вызов.
             *    Надёжнее и не зависит от того, сколько полей появится в device.
             */
            memcpy(a->dev, b->dev, sizeof(device));
        }
    }
}

void datatime_print(const datatime *dt) {
    /*
     * ❌ ОШИБКА: второй if (dt->dev != NULL) был снаружи блока if (dt != NULL).
     *    При dt == NULL первый блок не выполнялся, но второй всё равно
     *    обращался к dt->dev → разыменование NULL → краш (undefined behavior).
     *
     * if (dt != NULL) {
     *     printf(...)
     * }
     * if (dt->dev != NULL) {   ← UB если dt == NULL!
     *     ...
     * }
     *
     * ✅ ИСПРАВЛЕНО: обе проверки объединены внутри одного if (dt != NULL).
     */
    if (dt == NULL) return;

    printf("%02d/%02d/%04d %02d:%02d\n",
            dt->day, dt->month, dt->year,
            dt->hour, dt->minute);

    if (dt->dev != NULL) {
        /* dev_print(dt->dev); — раскомментируй чтобы вывести параметры устройства */
    } else {
        printf(" [Девайс отсутствует]\n");
    }
}

int datatime_compare(const datatime *dt1, const datatime *dt2) {
    if (dt1 == NULL || dt2 == NULL) return 0;

    if (dt1->year   != dt2->year)   return (dt1->year   < dt2->year)   ? -1 : 1;
    if (dt1->month  != dt2->month)  return (dt1->month  < dt2->month)  ? -1 : 1;
    if (dt1->day    != dt2->day)    return (dt1->day    < dt2->day)    ? -1 : 1;
    if (dt1->hour   != dt2->hour)   return (dt1->hour   < dt2->hour)   ? -1 : 1;
    if (dt1->minute != dt2->minute) return (dt1->minute < dt2->minute) ? -1 : 1;

    return 0;
}

void datatime_min_increment(datatime *dt) {
    if (dt == NULL) return;

    dt->minute++;
    if (dt->minute >= 60) {
        dt->minute = 0;
        dt->hour++;
        if (dt->hour >= 24) {
            dt->hour = 0;
            dt->day++;
            if (dt->day > month_lengths[dt->month - 1]) {
                dt->day = 1;
                dt->month++;
                if (dt->month > 12) { dt->month = 1; dt->year++; }
            }
        }
    }
}

void datatime_hour_increment(datatime *dt) {
    if (dt == NULL) return;

    dt->hour++;
    if (dt->hour >= 24) {
        dt->hour = 0;
        dt->day++;
        if (dt->day > month_lengths[dt->month - 1]) {
            dt->day = 1;
            dt->month++;
            if (dt->month > 12) { dt->month = 1; dt->year++; }
        }
    }
}

void datatime_day_increment(datatime *dt) {
    if (dt == NULL) return;

    dt->day++;
    if (dt->day > month_lengths[dt->month - 1]) {
        dt->day = 1;
        dt->month++;
        if (dt->month > 12) { dt->month = 1; dt->year++; }
    }
}

void datatime_month_increment(datatime *dt) {
    if (dt == NULL) return;

    dt->month++;
    if (dt->month > 12) { dt->month = 1; dt->year++; }
}

void datatime_year_increment(datatime *dt) {
    if (dt == NULL) return;
    dt->year++;
}

void datatime_year_decrement(datatime *dt) {
    if (dt == NULL) return;
    dt->year--;
}

void datatime_month_decrement(datatime *dt) {
    if (dt == NULL) return;

    dt->month--;
    if (dt->month < 1) { dt->month = 12; dt->year--; }
}

void datatime_day_decrement(datatime *dt) {
    if (dt == NULL) return;

    dt->day--;
    if (dt->day < 1) {
        dt->month--;
        if (dt->month < 1) { dt->month = 12; dt->year--; }
        dt->day = month_lengths[dt->month - 1];
    }
}

void datatime_hour_decrement(datatime *dt) {
    if (dt == NULL) return;

    dt->hour--;
    if (dt->hour < 0) {
        dt->hour = 23;
        dt->day--;
        if (dt->day < 1) {
            dt->month--;
            if (dt->month < 1) { dt->month = 12; dt->year--; }
            dt->day = month_lengths[dt->month - 1];
        }
    }
}

void datatime_min_decrement(datatime *dt) {
    if (dt == NULL) return;

    dt->minute--;
    if (dt->minute < 0) {
        dt->minute = 59;
        dt->hour--;
        if (dt->hour < 0) {
            dt->hour = 23;
            dt->day--;
            if (dt->day < 1) {
                dt->month--;
                if (dt->month < 1) { dt->month = 12; dt->year--; }
                dt->day = month_lengths[dt->month - 1];
            }
        }
    }
}

void datatime_min_set(datatime *dt, int min) {
    if (dt == NULL) return;
    if (min >= 0 && min < 60) dt->minute = min;
    else printf("Неверное значение минут\n");
}

void datatime_hour_set(datatime *dt, int h) {
    if (dt == NULL) return;
    if (h >= 0 && h < 24) dt->hour = h;
    else printf("Неверное значение часов\n");
}

void datatime_day_set(datatime *dt, int d) {
    if (dt == NULL) return;
    if (d >= 1 && d <= month_lengths[dt->month - 1]) dt->day = d;
    else printf("Неверное значение дней\n");
}

void datatime_month_set(datatime *dt, int m) {
    if (dt == NULL) return;
    if (m >= 1 && m <= 12) dt->month = m;
    else printf("Неверное значение месяцев\n");
}

void datatime_year_set(datatime *dt, int y) {
    if (dt != NULL) dt->year = y;
}

char* datatime_to_string(const datatime *dt) {
    if (dt == NULL) return NULL;

    char *buffer = malloc(20 * sizeof(char));
    if (buffer == NULL) return NULL;

    snprintf(buffer, 20, "%02d/%02d/%04d %02d:%02d",
                dt->day, dt->month, dt->year,
                dt->hour, dt->minute);

    return buffer;
}

datatime* datatime_from_string(const char *str) {
    if (str == NULL) return NULL;

    int d, m, y, h, min;
    if (sscanf(str, "%d/%d/%d %d:%d", &d, &m, &y, &h, &min) != 5) return NULL;

    if (!check_time_format(d, m, y, h, min)) {
        printf("Неверный формат строки\n");
        return NULL;
    }

    return datatime_create(d, m, y, h, min);
}

unsigned long long datatime_to_minutes(const datatime *dt) {
    if (dt == NULL) return 0;

    unsigned long long total = 0;
    total += (unsigned long long)dt->year * 365 * 24 * 60;
    for (int m = 0; m < dt->month; m++)
        total += month_lengths[m] * 24 * 60;
    total += (dt->day - 1) * 24 * 60;
    total += dt->hour * 60;
    total += dt->minute;

    return total;
}

/*
 * ❌ ОШИБКА: функция выделяла память под datatime, заполняла поля даты/времени,
 *    но НЕ инициализировала поле dev — оно оставалось мусором.
 *    datatime_destroy потом вызывал dev_destroy(dt->dev) с мусорным указателем → краш.
 *
 *    datatime* dt = malloc(sizeof *dt);
 *    dt->year = 1; dt->month = 1; ...  ← dev не трогался!
 *    return dt;
 *
 * ✅ ИСПРАВЛЕНО: dev инициализируется через dev_create() сразу после выделения памяти.
 */
datatime* datatime_from_minutes(unsigned long long total_minutes) {
    datatime *dt = malloc(sizeof *dt);
    if (dt == NULL) return NULL;

    /* ✅ инициализируем dev до любого return */
    dt->dev = dev_create();
    if (dt->dev == NULL) {
        free(dt);
        return NULL;
    }

    dt->year = 1; dt->month = 1; dt->day = 1; dt->hour = 0; dt->minute = 0;

    unsigned long long minutes_in_year = 365ULL * 24 * 60;
    while (total_minutes >= minutes_in_year) {
        total_minutes -= minutes_in_year;
        dt->year++;
    }

    while (dt->month <= 12) {
        unsigned long long minutes_in_month = month_lengths[dt->month - 1] * 24ULL * 60;
        if (total_minutes >= minutes_in_month) {
            total_minutes -= minutes_in_month;
            dt->month++;
        } else break;
    }

    unsigned long long minutes_in_day = 24 * 60;
    while (total_minutes >= minutes_in_day) {
        total_minutes -= minutes_in_day;
        dt->day++;
    }

    dt->hour   = (int)(total_minutes / 60);
    dt->minute = (int)(total_minutes % 60);

    return dt;
}

void datatime_print_diff(const datatime *dt1, const datatime *dt2) {
    if (dt1 == NULL || dt2 == NULL) {
        printf("Ошибка: одна из дат равна NULL\n");
        return;
    }

    const datatime *ear = dt1;
    const datatime *lat = dt2;
    if (datatime_compare(dt1, dt2) > 0) { ear = dt2; lat = dt1; }

    int years   = lat->year   - ear->year;
    int months  = lat->month  - ear->month;
    int days    = lat->day    - ear->day;
    int hours   = lat->hour   - ear->hour;
    int minutes = lat->minute - ear->minute;

    if (minutes < 0) { minutes += 60; hours--;  }
    if (hours   < 0) { hours   += 24; days--;   }
    if (days    < 0) { months--;      days += month_lengths[ear->month - 1]; }
    if (months  < 0) { years--;       months += 12; }

    printf("Разница между датами:\n");
    printf("  Лет: %d\n",     years);
    printf("  Месяцев: %d\n", months);
    printf("  Дней: %d\n",    days);
    printf("  Часов: %d\n",   hours);
    printf("  Минут: %d\n",   minutes);
}

unsigned long long datatime_diff_in_seconds_from_unix(const datatime *dt) {
    if (dt == NULL) return 0;

    datatime *unix_dt = datatime_from_minutes(unix_dt_in_minutes);
    if (unix_dt == NULL) return 0;

    if (datatime_compare(dt, unix_dt) < 0) {
        printf("Дата раньше Unix Epoch\n");
        datatime_destroy(unix_dt);
        unix_dt = NULL;
        return 0;
    }

    unsigned long long total_seconds =
        (datatime_to_minutes(dt) - unix_dt_in_minutes) * 60;

    datatime_destroy(unix_dt);
    unix_dt = NULL;

    return total_seconds;
}

/*
 * ❌ ОШИБКА: использовалась стековая переменная datatime temp с неинициализированным
 *    полем dev, затем copy_datatime(&temp, dt1) проверял temp.dev == NULL —
 *    но temp.dev был мусором (не NULL), поэтому делал temp.dev->data = ... с мусорным
 *    указателем → undefined behavior / краш.
 *
 * void datatimes_switch(datatime *dt1, datatime *dt2) {
 *     datatime temp;
 *     copy_datatime(&temp, dt1);   ← temp.dev = мусор → UB
 *     copy_datatime(dt1, dt2);
 *     copy_datatime(dt2, &temp);
 * }
 *
 * ✅ ИСПРАВЛЕНО: используем memcpy для побайтового обмена всей структуры.
 *    Это «поверхностный» swap — указатели dev меняются местами целиком,
 *    что корректно: dt1 получает устройство dt2 и наоборот.
 *    Никаких неинициализированных полей, никаких отдельных dev_create().
 */
void datatimes_switch(datatime *dt1, datatime *dt2) {
    if (dt1 == NULL || dt2 == NULL) return;

    datatime temp;
    memcpy(&temp, dt1, sizeof(datatime));
    memcpy(dt1,   dt2, sizeof(datatime));
    memcpy(dt2, &temp, sizeof(datatime));
}
