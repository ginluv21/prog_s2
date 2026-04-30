#ifndef FIOVECTOR_H
#define FIOVECTOR_H

#include "../lab_3/contvector.h"

#if defined(_WIN64)
    #define TXT_LL 24
#else
    #define TXT_LL 23
#endif

typedef struct {
    int day;
    int month;
    int year;
    int hour;
    int minute;
    uint16_t dev_data; 
} data_ft;


int rand_gen_struct_in_container(vector_t *vec, size_t n); // Генерация n случайных структур
int save_vec_txt(vector_t * const vec, const char *file); // Сохранение в текстовый файл
int count_elm_txt(const char *file); // Подсчет количества элементов в текстовом файле (медленный способ)
int count_elm_txt_fast(const char *file); // Подсчет количества элементов в текстовом файле (быстрый способ)
int save_vec_bin(vector_t *vec, const char *file); // Сохранение в бинарный файл
int count_elm_bin(const char *file); // Подсчет количества элементов в бинарном файле
int save_vec_bin(vector_t *vec, const char *file); // Сохранение в бинарный файл
datatime* get_elm_txt_slow(const char *file, int indx); // Получение элемента из текстового файла по индексу (медленный способ)
datatime* get_elm_txt_fast(const char *file, int indx); // Получение элемента из текстового файла по индексу (быстрый способ)
datatime* get_elm_bin(const char *file, int indx); // Получение элемента из бинарного файла по индексу
vector_t* load_vec_bin(const char *file); // Загрузка вектор из бинарного файла
vector_t* load_vec_txt(const char *file); // Загрузка вектор из текстового файла


#endif