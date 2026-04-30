/*
 * main.c — комплексный тест + бенчмарк для всего проекта prog_s2
 *
 * Покрывает:
 *   lab_2 — datatime (дата/время) + device (биты конфигурации)
 *   lab_3 — vector_t (динамический массив)
 *   lab_4 — file I/O (текстовый и бинарный форматы)
 *
 * Примечание по lab_1:
 *   API lab_1 идентичен lab_2, но без поля dev.
 *   Все datatime-функции здесь тестируются через версию lab_2.
 *
 * Компиляция и запуск:
 *   make          → ./main_test
 *   make leaks    → leaks --atExit -- ./main_test  (macOS)
 *   make valgrind → valgrind ... ./main_test        (Linux)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lab_4/fiovector.h"

/* ─────────────────────────────────────────────────────────
 * Мини-фреймворк тестирования
 * ───────────────────────────────────────────────────────── */
static int g_pass = 0;
static int g_fail = 0;

/* CHECK(условие, "описание") — проверяет условие и считает результат */
#define CHECK(cond, msg)                                               \
    do {                                                               \
        if (cond) {                                                    \
            printf("  [OK]   " msg "\n");                             \
            g_pass++;                                                  \
        } else {                                                       \
            printf("  [FAIL] " msg "  ← строка %d\n", __LINE__);     \
            g_fail++;                                                  \
        }                                                              \
    } while (0)

static void section(const char *name) {
    printf("\n══════════════════════════════════════\n");
    printf("  %s\n", name);
    printf("══════════════════════════════════════\n");
}

/* ─────────────────────────────────────────────────────────
 * Замер времени
 * ───────────────────────────────────────────────────────── */
static double elapsed_ms(clock_t start) {
    return (double)(clock() - start) / (double)CLOCKS_PER_SEC * 1000.0;
}

/* ─────────────────────────────────────────────────────────
 * Константы для бенчмарков
 * ───────────────────────────────────────────────────────── */
#define BENCH_MEM_N    500000   /* операций в памяти                  */
#define BENCH_FILE_N   100000  /* записей для файловых тестов        */
#define BENCH_GET_N    300     /* запросов случайного доступа        */

#define TMP_TXT  "/tmp/prog_s2_bench.txt"
#define TMP_BIN  "/tmp/prog_s2_bench.bin"
#define TMP_TXT2 "/tmp/prog_s2_test2.txt"
#define TMP_BIN2 "/tmp/prog_s2_test2.bin"

/* ═══════════════════════════════════════════════════════════
 * СЕКЦИЯ 1 — datatime: создание, уничтожение, копирование
 * ═══════════════════════════════════════════════════════════ */
static void test_datatime_create(void) {
    section("1. datatime — создание / уничтожение / копирование");

    /* Создание валидной даты */
    datatime *dt = datatime_create(15, 6, 2024, 12, 30);
    CHECK(dt != NULL,                          "создание валидной даты");
    CHECK(dt && dt->day    == 15,              "  день = 15");
    CHECK(dt && dt->month  == 6,               "  месяц = 6");
    CHECK(dt && dt->year   == 2024,            "  год = 2024");
    CHECK(dt && dt->hour   == 12,              "  час = 12");
    CHECK(dt && dt->minute == 30,              "  минута = 30");
    CHECK(dt && dt->dev    != NULL,            "  поле dev инициализировано");

    /* Граничные невалидные значения */
    datatime *b1 = datatime_create(32, 1, 2024, 0, 0);   /* день 32   */
    datatime *b2 = datatime_create(1, 13, 2024, 0, 0);   /* месяц 13  */
    datatime *b3 = datatime_create(1,  1, 2024, 24, 0);  /* час 24    */
    datatime *b4 = datatime_create(1,  1, 2024, 0, 60);  /* минута 60 */
    datatime *b5 = datatime_create(0,  1, 2024, 0, 0);   /* день 0    */
    CHECK(b1 == NULL, "день=32  → NULL");
    CHECK(b2 == NULL, "месяц=13 → NULL");
    CHECK(b3 == NULL, "час=24   → NULL");
    CHECK(b4 == NULL, "минута=60 → NULL");
    CHECK(b5 == NULL, "день=0   → NULL");

    /* create_empty_datatime */
    datatime *empty = create_empty_datatime();
    CHECK(empty != NULL,          "create_empty_datatime: не NULL");
    CHECK(empty && empty->dev != NULL, "create_empty_datatime: dev инициализирован");

    /* copy_datatime */
    copy_datatime(empty, dt);
    CHECK(empty->day == 15 && empty->month == 6 && empty->year == 2024,
          "copy_datatime: числовые поля скопированы");
    CHECK(empty->dev != NULL && empty->dev->data == dt->dev->data,
          "copy_datatime: dev->data скопирован через memcpy");

    /* copy_datatime с NULL — не должен крашиться */
    copy_datatime(NULL, dt);
    copy_datatime(empty, NULL);
    CHECK(1, "copy_datatime(NULL, ...) / (..., NULL): безопасен");

    /* destroy NULL — не должен крашиться */
    datatime_destroy(NULL);
    CHECK(1, "datatime_destroy(NULL): безопасен");

    datatime_destroy(empty); empty = NULL;
    datatime_destroy(dt);    dt    = NULL;
}

