#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "lab_2/datatime.h"
#include "lab_3/contvector.h"
#include "lab_4/fiovector.h"

#define N 10000
#define TXT_FILE "test_data.txt"
#define BIN_FILE "test_data.bin"

static double wtime(void) {
    return (double)clock() / CLOCKS_PER_SEC * 1000.0;
}

int main(void) {
    srand((unsigned)time(NULL));
    double t;

    // --- базовые операции с datatime ---

    printf("тест 1: datatime_create + print\n");
    datatime *dt1 = datatime_create(15, 3, 2025, 10, 30);
    datatime *dt2 = datatime_create(20, 5, 2025, 18, 0);
    datatime_print(dt1);
    datatime_print(dt2);
    printf("ок\n\n");

    printf("тест 2: datatime_compare\n");
    int cmp = datatime_compare(dt1, dt2);
    printf("dt1 < dt2: %s\n", cmp < 0 ? "да" : "нет");
    printf("ок\n\n");

    printf("тест 3: datatime_to_minutes\n");
    datatime *unix_start = datatime_create(1, 1, 1970, 0, 0);
    unsigned long long mins = datatime_to_minutes(unix_start);
    printf("01/01/1970 -> %llu минут (ожидалось 1035432000)\n", mins);
    datatime_destroy(unix_start);
    printf("ок\n\n");

    // --- операции с вектором ---

    printf("тест 4: vec push / pop / insert / remove\n");
    vector_t *vec = vec_create(2);
    vec_push(vec, datatime_create(1,  1, 2000,  0,  0));
    vec_push(vec, datatime_create(2,  2, 2001,  1,  1));
    vec_push(vec, datatime_create(3,  3, 2002,  2,  2));
    printf("после 3 push: len=%zu\n", vec_len(vec));

    datatime *popped = vec_pop(vec);
    printf("pop: "); datatime_print(popped);
    printf("после pop: len=%zu\n", vec_len(vec));
    // [LEAK 1] vec_pop отдаёт владение вызывателю — без destroy объект утекает
    // раскомментировать чтобы исправить:
    // datatime_destroy(popped);

    vec_insert(vec, 0, datatime_create(31, 12, 1999, 23, 59));
    printf("insert [0]: len=%zu, [0]=", vec_len(vec));
    datatime_print(vec_get(vec, 0));
    vec_remove(vec, 0);
    printf("remove [0]: len=%zu\n", vec_len(vec));
    printf("ок\n\n");

    printf("тест 5: vec_copy\n");
    vector_t *copy = vec_copy(vec);
    printf("оригинал len=%zu, копия len=%zu\n", vec_len(vec), vec_len(copy));
    vec_destroy(copy);
    vec_destroy(vec);
    // [LEAK 2] dt1 и dt2 созданы в тесте 1 — без destroy утекают
    // раскомментировать чтобы исправить:
    // datatime_destroy(dt1);
    // datatime_destroy(dt2);
    printf("ок\n\n");

    printf("тест 6: vec_merge\n");
    vector_t *v1 = vec_create(2);
    vector_t *v2 = vec_create(2);
    vec_push(v1, datatime_create(1, 6, 2020, 12, 0));
    vec_push(v2, datatime_create(2, 7, 2021, 13, 0));
    printf("до merge: v1 len=%zu, v2 len=%zu\n", vec_len(v1), vec_len(v2));
    vec_merge(v1, v2);
    printf("после merge: v1 len=%zu\n", vec_len(v1));
    vec_destroy(v1);
    vec_destroy(v2);
    printf("ок\n\n");

    printf("тест 7: datatime_print(NULL)\n");
    datatime_print(NULL);
    printf("ок — NULL обработан без краша\n\n");

    printf("---\n\n");

    // --- файловый ввод-вывод: N элементов ---

    printf("тест 8: генерация %d элементов\n", N);
    vector_t *big = vec_create(N);
    t = wtime();
    rand_gen_struct_in_container(big, N);
    printf("генерация: %.2f мс\n", wtime() - t);
    printf("ок\n\n");

    printf("тест 9: сохранение в файлы\n");
    t = wtime(); save_vec_txt(big, TXT_FILE);
    printf("save txt: %.2f мс\n", wtime() - t);

    t = wtime(); save_vec_bin(big, BIN_FILE);
    printf("save bin: %.2f мс\n", wtime() - t);

    vec_destroy(big);
    printf("ок\n\n");

    // count_elm_txt читает файл построчно — O(n)
    // count_elm_txt_fast и count_elm_bin считают через размер файла — O(1)
    printf("тест 10: count — slow vs fast\n");
    int cnt;

    t = wtime(); cnt = count_elm_txt(TXT_FILE);
    printf("count_elm_txt  (slow O(n)): %d эл, %.3f мс\n", cnt, wtime() - t);

    t = wtime(); cnt = count_elm_txt_fast(TXT_FILE);
    printf("count_elm_fast (fast O(1)): %d эл, %.3f мс\n", cnt, wtime() - t);

    t = wtime(); cnt = count_elm_bin(BIN_FILE);
    printf("count_elm_bin  (fast O(1)): %d эл, %.3f мс\n", cnt, wtime() - t);
    printf("ок\n\n");

    // get_elm_txt_slow сканирует с начала файла — O(n)
    // get_elm_txt_fast и get_elm_bin прыгают fseek на нужную позицию — O(1)
    printf("тест 11: get [%d] — slow vs fast\n", N / 2);
    datatime *el;

    t = wtime(); el = get_elm_txt_slow(TXT_FILE, N / 2);
    printf("txt slow: "); datatime_print(el); datatime_destroy(el);
    printf("время: %.3f мс\n", wtime() - t);

    t = wtime(); el = get_elm_txt_fast(TXT_FILE, N / 2);
    printf("txt fast: "); datatime_print(el); datatime_destroy(el);
    printf("время: %.3f мс\n", wtime() - t);

    t = wtime(); el = get_elm_bin(BIN_FILE, N / 2);
    printf("bin:      "); datatime_print(el); datatime_destroy(el);
    printf("время: %.3f мс\n", wtime() - t);
    printf("ок\n\n");

    printf("тест 12: load всего файла\n");
    vector_t *loaded;

    t = wtime(); loaded = load_vec_txt(TXT_FILE);
    printf("load_vec_txt: %zu эл, %.2f мс\n", vec_len(loaded), wtime() - t);
    vec_destroy(loaded);

    t = wtime(); loaded = load_vec_bin(BIN_FILE);
    printf("load_vec_bin: %zu эл, %.2f мс\n", vec_len(loaded), wtime() - t);
    vec_destroy(loaded);
    printf("ок\n\n");

    printf("---\n");
    printf("все тесты пройдены\n\n");

    // -----------------------------------------------
    // заготовки для демонстрации проблем
    // раскомментировать нужный блок перед запуском
    // -----------------------------------------------

    // [DOUBLE FREE] программа упадёт — краш хорошо видно в GDB
    // datatime *df = datatime_create(1, 1, 2001, 0, 0);
    // datatime_destroy(df);
    // datatime_destroy(df);

    // [USE AFTER FREE] UB — хорошо видно в Valgrind
    // datatime *uaf = datatime_create(5, 5, 2005, 5, 5);
    // datatime_destroy(uaf);
    // datatime_print(uaf);

    return 0;
}
