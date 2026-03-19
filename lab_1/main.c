#include "datatime.h"

int main(void) {
    printf("== Практическая работа №1 ==\n");
    printf("Структура: datatime (Дата и время)\n\n");

    // 1. Создание и инициализация (3 балла)

    datatime *dt1 = datatime_create(1, 2, 2026, 14, 30);
    datatime *dt2 = datatime_create(1, 2, 2026, 14, 32);

    printf("Исходные даты и время:\n");
    datatime_print(dt1);
    datatime_print(dt2);
    printf("\n");

    // 2. Сравнение дат (3 балла)

    int cmp = datatime_compare(dt1, dt2);
    if (cmp < 0) {
        printf("Первая дата меньше второй на %llu минут\n", datatime_to_minutes(dt2) - datatime_to_minutes(dt1));
    } else if (cmp > 0) {
        printf("Первая дата больше второй на %llu минут\n", datatime_to_minutes(dt1) - datatime_to_minutes(dt2));
    } else {
        printf("Даты равны\n");
    }
    printf("\n");

    // Дополнительные функции: создание пустого объекта и копирование (2 балла)
    datatime *empty_dt = create_empty_datatime();
    printf("Пустой объект datatime:\n");
    datatime_print(empty_dt);
    printf("Копируем первую дату в пустой объект:\n");
    copy_datatime(empty_dt, dt1);
    printf("После копирования:\n");
    datatime_print(empty_dt);
    printf("\n");
    datatime_destroy(&empty_dt);
      

    // 3. Изменение данных (++ / setter)
    printf("Увеличиваем первую дату на 1 минуту:\n");
    datatime_min_increment(dt1);
    datatime_print(dt1);

    printf("Уменьшаем вторую дату на 4 минуты:\n");
    datatime_min_decrement(dt2);
    datatime_min_decrement(dt2);
    datatime_min_decrement(dt2);
    datatime_min_decrement(dt2);
    datatime_print(dt2);
    printf("\n");

    printf("Устанавливаем часы первой даты в 16:\n");
    datatime_hour_set(dt1, 16);
    datatime_print(dt1);
    printf("\n");

    // 4. Строковый ввод / вывод (4 балла)

    char *str = datatime_to_string(dt1);
    if (str != NULL) {
        printf("Первая дата в строковом виде: %s\n", str);
        free(str);
    }
    printf("\n");

    datatime *dt3 = datatime_from_string("05/03/2026 10:15");
    printf("Дата, созданная из строки: \"05/03/2026 10:15\"\n");
    datatime_print(dt3);
    printf("\n");

    // 5. Разница между датами (4 балла)
    printf("Разница между первой и второй датой:\n");
    datatime_print_diff(dt1, dt2);
    printf("\n");

    // 6. Unix time (5 баллов)
    unsigned long long unix_time =
        datatime_diff_in_seconds_from_unix(dt3);

    printf("Количество секунд с начала эпохи Unix для третьей даты:\n");
    printf("%llu секунд\n\n", unix_time);


    // 8. Освобождение памяти
    datatime_destroy(&dt1);
    datatime_destroy(&dt2);
    datatime_destroy(&dt3);

    datatime_destroy(&dt3);
    printf("Память освобождена. Программа завершена.\n");
    return 0;
}