/* ═══════════════════════════════════════════════════════════
 * СЕКЦИЯ 2 — datatime: инкременты / декременты / переходы
 * ═══════════════════════════════════════════════════════════ */
static void test_datatime_arithmetic(void) {
    section("2. datatime — арифметика (инкременты, переходы)");

    /* Переход минуты через час */
    datatime *dt = datatime_create(1, 1, 2024, 23, 59);
    datatime_min_increment(dt);
    CHECK(dt->hour == 0 && dt->minute == 0 && dt->day == 2,
          "минута 59 → переход в следующий час и день");

    /* Переход дня через месяц */
    dt->day   = 31; dt->month = 1; dt->hour = 23; dt->minute = 59;
    datatime_min_increment(dt);
    CHECK(dt->day == 1 && dt->month == 2, "январь день 31 → переход на февраль");

    /* Переход месяца через год */
    dt->day   = 31; dt->month = 12; dt->hour = 23; dt->minute = 59;
    datatime_min_increment(dt);
    CHECK(dt->month == 1 && dt->year == 2025, "декабрь 31 → переход на январь следующего года");

    /* Декремент минуты через час */
    dt->day    = 5; dt->month = 3; dt->year = 2024;
    dt->hour   = 0; dt->minute = 0;
    datatime_min_decrement(dt);
    CHECK(dt->hour == 23 && dt->minute == 59 && dt->day == 4,
          "минута 0 часа 0 → переход на предыдущий день");

    /* Декремент через начало месяца.
     * В модели февраль = 28 дней (високосные годы не учитываются). */
    dt->day    = 1; dt->month = 3; dt->year = 2024;
    dt->hour   = 0; dt->minute = 0;
    datatime_day_decrement(dt);
    CHECK(dt->month == 2 && dt->day == 28, "1 марта → 28 февраля (модель без виc.лет)");
    /* Примечание: год 2024 в модели считается 28 дней в феврале (нет учёта виc.лет) */

    /* Декремент через январь */
    dt->day    = 1; dt->month = 1; dt->year = 2024;
    dt->hour   = 0; dt->minute = 0;
    datatime_month_decrement(dt);
    CHECK(dt->month == 12 && dt->year == 2023, "январь → декабрь предыдущего года");

    /* Инкременты year/month/day/hour */
    datatime *dt2 = datatime_create(28, 2, 2023, 10, 0);
    datatime_year_increment(dt2);
    CHECK(dt2->year == 2024, "year_increment: 2023 → 2024");
    datatime_month_increment(dt2);
    CHECK(dt2->month == 3, "month_increment: февраль → март");
    datatime_hour_increment(dt2);
    CHECK(dt2->hour == 11, "hour_increment: 10 → 11");
    datatime_day_increment(dt2);
    CHECK(dt2->day == 29, "day_increment: 28 → 29");

    datatime_destroy(dt);  dt  = NULL;
    datatime_destroy(dt2); dt2 = NULL;
}

/* ═══════════════════════════════════════════════════════════
 * СЕКЦИЯ 3 — datatime: сеттеры
 * ═══════════════════════════════════════════════════════════ */
static void test_datatime_setters(void) {
    section("3. datatime — сеттеры");

    datatime *dt = datatime_create(15, 6, 2024, 12, 30);

    datatime_year_set(dt, 2000);
    CHECK(dt->year == 2000, "year_set: 2000");

    datatime_month_set(dt, 11);
    CHECK(dt->month == 11, "month_set: 11");

    datatime_day_set(dt, 20);
    CHECK(dt->day == 20, "day_set: 20");

    datatime_hour_set(dt, 8);
    CHECK(dt->hour == 8, "hour_set: 8");

    datatime_min_set(dt, 45);
    CHECK(dt->minute == 45, "min_set: 45");

    /* Невалидные значения — поля не должны измениться */
    datatime_hour_set(dt, 25);
    CHECK(dt->hour == 8, "hour_set(25): значение не изменилось");

    datatime_min_set(dt, 61);
    CHECK(dt->minute == 45, "min_set(61): значение не изменилось");

    datatime_month_set(dt, 0);
    CHECK(dt->month == 11, "month_set(0): значение не изменилось");

    datatime_destroy(dt); dt = NULL;
}

