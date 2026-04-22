#include "fiovector.h"

int rand_gen_struct_in_container(vector_t *vec, size_t n) {

    if(vec == NULL) return 1;

    for(size_t i = 0; i < n; i++){ 
        int m = 1 + rand() % 12;
        int d = 1 + rand() % month_lengths[m - 1];
        int y = 1900 + rand() % 200; 
        int h = rand() % 24;
        int min = rand() % 60;
        
        datatime *dt = datatime_create(d, m, y, h, min);
        if(dt != NULL){
            if(dt->dev != NULL){
                dev_set_display(dt->dev, rand() % 4); 
                dev_set_brightness(dt->dev, rand() % 16);
                dev_set_time_format(dt->dev, rand() % 2);
                dev_set_alarm(dt->dev, rand() % 2);
                dev_set_memory(dt->dev, rand() % 4);
                dev_set_cpu(dt->dev, rand() % 4); 
                dev_set_water(dt->dev, rand() % 3);
            }

            vec_push(vec, dt);
        }
    }

    return 0;
}


int save_vec_txt(vector_t * const vec, const char *file){
    if(vec == NULL || file == NULL) return 1;

    FILE *fp = fopen(file, "w");
    if(fp == NULL){
        perror("Ошибка открытия файла для записи (text)");
        return 1;
    }

    for(int i = 0; i < vec->len; i++){
        datatime *dt = vec->data[i];
        if(dt == NULL) continue;

        uint16_t dev = (dt->dev != NULL) ? dt->dev->data : 0;

        fprintf(fp, "%02d %02d %04d %02d %02d %05u\n", 
                dt->day, dt->month, dt->year, dt->hour, dt->minute, dev);
    }

    fclose(fp);
    return 0;
}

datatime* get_elm_txt_slow(const char *file, int indx){
    if(file == NULL || indx < 0) return NULL;

    FILE *fp = fopen(file, "r");
    if(fp == NULL){
        perror("Ошибка открытия файла для чтения (текст)");
        return NULL;
    }

    int curr_indx = 0;
    int d, m, y, h, min;
    //uint16_t dev;
    unsigned int dev;
    datatime *res = NULL;

    while (fscanf(fp, "%d %d %d %d %d %u", &d, &m, &y, &h, &min, &dev) == 6){
        if(curr_indx == indx){
            res = datatime_create(d, m, y, h, min);
            if(res != NULL){
                if(res->dev != NULL){
                    dev_set_raw_data(res->dev, (uint16_t)dev);
                }
            }
            break;
        }
        curr_indx++;
    }
    
    fclose(fp);
    return res;
}


datatime* get_elm_txt_fast(const char *file, int indx){
    if(file == NULL || indx < 0) return NULL;

    FILE *fp = fopen(file, "r");
    if(fp == NULL){
        perror("Ошибка открытия файла для чтения (текст)");
        return NULL;
    }

    long offs = indx * TXT_LL;

    if(fseek(fp, offs, SEEK_SET) != 0){
        fclose(fp);
        return NULL;
    }

    int d, m, y, h, min;
    //uint16_t dev;
    unsigned int dev;
    datatime *res = NULL;

    if(fscanf(fp, "%d %d %d %d %d %u", &d, &m, &y, &h, &min, &dev) == 6){
        res = datatime_create(d, m, y, h, min);
        if(res != NULL){
            if(res->dev != NULL){
                dev_set_raw_data(res->dev, (uint16_t)dev);
            }
        }
    }

    fclose(fp);
    return res;
}

