#ifndef VECTOR_H
#define VECTOR_H


#include <stdio.h>
#include <stdlib.h>
#include "../lab_2/datatime.h"

typedef struct{
    datatime** data;
    size_t len;
    size_t cap;
} vector_t;




#endif // VECTOR_H