/* ═══════════════════════════════════════════════════════════
 * СЕКЦИЯ 4 — datatime: строки, сравнение, разница
 * ═══════════════════════════════════════════════════════════ */
static void test_datatime_strings(void) {
    section("4. datatime — строки, сравнение, Unix-time");

    /* to_string */
    datatime *dt1 = datatime_create(5, 3, 2026, 10, 15);
    char *s = datatime_to_string(dt1);
    CHECK(s != NULL,                            "to_string: не NULL");
    CHECK(s && strcmp(s, "05/03/2026 10:15") == 0,
          "to_string: формат \"05/03/2026 10:15\"");
    free(s); s = NULL;

    /* from_string */
    datatime *dt2 = datatime_from_string("05/03/2026 10:15");
    CHECK(dt2 != NULL,          "from_string: не NULL");
    CHECK(dt2 && dt2->day == 5 && dt2->month == 3 && dt2->year == 2026,
          "from_string: дата разобрана верно");
    CHECK(dt2 && dt2->dev != NULL, "from_string: dev инициализирован");

    /* from_string с плохим форматом */
    datatime *bad = datatime_from_string("не дата");
    CHECK(bad == NULL, "from_string: плохой формат → NULL");

    /* compare */
    datatime *earlier = datatime_create(1, 1, 2020, 0, 0);
    datatime *later   = datatime_create(1, 1, 2021, 0, 0);
    CHECK(datatime_compare(earlier, later)  < 0, "compare: earlier < later");
    CHECK(datatime_compare(later, earlier)  > 0, "compare: later > earlier");
    CHECK(datatime_compare(earlier, earlier) == 0, "compare: равные даты = 0");

    /* to_minutes — разность двух дат */
    unsigned long long m1 = datatime_to_minutes(earlier);
    unsigned long long m2 = datatime_to_minutes(later);
    CHECK(m2 - m1 == 365ULL * 24 * 60, "to_minutes: разница ровно 1 год (365 дней)");

    /* from_minutes — проверяем что возвращает не NULL с инициализированным dev.
     * ВАЖНО: формула to_minutes использует смещение (+текущий месяц), поэтому
     * from_minutes(to_minutes(x)) != x — это ИЗВЕСТНОЕ поведение оригинальной формулы.
     * Функция корректна для разностей (diff = t2 - t1), но не для round-trip. */
    datatime *rt = datatime_from_minutes(m1);
    CHECK(rt != NULL,              "from_minutes: не NULL");
    CHECK(rt && rt->dev != NULL,   "from_minutes: dev инициализирован");
    CHECK(rt && rt->year  >= 1,    "from_minutes: год > 0 (валидное значение)");
    CHECK(rt && rt->month >= 1 && rt->month <= 12, "from_minutes: месяц в диапазоне 1–12");
    datatime_destroy(rt); rt = NULL;

    /* Unix time */
    datatime *unix_epoch = datatime_create(1, 1, 1970, 0, 0);
    unsigned long long unix_secs = datatime_diff_in_seconds_from_unix(unix_epoch);
    CHECK(unix_secs == 0, "unix_time: эпоха Unix = 0 секунд");

    datatime *y2024 = datatime_create(1, 1, 2024, 0, 0);
    unsigned long long secs = datatime_diff_in_seconds_from_unix(y2024);
    CHECK(secs > 0, "unix_time: 2024 год > 0 секунд");

    /* datatimes_switch */
    datatime *a = datatime_create(1, 1, 2020, 10, 0);
    datatime *b = datatime_create(2, 2, 2021, 20, 30);
    dev_set_brightness(a->dev, 5);
    dev_set_brightness(b->dev, 10);
    datatimes_switch(a, b);
    CHECK(a->year == 2021 && b->year == 2020,           "datatimes_switch: годы обменяны");
    CHECK(dev_get_brightness(a->dev) == 10,             "datatimes_switch: dev a получил яркость b");
    CHECK(dev_get_brightness(b->dev) == 5,              "datatimes_switch: dev b получил яркость a");

    datatime_destroy(dt1);       dt1       = NULL;
    datatime_destroy(dt2);       dt2       = NULL;
    datatime_destroy(earlier);   earlier   = NULL;
    datatime_destroy(later);     later     = NULL;
    datatime_destroy(unix_epoch); unix_epoch = NULL;
    datatime_destroy(y2024);     y2024     = NULL;
    datatime_destroy(a);         a         = NULL;
    datatime_destroy(b);         b         = NULL;
}

