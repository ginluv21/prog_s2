#ifndef FIOVECTOR_H
#define FIOVECTOR_H

#include "../lab_3/contvector.h"

#define TXT_LL 23

typedef struct {
    int day;
    int month;
    int year;
    int hour;
    int minute;
    uint16_t dev_data; 
} data_ft;


int rand_gen_struct_in_container(vector_t *vec, size_t n);
int save_vec_txt(vector_t * const vec, const char *file);
int count_elm_txt(const char *file);
int count_elm_txt_fast(const char *file);
int count_elm_bin(const char *file);
int save_vec_bin(vector_t *vec, const char *file);
datatime* get_elm_txt_slow(const char *file, int indx);
datatime* get_elm_txt_fast(const char *file, int indx);
datatime* get_elm_bin(const char *file, int indx);
vector_t* load_vec_bin(const char *file);
vector_t* load_vec_txt(const char *file);


#endif