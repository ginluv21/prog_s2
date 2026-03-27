#ifndef VECTOR_H
#define VECTOR_H


#include <stdio.h>
#include <stdlib.h>
#include "../lab_2/datatime.h"

typedef struct{
    datatime** data;
    datatime** res;
    size_t len;
    size_t cap;
} vector_t;

typedef struct{
    vector_t *vec;
    size_t ind;
}vec_iter_t;

vector_t *vec_create(size_t cap); // создание вектора
void vec_destroy(vector_t *vec); // уничтожение вектора и всех его элементов
int vec_push(vector_t *vec, datatime *dt); // добавление элемента в конец вектора
datatime *vec_pop(vector_t *vec); // удаление элемента из конца вектора
datatime *vec_get(vector_t *vec, size_t ind); // получение элемента по индексу
size_t vec_len(vector_t *vec); // получение количества элементов в векторе
size_t vec_cap(vector_t *vec); // получение вместимости вектора
int vec_insert(vector_t *vec, size_t ind, datatime *dt); // вставка элемента по индексу
int vec_remove(vector_t *vec, size_t ind); // удаление элемента по индексу
int vec_change(vector_t *vec, size_t ind, datatime *dt); // изменение элемента по индексу
vector_t *vec_copy(vector_t *vec); // создание копии вектора
int vec_merge(vector_t *v1, vector_t *v2); // объединение двух векторов
vec_iter_t vec_begin(vector_t *vec); // получение итератора на начало вектора
vec_iter_t vec_end(vector_t *vec); // получение итератора на конец вектора
void vec_iter_next(vec_iter_t *iter); // переход к следующему элементу итератора
int vec_isequal(vec_iter_t it1, vec_iter_t it2); // сравнение двух итераторов
int vec_iter_belong(vec_iter_t it, vector_t *vec); // проверка принадлежности итератора вектору
void print_vector(vector_t *vec); // печать вектора (для отладки)
#endif // VECTOR_H