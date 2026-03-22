// https://open.spotify.com/album/5epJvt9jHbYI1j6WqCppGc?si=Ku7bcDeEStCHNp6WAwQ1gg
// https://open.spotify.com/playlist/6f2WRnB0KEgqGKr53FuwOV?si=8m4RXWNqQd-2P1pu0bZXfA
#include "contvector.h"

vector_t * vec_create(size_t ini_cap){
    vector_t* temp = malloc(sizeof(vector_t));

    if(ini_cap == 0)
        temp->cap = 1;
    else
        temp->cap = ini_cap;
    temp->len = 0;

    temp->data = malloc(temp->cap * sizeof(datatime*));

    return temp;
}

void vec_destroy(vector_t *vec){
    if(vec == NULL) return;

    for(int i = 0; i < vec->len; i++)
        datatime_destroy(*(vec->data + i));

    free(vec->data);
    free(vec);
}

static void vec_resize(vector_t *vec, size_t new_cap){
    if(vec == NULL) return;
    vec->data = realloc(vec->data, new_cap * sizeof(datatime*));
    vec->cap = new_cap;
}

static int vec_reserve(vector_t *vec, size_t new_cap){
    if(vec == NULL) return 0;
    if(new_cap <= vec->cap) return 1;

    datatime** new_data = malloc(new_cap * sizeof(datatime*));
    if(new_data == NULL) return 0;

    for(int i = 0; i < vec->len; i++)
        *(new_data + i) = *(vec->data + i);

    free(vec->data);
    vec->data = new_data;
    vec->cap = new_cap;
    return 1;
}

int vec_push(vector_t *vec, datatime *dt){
    if(vec == NULL || dt == NULL) return 1;

    if(vec->len == vec->cap){
        size_t new_cap = (size_t)(vec->cap * 1.41);
        
        if(new_cap <= vec->cap)
            new_cap = vec->cap + 1;

        if(!vec_reserve(vec, new_cap))
            return 1;
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

    if(ind == vec->len){
        return vec_push(vec, dt);
        return 0;
    }
    if(vec->len == vec->cap){
        size_t new_cap = (size_t)(vec->cap * 1.41);

        if(new_cap <= vec->cap)
            new_cap = vec->cap + 1;

    if(!vec_reserve(vec, new_cap))
        return 1;
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


