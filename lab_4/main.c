#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fiovector.h"

int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));

    if (argc < 4) {
        printf("Использование:\n");
        printf("  %s save <txt|bin> <файл> [количество]\n", argv[0]);
        printf("  %s load <txt|bin> <файл>\n", argv[0]);
        printf("  %s list <txt|bin> <файл>\n", argv[0]);
        printf("  %s get  <txt|bin> <индекс> <файл>\n", argv[0]);
        return 1;
    }

    char *cmd  = argv[1];
    char *mode = argv[2];

    /* SAVE */
    if (strcmp(cmd, "save") == 0) {
        char *file  = argv[3];
        int   count = (argc >= 5) ? atoi(argv[4]) : 10000;

        vector_t *vec = vec_create((size_t)count);
        if (vec == NULL) { printf("Ошибка выделения памяти\n"); return 1; }

        printf("Генерация %d элементов...\n", count);
        rand_gen_struct_in_container(vec, (size_t)count);

        if (strcmp(mode, "txt") == 0) save_vec_txt(vec, file);
        else                          save_vec_bin(vec, file);

        printf("Успешно сохранено %d элементов в '%s'\n", count, file);
        vec_destroy(vec);
        vec = NULL;
        return 0;
    }

    /* LOAD */
    else if (strcmp(cmd, "load") == 0) {
        char     *file = argv[3];
        vector_t *vec  = (strcmp(mode, "txt") == 0)
                           ? load_vec_txt(file)
                           : load_vec_bin(file);

        if (vec == NULL) {
            printf("Ошибка загрузки файла %s\n", file);
            return 1;
        }

        printf("Загружено %zu элементов.\n", vec->len);

        for (size_t i = 0; i < vec->len; i++) {
            printf("[%zu] ", i);
            datatime_print(vec->data[i]);
        }

        vec_destroy(vec);
        vec = NULL;
        return 0;
    }

    /* LIST */
    else if (strcmp(cmd, "list") == 0) {
        char *file  = argv[3];
        int   count = (strcmp(mode, "txt") == 0)
                        ? count_elm_txt_fast(file)
                        : count_elm_bin(file);
        printf("В файле '%s' найдено записей: %d\n", file, count);
        return 0;
    }

    /* GET */
    else if (strcmp(cmd, "get") == 0) {
        if (argc < 5) {
            printf("Для get укажите индекс и файл: %s get bin 5 file.bin\n", argv[0]);
            return 1;
        }

        int   index = atoi(argv[3]);
        char *file  = argv[4];

        datatime *dt = (strcmp(mode, "txt") == 0)
                         ? get_elm_txt_fast(file, index)
                         : get_elm_bin(file, index);

        if (dt != NULL) {
            printf("Элемент [%d]: ", index);
            datatime_print(dt);

            /*
             * ❌ ОШИБКА: ручное освобождение через free() в обход datatime_destroy.
             *    free(dt->dev) — не вызывает dev_destroy, нарушает инкапсуляцию.
             *    free(dt)      — не обнуляет указатель.
             *
             * free(dt->dev);
             * free(dt);
             *
             * ✅ ИСПРАВЛЕНО: используем штатную функцию уничтожения,
             *    которая правильно освобождает dev, а затем саму структуру.
             */
            datatime_destroy(dt);
            dt = NULL;

        } else {
            printf("Элемент с индексом %d не найден!\n", index);
        }

        return 0;
    }

    printf("Неизвестная команда.\n");
    return 1;
}