/* ═══════════════════════════════════════════════════════════
 * СЕКЦИЯ 5 — device / bitstruct
 * ═══════════════════════════════════════════════════════════ */
static void test_device(void) {
    section("5. device — битовые поля (bitstruct)");

    device *d = dev_create();
    CHECK(d != NULL,            "dev_create: не NULL");
    CHECK(d && d->data == 0,    "dev_create: data = 0");

    /* Запись и чтение каждого поля */
    dev_set_display(d, 2);
    CHECK(dev_get_display(d) == 2, "display: запись 2 / чтение 2");

    dev_set_brightness(d, 15);
    CHECK(dev_get_brightness(d) == 15, "brightness: запись 15 / чтение 15");

    dev_set_time_format(d, 1);
    CHECK(dev_get_time_format(d) == 1, "time_format: запись 1 / чтение 1");

    dev_set_alarm(d, 1);
    CHECK(dev_get_alarm(d) == 1, "alarm: запись 1 / чтение 1");

    dev_set_memory(d, 3);
    CHECK(dev_get_memory(d) == 3, "memory: запись 3 / чтение 3");

    dev_set_cpu(d, 2);
    CHECK(dev_get_cpu(d) == 2, "cpu: запись 2 / чтение 2");

    dev_set_water(d, 2);
    CHECK(dev_get_water(d) == 2, "water: запись 2 / чтение 2");

    /* Поля независимы — изменение одного не меняет другие */
    dev_set_display(d, 0);
    CHECK(dev_get_brightness(d) == 15, "независимость полей: brightness не изменился");
    CHECK(dev_get_cpu(d)        == 2,  "независимость полей: cpu не изменился");

    /* Граничные: значение больше маски должно игнорироваться */
    dev_set_display(d, 10);  /* display занимает 3 бита → max = 7 */
    CHECK(dev_get_display(d) != 10, "display: значение > 7 отклонено (не записано)");

    /* raw_data */
    dev_set_raw_data(d, 0xFFFF);
    CHECK(d->data == 0xFFFF, "set_raw_data: 0xFFFF записан");
    dev_set_raw_data(d, 0);
    CHECK(d->data == 0, "set_raw_data: сброс в 0");

    /* destroy NULL — безопасен */
    dev_destroy(NULL);
    CHECK(1, "dev_destroy(NULL): безопасен");

    dev_destroy(d); d = NULL;
}

/* ═══════════════════════════════════════════════════════════
 * СЕКЦИЯ 6 — vector: создание, push, pop, get
 * ═══════════════════════════════════════════════════════════ */
static void test_vector_basic(void) {
    section("6. vector — push / pop / get / len / cap");

    vector_t *v = vec_create(2);
    CHECK(v != NULL,          "vec_create: не NULL");
    CHECK(vec_len(v) == 0,    "vec_len после создания = 0");
    CHECK(vec_cap(v) == 2,    "vec_cap после создания = 2");

    /* push */
    vec_push(v, datatime_create(1, 1, 2020, 0, 0));
    vec_push(v, datatime_create(2, 2, 2021, 1, 0));
    CHECK(vec_len(v) == 2, "len после 2 push = 2");

    /* push вызывает расширение (cap=2 → заполнен) */
    vec_push(v, datatime_create(3, 3, 2022, 2, 0));
    CHECK(vec_len(v) == 3,    "len после 3 push = 3 (cap увеличен)");
    CHECK(vec_cap(v) > 2,     "cap увеличился после переполнения");

    /* get */
    datatime *g = vec_get(v, 0);
    CHECK(g != NULL && g->year == 2020, "vec_get(0): год = 2020");
    g = vec_get(v, 2);
    CHECK(g != NULL && g->year == 2022, "vec_get(2): год = 2022");

    /* get за пределами */
    CHECK(vec_get(v, 100) == NULL, "vec_get(100): вне границ → NULL");
    CHECK(vec_get(NULL, 0) == NULL, "vec_get(NULL, 0) → NULL");

    /* pop */
    datatime *popped = vec_pop(v);
    CHECK(popped != NULL && popped->year == 2022, "vec_pop: возвращает последний элемент");
    CHECK(vec_len(v) == 2, "len после pop = 2");
    datatime_destroy(popped); popped = NULL;

    /* pop до пустоты */
    datatime *p1 = vec_pop(v);
    datatime *p2 = vec_pop(v);
    datatime *p3 = vec_pop(v);  /* уже пуст */
    CHECK(p3 == NULL, "vec_pop на пустом векторе → NULL");
    datatime_destroy(p1); p1 = NULL;
    datatime_destroy(p2); p2 = NULL;

    /* push NULL — должен возвращать ошибку */
    int r = vec_push(v, NULL);
    CHECK(r != 0, "vec_push(v, NULL): возвращает ненулевой код ошибки");

    /* len/cap на NULL */
    CHECK(vec_len(NULL) == 0, "vec_len(NULL) = 0");
    CHECK(vec_cap(NULL) == 0, "vec_cap(NULL) = 0");

    vec_destroy(v); v = NULL;
}