vector_t* load_vec_txt(const char *file){
    if(file == NULL) return NULL;

    FILE *fp = fopen(file, "r");
    if(fp == NULL){
        perror("Ошибка открытия файла для чтения (текст)");
        return NULL;
    }

    int count = count_elm_txt_fast(file);
    if(count <= 0) count = 1;

    vector_t *vec = vec_create(count);
    if(vec == NULL){
        fclose(fp);
        return NULL;
    }

    int d, m, y, h, min;
    unsigned int dev;

    while(fscanf(fp, "%d %d %d %d %d %u", &d, &m, &y, &h, &min, &dev) == 6){
        datatime *dt = datatime_create(d, m, y, h, min);
        if(dt != NULL){
            if(dt->dev != NULL)
                dev_set_raw_data(dt->dev, (uint16_t)dev);
            
            vec_push(vec, dt);
        }
    }

    fclose(fp);
    return vec;
}

int count_elm_txt(const char *file){
    FILE *fp = fopen(file, "r");
    if(fp == NULL) return 0;
    
    int count = 0;
    int d, m, y, h, min;
    //uint16_t dev;
    unsigned int dev;

    while (fscanf(fp, "%d %d %d %d %d %u", &d, &m, &y, &h, &min, &dev) == 6)
        count++;

    fclose(fp);
    return count;
}

int count_elm_txt_fast(const char *file) {
    if (file == NULL) return 0;

    FILE *fp = fopen(file, "r");
    if (fp == NULL) return 0;

    fseek(fp, 0, SEEK_END);
    
    long file_size = ftell(fp);
    
    fclose(fp);

    return (int)(file_size / TXT_LL);
}

int save_vec_bin(vector_t *vec, const char *file){
    if(vec == NULL || file == NULL) return 1;

    FILE *fp = fopen(file, "wb");
    if(fp == NULL){
        perror("Ошибка открытия файла для записи (bin)");
        return 1;
    }

    for(size_t i =0; i < vec->len; i++){
        datatime *dt = vec->data[i];
        if(dt == NULL){
            fclose(fp);
            return 1;
        }

        data_ft rec;
        rec.day = dt->day;
        rec.month = dt->month;
        rec.year = dt->year;
        rec.hour = dt->hour;
        rec.minute = dt->minute;
        rec.dev_data = (dt->dev != NULL) ? dt->dev->data : 0;
        

        fwrite(&rec, sizeof(data_ft), 1, fp);
    }

    fclose(fp);
    return 0;
}

vector_t* load_vec_bin(const char *file){
    if(file == NULL) return NULL;

    FILE *fp = fopen(file, "rb");
    if(fp == NULL){
        perror("Ошибка открытия файла для чтения (bin)");
        return NULL;
    }

    int a = count_elm_bin(file);

    vector_t *vec = vec_create(a); 

    if(vec == NULL){
        fclose(fp);
        return NULL;
    }

    data_ft rec;
    while(fread(&rec, sizeof(data_ft), 1, fp) == 1){
        datatime *dt = datatime_create(rec.day, rec.month,
                rec.year, rec.hour, rec.minute);

        if(dt != NULL){
            if(dt->dev != NULL)
                dev_set_raw_data(dt->dev, rec.dev_data);
            
            vec_push(vec, dt);
        }
    }

    fclose(fp);
    return vec;
}

datatime* get_elm_bin(const char *file, int indx){
    if(file == NULL || indx < 0) return NULL;

    FILE *fp = fopen(file, "rb");
    if(fp == NULL) return NULL;

    long offs = indx * sizeof(data_ft);

    if(fseek(fp, offs, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }

    data_ft rec; 
    datatime *res = NULL;

    if(fread(&rec, sizeof(data_ft), 1, fp) == 1){
        res = datatime_create(rec.day, rec.month,
                rec.year, rec.hour, rec.minute);

        if(res != NULL){
            if(res->dev != NULL)
                dev_set_raw_data(res->dev, rec.dev_data);
        }
    }

    fclose(fp);
    return res;
}

int count_elm_bin(const char *file){
    if(file == NULL) return 0;

    FILE *fp = fopen(file, "rb");
    if(fp == NULL) return 0;

    fseek(fp, 0, SEEK_END);

    long file_size = ftell(fp);

    fclose(fp);

    return (int)file_size / sizeof(data_ft);
}

