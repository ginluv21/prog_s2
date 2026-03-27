#include "contvector.h"

vector_t *vec_create(size_t ini_cap){
    vector_t* temp = malloc(sizeof(vector_t));
    if(temp == NULL) return NULL;

    if(ini_cap == 0)
        ini_cap = 1;
    temp->cap = ini_cap;
    temp->len = 0;

    temp->data = malloc(temp->cap * sizeof(datatime*));
    if(temp->data == NULL){
        free(temp);
        return NULL;
    }

    size_t res_cap = (size_t)(temp->cap * 1.41);
    if(res_cap <= temp->cap)
        res_cap = temp->cap + 1;

    temp->res = malloc(res_cap * sizeof(datatime*));
    if(temp->res == NULL){
        free(temp->data);
        free(temp);
        return NULL;
    }

    return temp;
}

void vec_destroy(vector_t *vec){
    if(vec == NULL) return;

    for(int i = 0; i < vec->len; i++)
        datatime_destroy(*(vec->data + i));

    free(vec->data);
    free(vec->res);
    free(vec);
}

static void vec_resize(vector_t *vec, size_t new_cap){
    if(vec == NULL) return;
    vec->data = realloc(vec->data, new_cap * sizeof(datatime*));
    vec->cap = new_cap;
}

static int vec_reserve(vector_t *vec){
    if(vec == NULL) return 1;

    for(size_t i = 0; i < vec->len; i++)
        *(vec->res + i) = *(vec->data + i);

    free(vec->data);
    vec->data = vec->res;

    size_t new_cap = (size_t)(vec->cap * 1.41);
    if(new_cap <= vec->cap)
        new_cap = vec->cap + 1;
    vec->cap = new_cap;

    size_t res_cap = (size_t)(vec->cap * 1.41);
    if(res_cap <= vec->cap)
        res_cap = vec->cap + 1;

    vec->res = malloc(res_cap * sizeof(datatime*));
    if(vec->res == NULL) return 1;

    return 0;
}

int vec_push(vector_t *vec, datatime *dt){
    if(vec == NULL || dt == NULL) return 1;

    if(vec->len == vec->cap){
        vec_reserve(vec);
    }
        
    *(vec->data + vec->len) = dt;
    vec->len++;

    return 0;
} 

datatime *vec_pop(vector_t *vec){
    if(vec == NULL || vec->len == 0)
        return NULL;

    datatime *temp = *(vec->data + vec->len - 1);
    vec->len--;

    if(vec->len == vec->cap / 4 && vec->len > 0)
        vec_resize(vec, vec->cap / 2);

    return temp;
}


datatime *vec_get(vector_t *vec, size_t ind){
    if(vec == NULL || ind >= vec->len)
        return NULL;

    return *(vec->data + ind);
}

size_t vec_len(vector_t *vec){
    if(vec == NULL) return 0;

    return vec->len;
}

size_t vec_cap(vector_t *vec){
    if(vec == NULL) return 0;

    return vec->cap;
}

static void vec_shr(vector_t *vec, size_t ind){
    for(int i = vec->len; i > ind; i--)
        *(vec->data + i) = *(vec->data + i - 1);
}

static void vec_shl(vector_t *vec, size_t ind){
    for(int i = ind; i < vec->len -1; i++)
        *(vec->data + i) = *(vec->data + i + 1);
}

int vec_insert(vector_t *vec, size_t ind, datatime *dt){
    if(vec == NULL || dt == NULL) return 1;
    if(ind > vec->len) return 1;

    if(ind == vec->len)
        return vec_push(vec, dt);

    if(vec->len == vec->cap){
        vec_reserve(vec);
    }

    vec_shr(vec, ind);
    *(vec->data + ind) = dt;
    vec->len++;
    return 0;
}

int vec_remove(vector_t *vec, size_t ind){
    if(vec == NULL) return 1;
    if(ind >= vec->len) return 1;

    if (ind == vec->len - 1) {
        datatime_destroy(vec_pop(vec));
        return 0;
    }

    datatime *temp = *(vec->data + ind);

    vec_shl(vec, ind);
    vec->len--;

    if(vec->len == vec->cap / 4 && vec->len > 0)
        vec_resize(vec, vec->cap / 2);

    datatime_destroy(temp);

    return 0;
}

int vec_change(vector_t *vec, size_t ind, datatime *dt){
    if(vec == NULL || dt == NULL) return 1;
    if(ind >= vec->len) return 1;

    datatime_destroy(*(vec->data + ind));

    *(vec->data + ind) = dt;

    return 0;
}

vector_t *vec_copy(vector_t *vec){
    if(vec == NULL) return NULL;

    vector_t *temp = vec_create(vec->cap);
    if(temp == NULL) return NULL;

    for(int i = 0; i < vec->len; i++){
        datatime *new_dt = malloc(sizeof(datatime));
        if(new_dt == NULL) return NULL;

        copy_datatime(new_dt, *(vec->data + i));
        *(temp->data + i) = new_dt;
        temp->len++;
    }
    return temp;
}

int vec_merge(vector_t *v1, vector_t *v2){
    if(v1 == NULL || v2 == NULL) return 1;

    size_t total_len = v1->len + v2->len;
    if(total_len > v1->cap)
        vec_resize(v1,total_len);
    
    for(int i = 0; i < v2->len; i++){
        datatime *new_dt = malloc(sizeof(datatime));

        if(new_dt == NULL) return 1;
        copy_datatime(new_dt, *(v2->data + i));
        vec_push(v1, new_dt);
    }

    return 0;
}


vec_iter_t vec_begin(vector_t *vec){
    vec_iter_t temp;
    temp.vec = vec;
    temp.ind = 0;
    return temp;
}

vec_iter_t vec_end(vector_t *vec){
    vec_iter_t temp;
    temp.vec = vec;
    temp.ind = (vec->len) ? vec->len : 0;
    return temp;
}

void vec_iter_next(vec_iter_t *iter){
    if(iter->vec->len > iter->ind)
        iter->ind++;
}

int vec_isequal(vec_iter_t it1, vec_iter_t it2){
    if(it1.vec == it2.vec && it1.ind == it2.ind)
        return 1;
    return 0;
}
int vec_iter_belong(vec_iter_t it, vector_t *vec){
    if(it.vec == vec)
        return 1;
    return 0;
}

void print_vector(vector_t *vec) {
    printf("Вектор [len: %zu, cap: %zu]:\n", vec_len(vec), vec_cap(vec));
    for (size_t i = 0; i < vec_len(vec); i++) {
        datatime *dt = vec_get(vec, i);
        if (dt) {
            printf("  [%zu] -> ", i);
            datatime_print(dt); // Твоя функция
        }
    }
    printf("\n");
}
