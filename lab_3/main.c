// https://open.spotify.com/album/5epJvt9jHbYI1j6WqCppGc?si=Ku7bcDeEStCHNp6WAwQ1gg
// https://open.spotify.com/playlist/6f2WRnB0KEgqGKr53FuwOV?si=8m4RXWNqQd-2P1pu0bZXfA
#include "contvector.h"

int main() {
    printf("1. СОЗДАНИЕ И PUSH\n");
    vector_t *v1 = vec_create(2);
    vec_push(v1, datatime_create(13, 2, 1970, 15, 43));
    vec_push(v1, datatime_create(8, 9, 1970, 22, 43));
    printf("Вектор v1 после создания и push:\n");
    print_vector(v1);

    printf("Добавляем 3-й элемент:\n");
    vec_push(v1, datatime_create(18, 9, 1970, 10, 0));
    printf("Вектор v1 после добавления 3-го элемента:\n");
    print_vector(v1);

    printf("2. ВСТАВКА (INSERT) И ИЗМЕНЕНИЕ (CHANGE)\n");
    printf("Вставляем 21.07.1971 20:00 на индекс 1:\n");
    vec_insert(v1, 1, datatime_create(21, 7, 1971, 20, 0));
    print_vector(v1);

    printf("Вставляем 15.05.2023 12:00 на индекс 1:\n");
    vec_insert(v1, 1, datatime_create(25, 9, 1972, 2, 12));
    print_vector(v1);

    printf("3. УДАЛЕНИЕ (REMOVE) И ИЗВЛЕЧЕНИЕ (POP)\n");
    printf("Удаляем элемент по индексу 2:\n");
    vec_remove(v1, 2);
    print_vector(v1);

    printf("Делаем pop (забираем последний элемент):\n");
    datatime *popped = vec_pop(v1);
    printf("Извлечённый элемент:\n");
    datatime_print(popped);
    datatime_destroy(popped);
    printf("Вектор v1 после pop:\n");
    print_vector(v1);

    printf("4. КОПИРОВАНИЕ (COPY) И ОБЪЕДИНЕНИЕ (MERGE)\n");
    printf("Создаём копию v1 в v2:\n");
    vector_t *v2 = vec_copy(v1);
    printf("Вектор v2 (копия v1):\n");
    print_vector(v2);

    printf("5. СЛИЯНИЕ (MERGE)\n");
    printf("Объединяем v1 и v2 в v1:\n");
    vec_merge(v1, v2);
    printf("Вектор v1 после объединения с v2:\n");
    print_vector(v1);

    printf("6. ИТЕРАТОРЫ (ПЕРЕБОР МАССИВА)\n");
    vec_iter_t beg = vec_begin(v1);
    vec_iter_t end = vec_end(v1);
    printf("Перебор элементов вектора v1 с помощью итераторов:\n");
    while (!vec_isequal(beg, end)) {
        if(vec_iter_belong(beg, v1)){
            datatime *dt = vec_get(v1, beg.ind);
            printf("Итератор указывает на индекс %zu: ", beg.ind);
            datatime_print(dt);
        }
        vec_iter_next(&beg);
    }

    printf("7. ОЧИСТКА ПАМЯТИ (DESTROY)\n");
    vec_destroy(v1);
    vec_destroy(v2);
    v1 = NULL;
    v2 = NULL;
    return 0;
}