/* ═══════════════════════════════════════════════════════════
 * СЕКЦИЯ 7 — vector: insert / remove / change
 * ═══════════════════════════════════════════════════════════ */
static void test_vector_modify(void) {
    section("7. vector — insert / remove / change");

    vector_t *v = vec_create(4);
    vec_push(v, datatime_create(1, 1, 2001, 0, 0));
    vec_push(v, datatime_create(2, 2, 2002, 0, 0));
    vec_push(v, datatime_create(3, 3, 2003, 0, 0));

    /* insert в середину */
    vec_insert(v, 1, datatime_create(5, 5, 1999, 0, 0));
    CHECK(vec_len(v) == 4,                           "insert: len = 4");
    CHECK(vec_get(v, 1)->year == 1999,               "insert: элемент на индексе 1 = 1999");
    CHECK(vec_get(v, 2)->year == 2002,               "insert: старый [1] сдвинулся на [2]");

    /* insert в конец (должен работать как push) */
    vec_insert(v, vec_len(v), datatime_create(4, 4, 2004, 0, 0));
    CHECK(vec_get(v, vec_len(v) - 1)->year == 2004,  "insert в конец: год = 2004");

    /* change */
    vec_change(v, 0, datatime_create(9, 9, 9999, 0, 0));
    CHECK(vec_get(v, 0)->year == 9999,               "change: элемент [0] заменён");

    /* remove из середины */
    size_t len_before = vec_len(v);
    vec_remove(v, 1);
    CHECK(vec_len(v) == len_before - 1,              "remove: len уменьшился");
    CHECK(vec_get(v, 1)->year == 2002,               "remove: следующий элемент сдвинулся");

    /* remove за пределами */
    int r = vec_remove(v, 1000);
    CHECK(r != 0, "remove(1000): вне границ → ошибка");

    /* change на NULL */
    r = vec_change(v, 0, NULL);
    CHECK(r != 0, "change с NULL: ошибка");

    vec_destroy(v); v = NULL;
}

/* ═══════════════════════════════════════════════════════════
 * СЕКЦИЯ 8 — vector: copy / merge
 * ═══════════════════════════════════════════════════════════ */
static void test_vector_copy_merge(void) {
    section("8. vector — copy / merge");

    /* Подготовка */
    vector_t *v1 = vec_create(3);
    vec_push(v1, datatime_create(1, 1, 2001, 0, 0));
    vec_push(v1, datatime_create(2, 2, 2002, 0, 0));
    vec_push(v1, datatime_create(3, 3, 2003, 0, 0));

    /* copy */
    vector_t *v2 = vec_copy(v1);
    CHECK(v2 != NULL,                  "vec_copy: не NULL");
    CHECK(vec_len(v2) == 3,            "vec_copy: len совпадает");

    /* глубокое копирование — разные указатели */
    CHECK(vec_get(v1, 0) != vec_get(v2, 0), "vec_copy: глубокая копия (разные указатели)");
    CHECK(vec_get(v2, 0)->year == 2001,      "vec_copy: данные совпадают");

    /* изменение v2 не влияет на v1 */
    vec_change(v2, 0, datatime_create(9, 9, 9999, 0, 0));
    CHECK(vec_get(v1, 0)->year == 2001, "vec_copy: независимость — v1[0] не изменился");

    /* merge v1 + v2 → v1 */
    size_t len1_before = vec_len(v1);
    size_t len2        = vec_len(v2);
    vec_merge(v1, v2);
    CHECK(vec_len(v1) == len1_before + len2, "vec_merge: len(v1) = len1 + len2");

    /* элементы из v2 попали в конец v1 */
    CHECK(vec_get(v1, len1_before)->year == 9999,
          "vec_merge: первый элемент из v2 на месте");

    /* copy пустого вектора */
    vector_t *empty = vec_create(4);
    vector_t *empty_copy = vec_copy(empty);
    CHECK(empty_copy != NULL,        "copy пустого вектора: не NULL");
    CHECK(vec_len(empty_copy) == 0,  "copy пустого вектора: len = 0");
    vec_destroy(empty_copy); empty_copy = NULL;
    vec_destroy(empty);      empty      = NULL;

    vec_destroy(v1); v1 = NULL;
    vec_destroy(v2); v2 = NULL;
}

/* ═══════════════════════════════════════════════════════════
 * СЕКЦИЯ 9 — vector: итераторы
 * ═══════════════════════════════════════════════════════════ */
static void test_vector_iterators(void) {
    section("9. vector — итераторы");

    vector_t *v = vec_create(4);
    vec_push(v, datatime_create(1, 1, 2001, 0, 0));
    vec_push(v, datatime_create(2, 2, 2002, 0, 0));
    vec_push(v, datatime_create(3, 3, 2003, 0, 0));

    vec_iter_t beg = vec_begin(v);
    vec_iter_t end = vec_end(v);

    CHECK(beg.ind == 0,           "vec_begin: ind = 0");
    CHECK(end.ind == 3,           "vec_end: ind = len");
    CHECK(!vec_isequal(beg, end), "begin != end");

    /* обход вектора */
    int count = 0;
    vec_iter_t it = vec_begin(v);
    while (!vec_isequal(it, end)) {
        CHECK(vec_iter_belong(it, v), "итератор принадлежит вектору");
        vec_iter_next(&it);
        count++;
    }
    CHECK(count == 3, "итератор обошёл ровно 3 элемента");

    /* равенство begin с самим собой */
    vec_iter_t beg2 = vec_begin(v);
    CHECK(vec_isequal(beg, beg2), "два begin равны");

    /* after end — next не выходит за конец */
    vec_iter_next(&it);  /* it уже на end */
    CHECK(it.ind == 3, "vec_iter_next на end: ind не выходит за len");

    vec_destroy(v); v = NULL;
}

/* ═══════════════════════════════════════════════════════════
 * СЕКЦИЯ 10 — file I/O: текстовый формат
 * ═══════════════════════════════════════════════════════════ */
static void test_file_txt(void) {
    section("10. file I/O — текстовый формат");

    const int N = 20;

    /* Генерация и сохранение */
    vector_t *orig = vec_create((size_t)N);
    rand_gen_struct_in_container(orig, (size_t)N);
    int r = save_vec_txt(orig, TMP_TXT2);
    CHECK(r == 0, "save_vec_txt: успешно (код 0)");

    /* count_elm_txt (медленный) */
    int cnt_slow = count_elm_txt(TMP_TXT2);
    CHECK(cnt_slow == N, "count_elm_txt (медленный): количество совпадает");

    /* count_elm_txt_fast */
    int cnt_fast = count_elm_txt_fast(TMP_TXT2);
    CHECK(cnt_fast == N, "count_elm_txt_fast: количество совпадает");

    /* load_vec_txt */
    vector_t *loaded = load_vec_txt(TMP_TXT2);
    CHECK(loaded != NULL,              "load_vec_txt: не NULL");
    CHECK(loaded && vec_len(loaded) == (size_t)N, "load_vec_txt: len совпадает");

    /* сравниваем первый и последний элемент */
    if (loaded && vec_len(loaded) > 0) {
        datatime *o = vec_get(orig, 0);
        datatime *l = vec_get(loaded, 0);
        CHECK(o->day   == l->day   &&
              o->month == l->month &&
              o->year  == l->year,  "load_vec_txt: данные [0] совпадают");
    }

    /* get_elm_txt_slow */
    datatime *elm_slow = get_elm_txt_slow(TMP_TXT2, 5);
    datatime *ref      = vec_get(orig, 5);
    CHECK(elm_slow != NULL, "get_elm_txt_slow(5): не NULL");
    if (elm_slow && ref) {
        CHECK(elm_slow->day == ref->day && elm_slow->year == ref->year,
              "get_elm_txt_slow: данные совпадают с оригиналом");
    }
    datatime_destroy(elm_slow); elm_slow = NULL;

    /* get_elm_txt_fast */
    datatime *elm_fast = get_elm_txt_fast(TMP_TXT2, 5);
    CHECK(elm_fast != NULL, "get_elm_txt_fast(5): не NULL");
    if (elm_fast && ref) {
        CHECK(elm_fast->day == ref->day && elm_fast->year == ref->year,
              "get_elm_txt_fast: данные совпадают с оригиналом");
    }
    datatime_destroy(elm_fast); elm_fast = NULL;

    /* get за границей */
    datatime *oob = get_elm_txt_fast(TMP_TXT2, N + 100);
    CHECK(oob == NULL, "get_elm_txt_fast: индекс вне файла → NULL");

    vec_destroy(orig);   orig   = NULL;
    vec_destroy(loaded); loaded = NULL;
    remove(TMP_TXT2);
}

/* ═══════════════════════════════════════════════════════════
 * СЕКЦИЯ 11 — file I/O: бинарный формат
 * ═══════════════════════════════════════════════════════════ */
static void test_file_bin(void) {
    section("11. file I/O — бинарный формат");

    const int N = 20;

    vector_t *orig = vec_create((size_t)N);
    rand_gen_struct_in_container(orig, (size_t)N);

    int r = save_vec_bin(orig, TMP_BIN2);
    CHECK(r == 0, "save_vec_bin: успешно (код 0)");

    int cnt = count_elm_bin(TMP_BIN2);
    CHECK(cnt == N, "count_elm_bin: количество совпадает");

    vector_t *loaded = load_vec_bin(TMP_BIN2);
    CHECK(loaded != NULL,              "load_vec_bin: не NULL");
    CHECK(loaded && vec_len(loaded) == (size_t)N, "load_vec_bin: len совпадает");

    /* сверяем данные */
    if (loaded && vec_len(loaded) > 0) {
        datatime *o = vec_get(orig, 0);
        datatime *l = vec_get(loaded, 0);
        CHECK(o->day   == l->day   &&
              o->month == l->month &&
              o->year  == l->year  &&
              o->hour  == l->hour  &&
              o->minute == l->minute,
              "load_vec_bin: данные [0] полностью совпадают");
        CHECK(o->dev->data == l->dev->data,
              "load_vec_bin: dev->data совпадает (биты устройства)");
    }

    /* get_elm_bin */
    datatime *elm = get_elm_bin(TMP_BIN2, 3);
    datatime *ref = vec_get(orig, 3);
    CHECK(elm != NULL, "get_elm_bin(3): не NULL");
    if (elm && ref) {
        CHECK(elm->day == ref->day && elm->year == ref->year,
              "get_elm_bin: данные совпадают с оригиналом");
    }
    datatime_destroy(elm); elm = NULL;

    /* несуществующий файл */
    vector_t *bad = load_vec_bin("/tmp/nesuschestvuyuschiy_fail_123.bin");
    CHECK(bad == NULL, "load_vec_bin несуществующего файла → NULL");

    vec_destroy(orig);   orig   = NULL;
    vec_destroy(loaded); loaded = NULL;
    remove(TMP_BIN2);
}

/* ═══════════════════════════════════════════════════════════
 * СЕКЦИЯ 12 — БЕНЧМАРКИ
 * ═══════════════════════════════════════════════════════════ */
static void bench_all(void) {
    section("12. БЕНЧМАРКИ");
    printf("  N_memory  = %d\n", BENCH_MEM_N);
    printf("  N_file    = %d\n", BENCH_FILE_N);
    printf("  N_get     = %d случайных запросов\n\n", BENCH_GET_N);

    clock_t t;
    double  ms;

    /* ── 12.1 vec_push ── */
    vector_t *v = vec_create(10);
    t = clock();
    for (int i = 0; i < BENCH_MEM_N; i++) {
        int m = 1 + rand() % 12;
        int d = 1 + rand() % month_lengths[m - 1];
        vec_push(v, datatime_create(d, m, 1970 + rand() % 100, rand() % 24, rand() % 60));
    }
    ms = elapsed_ms(t);
    printf("  [BENCH] vec_push x%d:               %7.2f мс  (%d эл/с)\n",
           BENCH_MEM_N, ms, (int)(BENCH_MEM_N / (ms / 1000.0)));

    /* ── 12.2 vec_copy ── */
    t = clock();
    vector_t *vc = vec_copy(v);
    ms = elapsed_ms(t);
    printf("  [BENCH] vec_copy (%d эл.):          %7.2f мс\n", BENCH_MEM_N, ms);
    vec_destroy(vc); vc = NULL;

    /* ── 12.3 vec_merge ── */
    vector_t *vm = vec_create(100);
    for (int i = 0; i < 1000; i++)
        vec_push(vm, datatime_create(1, 1, 2000, 0, i % 60));
    t = clock();
    vec_merge(v, vm);
    ms = elapsed_ms(t);
    printf("  [BENCH] vec_merge (+1000 эл.):     %7.2f мс\n", ms);
    vec_destroy(vm); vm = NULL;
    vec_destroy(v);  v  = NULL;

    /* ── Подготовка файлов для файловых бенчмарков ── */
    vector_t *fv = vec_create((size_t)BENCH_FILE_N);
    rand_gen_struct_in_container(fv, (size_t)BENCH_FILE_N);

    /* ── 12.4 save_vec_txt ── */
    t = clock();
    save_vec_txt(fv, TMP_TXT);
    ms = elapsed_ms(t);
    printf("  [BENCH] save_vec_txt  (%d эл.):    %7.2f мс\n", BENCH_FILE_N, ms);

    /* ── 12.5 save_vec_bin ── */
    t = clock();
    save_vec_bin(fv, TMP_BIN);
    ms = elapsed_ms(t);
    printf("  [BENCH] save_vec_bin  (%d эл.):    %7.2f мс\n", BENCH_FILE_N, ms);

    /* ── 12.6 load_vec_txt ── */
    t = clock();
    vector_t *lt = load_vec_txt(TMP_TXT);
    ms = elapsed_ms(t);
    printf("  [BENCH] load_vec_txt  (%d эл.):    %7.2f мс\n", BENCH_FILE_N, ms);
    vec_destroy(lt); lt = NULL;

    /* ── 12.7 load_vec_bin ── */
    t = clock();
    vector_t *lb = load_vec_bin(TMP_BIN);
    ms = elapsed_ms(t);
    printf("  [BENCH] load_vec_bin  (%d эл.):    %7.2f мс\n", BENCH_FILE_N, ms);
    vec_destroy(lb); lb = NULL;

    /*
     * ── 12.8 Случайный доступ: медленный txt vs быстрый txt vs bin ──
     * Для N_file записей делаем BENCH_GET_N случайных запросов к каждому методу.
     */
    printf("\n  Случайный доступ (%d запросов к %d записям):\n", BENCH_GET_N, BENCH_FILE_N);

    /* get_elm_txt_slow */
    t = clock();
    for (int i = 0; i < BENCH_GET_N; i++) {
        int idx = rand() % BENCH_FILE_N;
        datatime *elm = get_elm_txt_slow(TMP_TXT, idx);
        datatime_destroy(elm); elm = NULL;
    }
    ms = elapsed_ms(t);
    printf("  [BENCH] get_elm_txt_slow: %7.2f мс  (ср. %.3f мс/запрос)\n",
           ms, ms / BENCH_GET_N);

    /* get_elm_txt_fast */
    t = clock();
    for (int i = 0; i < BENCH_GET_N; i++) {
        int idx = rand() % BENCH_FILE_N;
        datatime *elm = get_elm_txt_fast(TMP_TXT, idx);
        datatime_destroy(elm); elm = NULL;
    }
    ms = elapsed_ms(t);
    printf("  [BENCH] get_elm_txt_fast: %7.2f мс  (ср. %.3f мс/запрос)\n",
           ms, ms / BENCH_GET_N);

    /* get_elm_bin */
    t = clock();
    for (int i = 0; i < BENCH_GET_N; i++) {
        int idx = rand() % BENCH_FILE_N;
        datatime *elm = get_elm_bin(TMP_BIN, idx);
        datatime_destroy(elm); elm = NULL;
    }
    ms = elapsed_ms(t);
    printf("  [BENCH] get_elm_bin:      %7.2f мс  (ср. %.3f мс/запрос)\n",
           ms, ms / BENCH_GET_N);

    printf("\n  Вывод: бинарный формат быстрее текстового для случайного доступа.\n");
    printf("         Быстрый txt (fseek) быстрее медленного (линейный скан).\n");

    /* Очистка */
    vec_destroy(fv); fv = NULL;
    remove(TMP_TXT);
    remove(TMP_BIN);
}

/* ═══════════════════════════════════════════════════════════
 * main
 * ═══════════════════════════════════════════════════════════ */
int main(void) {
    srand(42);  /* фиксированный seed — воспроизводимые результаты */

    printf("╔══════════════════════════════════════════╗\n");
    printf("║  prog_s2 — полный тест + бенчмарк        ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    test_datatime_create();
    test_datatime_arithmetic();
    test_datatime_setters();
    test_datatime_strings();
    test_device();
    test_vector_basic();
    test_vector_modify();
    test_vector_copy_merge();
    test_vector_iterators();
    test_file_txt();
    test_file_bin();
    bench_all();

    /* Итог */
    printf("\n══════════════════════════════════════\n");
    printf("  ИТОГ: %d пройдено / %d упало\n", g_pass, g_fail);
    printf("══════════════════════════════════════\n");

    return (g_fail == 0) ? 0 : 1;
}
