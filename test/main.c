/*
 * test/main.c — полный тестовый набор для lab_1 – lab_4
 *
 * Сборка:  make
 * Запуск:  make run
 * Утечки:  make memcheck   (leaks на macOS, valgrind на Linux)
 *
 * Возвращает 0 если все тесты прошли, 1 если есть провалы.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../lab_4/fiovector.h"

/* ══════════════════════════════════════════════════════════════
   Мини-фреймворк
   ══════════════════════════════════════════════════════════════ */

static int s_total = 0, s_pass = 0, s_fail = 0;

#define ASSERT(cond, msg) do { \
    s_total++; \
    if (cond) { s_pass++; printf("  [PASS] %s\n", msg); } \
    else      { s_fail++; printf("  [FAIL] %s  <- строка %d\n", msg, __LINE__); } \
} while(0)

#define SECTION(name) printf("\n=== %s ===\n", name)

static struct timespec _ts_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}
static double _ts_ms(struct timespec t0, struct timespec t1) {
    return (t1.tv_sec - t0.tv_sec) * 1000.0 +
           (t1.tv_nsec - t0.tv_nsec) / 1e6;
}

#define TMP_TXT "_test_tmp.txt"
#define TMP_BIN "_test_tmp.bin"

/* ══════════════════════════════════════════════════════════════
   LAB 1/2 — datatime
   ══════════════════════════════════════════════════════════════ */

static void test_datatime_create_destroy(void) {
    SECTION("datatime: create / destroy");

    datatime *dt = datatime_create(15, 6, 2025, 14, 30);
    ASSERT(dt != NULL, "create(15,6,2025,14,30) не NULL");
    ASSERT(dt->day   == 15,   "day = 15");
    ASSERT(dt->month == 6,    "month = 6");
    ASSERT(dt->year  == 2025, "year = 2025");
    ASSERT(dt->hour  == 14,   "hour = 14");
    ASSERT(dt->minute == 30,  "minute = 30");
    ASSERT(dt->dev   != NULL, "dev создан вместе с datatime");
    datatime_destroy(dt);

    /* destroy(NULL) не должен падать */
    datatime_destroy(NULL);
    ASSERT(1, "destroy(NULL) не падает");

    /* БАГИ НИЖЕ — ожидаются FAIL ↓ */

    /*
     * BUG-1: create_empty_datatime() вызывает datatime_create(0,0,0,0,0).
     * check_time_format отклоняет month=0 и day=0 → возвращает NULL.
     * Функция задумана как «создать пустой», но реально не работает.
     */
    datatime *empty = create_empty_datatime();
    ASSERT(empty != NULL, "create_empty_datatime() не NULL  [BUG: всегда NULL]");
    if (empty) datatime_destroy(empty);

    /*
     * BUG-2: datatime_print(NULL) — вторая проверка dt->dev != NULL
     * выполняется без охраны, если dt == NULL → crash/UB.
     * Тест ПРОПУЩЕН намеренно чтобы не падал процесс.
     */
    printf("  [SKIP] datatime_print(NULL) — crash (BUG: вторая ветка без проверки dt)\n");

    /* невалидные аргументы */
    datatime *bad = datatime_create(32, 1, 2000, 0, 0); /* день 32 */
    ASSERT(bad == NULL, "create(день=32) = NULL (валидация работает)");

    datatime *bad2 = datatime_create(1, 13, 2000, 0, 0); /* месяц 13 */
    ASSERT(bad2 == NULL, "create(месяц=13) = NULL");
}

static void test_datatime_compare(void) {
    SECTION("datatime: compare");

    datatime *a = datatime_create(1, 1, 2020, 0, 0);
    datatime *b = datatime_create(1, 1, 2021, 0, 0);
    datatime *c = datatime_create(1, 1, 2020, 0, 0);
    datatime *d = datatime_create(1, 1, 2020, 0, 1);

    ASSERT(datatime_compare(a, b) < 0,  "a(2020) < b(2021) → отрицательное");
    ASSERT(datatime_compare(b, a) > 0,  "b(2021) > a(2020) → положительное");
    ASSERT(datatime_compare(a, c) == 0, "a == c (одинаковые) → 0");
    ASSERT(datatime_compare(a, d) < 0,  "00:00 < 00:01");
    ASSERT(datatime_compare(d, a) > 0,  "00:01 > 00:00");

    datatime_destroy(a); datatime_destroy(b);
    datatime_destroy(c); datatime_destroy(d);
}

static void test_datatime_increment(void) {
    SECTION("datatime: increment — переполнения");

    /* min: обычный */
    datatime *dt = datatime_create(1, 1, 2000, 10, 0);
    datatime_min_increment(dt);
    ASSERT(dt->minute == 1, "min_increment: 0 → 1");

    /* min → hour overflow */
    datatime_min_set(dt, 59);
    dt->hour = 10;
    datatime_min_increment(dt);
    ASSERT(dt->minute == 0,  "min 59 → 0 при overflow");
    ASSERT(dt->hour   == 11, "min overflow → hour++");

    /* hour → day overflow */
    datatime_hour_set(dt, 23);
    dt->day = 1;
    datatime_hour_increment(dt);
    ASSERT(dt->hour == 0, "hour 23 → 0 при overflow");
    ASSERT(dt->day  == 2, "hour overflow → day++");

    /* конец февраля (не високосный 2001): 28 фев + 1 мин = 1 марта */
    datatime_year_set(dt, 2001);
    datatime_month_set(dt, 2);
    datatime_day_set(dt, 28);
    datatime_hour_set(dt, 23);
    datatime_min_set(dt, 59);
    datatime_min_increment(dt);
    ASSERT(dt->month == 3, "конец февраля 2001 → месяц 3");
    ASSERT(dt->day   == 1, "конец февраля 2001 → день 1");

    /* декабрь → январь следующего года */
    datatime_month_set(dt, 12);
    datatime_day_set(dt, 31);
    datatime_month_increment(dt);
    ASSERT(dt->month == 1,    "декабрь month_increment → январь");
    ASSERT(dt->year  == 2002, "декабрь month_increment → год++");

    datatime_destroy(dt);
}

static void test_datatime_decrement(void) {
    SECTION("datatime: decrement — переполнения");

    /* 1 марта 2001, 00:00 - 1 мин = 28 фев 2001, 23:59 */
    datatime *dt = datatime_create(1, 3, 2001, 0, 0);
    datatime_min_decrement(dt);
    ASSERT(dt->minute == 59, "min_decrement: 0 → 59");
    ASSERT(dt->hour   == 23, "min underflow → hour = 23");
    ASSERT(dt->day    == 28, "hour underflow → день 28 (конец февраля 2001)");
    ASSERT(dt->month  == 2,  "day underflow → месяц 2");
    datatime_destroy(dt);

    /* year_decrement */
    datatime *dt2 = datatime_create(1, 1, 2000, 0, 0);
    datatime_year_decrement(dt2);
    ASSERT(dt2->year == 1999, "year_decrement: 2000 → 1999");
    datatime_destroy(dt2);

    /* month_decrement через год */
    datatime *dt3 = datatime_create(1, 1, 2000, 0, 0);
    datatime_month_decrement(dt3);
    ASSERT(dt3->month == 12,  "month_decrement: январь → декабрь");
    ASSERT(dt3->year  == 1999,"month_decrement: год уменьшился");
    datatime_destroy(dt3);
}

static void test_datatime_setters(void) {
    SECTION("datatime: setters");

    datatime *dt = datatime_create(1, 1, 2000, 0, 0);
    datatime_day_set(dt, 25);
    datatime_month_set(dt, 12);
    datatime_year_set(dt, 2024);
    datatime_hour_set(dt, 18);
    datatime_min_set(dt, 45);

    ASSERT(dt->day    == 25,   "day_set(25)");
    ASSERT(dt->month  == 12,   "month_set(12)");
    ASSERT(dt->year   == 2024, "year_set(2024)");
    ASSERT(dt->hour   == 18,   "hour_set(18)");
    ASSERT(dt->minute == 45,   "min_set(45)");

    /* невалидные значения игнорируются */
    datatime_min_set(dt, 99);
    ASSERT(dt->minute == 45, "min_set(99) — значение не изменилось");
    datatime_hour_set(dt, 25);
    ASSERT(dt->hour == 18,   "hour_set(25) — значение не изменилось");

    datatime_destroy(dt);
}

static void test_datatime_string(void) {
    SECTION("datatime: to_string / from_string");

    datatime *orig = datatime_create(5, 7, 2023, 9, 3);
    char *s = datatime_to_string(orig);
    ASSERT(s != NULL, "to_string не NULL");
    ASSERT(strcmp(s, "05/07/2023 09:03") == 0, "формат DD/MM/YYYY HH:MM");

    datatime *parsed = datatime_from_string(s);
    ASSERT(parsed != NULL, "from_string не NULL");
    ASSERT(datatime_compare(orig, parsed) == 0, "roundtrip строки: orig == parsed");

    free(s);
    datatime_destroy(orig);
    datatime_destroy(parsed);

    /* невалидная строка возвращает NULL */
    datatime *bad = datatime_from_string("не дата");
    ASSERT(bad == NULL, "from_string(мусор) = NULL");

    /* NULL на входе */
    ASSERT(datatime_from_string(NULL) == NULL, "from_string(NULL) = NULL");
    ASSERT(datatime_to_string(NULL)   == NULL, "to_string(NULL) = NULL");
}

static void test_datatime_minutes(void) {
    SECTION("datatime: to_minutes / from_minutes");

    /*
     * BUG-3: datatime_from_minutes() не вызывает dev_create() и не
     * обнуляет dt->dev. При destroy() вызывается free(мусор) → crash/UB.
     * Workaround в тестах: вручную зануляем dev перед уничтожением.
     *
     * BUG-4: roundtrip to_minutes/from_minutes даёт неверный результат:
     * to_minutes добавляет лишний месяц через for-loop (m < dt->month),
     * что смещает обратное преобразование на ~1 год и ~1 месяц.
     */

    /* from_minutes(90) = год=1, месяц=1, день=1, час=1, мин=30 */
    datatime *interval = datatime_from_minutes(90);
    ASSERT(interval != NULL, "from_minutes(90) не NULL");
    if (interval) {
        ASSERT(interval->hour == 1 && interval->minute == 30,
               "from_minutes(90): час=1, мин=30 (корректно для малых значений)");
        interval->dev = NULL; /* workaround BUG-3 */
        datatime_destroy(interval);
    }

    /* roundtrip — ожидаем FAIL из-за BUG-4 */
    datatime *orig = datatime_create(15, 6, 2025, 14, 30);
    unsigned long long mins = datatime_to_minutes(orig);
    ASSERT(mins > 0, "to_minutes > 0");

    datatime *back = datatime_from_minutes(mins);
    ASSERT(back != NULL, "from_minutes(to_minutes(orig)) не NULL");
    if (back) {
        ASSERT(datatime_compare(orig, back) == 0,
               "roundtrip to_minutes/from_minutes  [BUG-4: ожидается FAIL]");
        back->dev = NULL; /* workaround BUG-3 */
        datatime_destroy(back);
    }
    datatime_destroy(orig);

    /*
     * BUG-5: datatime_diff_in_seconds_from_unix() внутри вызывает
     * datatime_from_minutes() и затем datatime_destroy() → crash.
     * Тест ПРОПУЩЕН намеренно.
     */
    printf("  [SKIP] datatime_diff_in_seconds_from_unix — crash (BUG-5: использует from_minutes)\n");
}

static void test_datatime_copy(void) {
    SECTION("datatime: copy_datatime");

    datatime *src = datatime_create(15, 8, 2024, 12, 0);
    dev_set_brightness(src->dev, 7);

    datatime *dst = datatime_create(1, 1, 2000, 0, 0);
    copy_datatime(dst, src);

    ASSERT(datatime_compare(src, dst) == 0, "copy: даты совпадают");
    ASSERT(dev_get_brightness(dst->dev) == 7, "copy: dev->data скопирован");

    /* глубокая независимость — числовые поля */
    dst->day = 99;
    ASSERT(src->day == 15, "copy: изменение dst не влияет на src");

    /* глубокая независимость — dev */
    dev_set_brightness(dst->dev, 0);
    ASSERT(dev_get_brightness(src->dev) == 7, "copy: изменение dst->dev не влияет на src->dev");

    datatime_destroy(src);
    datatime_destroy(dst);
}

/* ══════════════════════════════════════════════════════════════
   LAB 2 — device (bitstruct)
   ══════════════════════════════════════════════════════════════ */

static void test_device(void) {
    SECTION("device (bitstruct): set / get всех полей");

    device *d = dev_create();
    ASSERT(d != NULL, "dev_create() не NULL");
    ASSERT(d->data == 0, "новое устройство: data = 0");

    dev_set_display(d, 2);
    ASSERT(dev_get_display(d) == 2, "display: set(2) → get == 2");

    dev_set_brightness(d, 15);
    ASSERT(dev_get_brightness(d) == 15, "brightness: set(15) → get == 15");

    dev_set_time_format(d, 1);
    ASSERT(dev_get_time_format(d) == 1, "time_format: set(1) → get == 1");

    dev_set_alarm(d, 1);
    ASSERT(dev_get_alarm(d) == 1, "alarm: set(1) → get == 1");

    dev_set_memory(d, 3);
    ASSERT(dev_get_memory(d) == 3, "memory: set(3) → get == 3");

    dev_set_cpu(d, 2);
    ASSERT(dev_get_cpu(d) == 2, "cpu: set(2) → get == 2");

    dev_set_water(d, 2);
    ASSERT(dev_get_water(d) == 2, "water: set(2) → get == 2");

    /* поля не затирают друг друга */
    ASSERT(dev_get_display(d)    == 2,  "display не изменился после прочих set");
    ASSERT(dev_get_brightness(d) == 15, "brightness не изменился");
    ASSERT(dev_get_cpu(d)        == 2,  "cpu не изменился");

    /* raw data roundtrip */
    uint16_t raw = d->data;
    device *d2 = dev_create();
    dev_set_raw_data(d2, raw);
    ASSERT(dev_get_display(d2)    == 2,  "raw roundtrip: display");
    ASSERT(dev_get_brightness(d2) == 15, "raw roundtrip: brightness");
    ASSERT(dev_get_cpu(d2)        == 2,  "raw roundtrip: cpu");
    ASSERT(dev_get_water(d2)      == 2,  "raw roundtrip: water");

    /* граничные значения — выход за диапазон игнорируется */
    dev_set_brightness(d, 16); /* max=15, должен быть проигнорирован */
    ASSERT(dev_get_brightness(d) == 15, "brightness: set(16) игнорируется");

    dev_destroy(d);
    dev_destroy(d2);
    dev_destroy(NULL); /* не должен падать */
    ASSERT(1, "dev_destroy(NULL) не падает");
}

/* ══════════════════════════════════════════════════════════════
   LAB 3 — vector
   ══════════════════════════════════════════════════════════════ */

static void test_vector_basic(void) {
    SECTION("vector: create / push / get / len / cap");

    vector_t *v = vec_create(4);
    ASSERT(v != NULL,           "vec_create(4) не NULL");
    ASSERT(vec_len(v) == 0,     "начальная длина = 0");
    ASSERT(vec_cap(v) == 4,     "начальная ёмкость = 4");

    for (int i = 1; i <= 4; i++)
        vec_push(v, datatime_create(i, 1, 2000, 0, 0));
    ASSERT(vec_len(v) == 4, "после 4 push: len = 4");

    /* push за пределы ёмкости — должен расшириться */
    vec_push(v, datatime_create(5, 1, 2000, 0, 0));
    ASSERT(vec_len(v) == 5,     "после 5-го push: len = 5");
    ASSERT(vec_cap(v) > 4,      "ёмкость выросла после overflow");

    ASSERT(vec_get(v, 0)->day == 1, "vec_get(0)->day == 1");
    ASSERT(vec_get(v, 2)->day == 3, "vec_get(2)->day == 3");
    ASSERT(vec_get(v, 4)->day == 5, "vec_get(4)->day == 5");
    ASSERT(vec_get(v, 100) == NULL, "vec_get(out-of-bounds) = NULL");

    vec_destroy(v);
}

static void test_vector_pop(void) {
    SECTION("vector: pop");

    vector_t *v = vec_create(4);
    for (int i = 1; i <= 3; i++)
        vec_push(v, datatime_create(i, 1, 2000, 0, 0));

    datatime *popped = vec_pop(v);
    ASSERT(popped != NULL,      "pop возвращает не NULL");
    ASSERT(popped->day == 3,    "pop вернул последний элемент (day=3)");
    ASSERT(vec_len(v) == 2,     "после pop: len = 2");
    datatime_destroy(popped);

    vec_destroy(v);

    /* pop из пустого вектора */
    vector_t *empty = vec_create(1);
    ASSERT(vec_pop(empty) == NULL, "pop пустого = NULL");
    vec_destroy(empty);
}

static void test_vector_insert_remove(void) {
    SECTION("vector: insert / remove");

    vector_t *v = vec_create(4);
    for (int i = 1; i <= 4; i++)
        vec_push(v, datatime_create(i, 1, 2000, 0, 0));
    /* [1, 2, 3, 4] */

    /* insert в начало */
    vec_insert(v, 0, datatime_create(29, 1, 2000, 0, 0));
    ASSERT(vec_get(v, 0)->day == 29, "insert(0): новый элемент на позиции 0");
    ASSERT(vec_get(v, 1)->day == 1,  "insert(0): старый [0] сдвинулся на [1]");
    /* [29, 1, 2, 3, 4] */

    /* insert в середину */
    vec_insert(v, 2, datatime_create(20, 1, 2000, 0, 0));
    ASSERT(vec_get(v, 2)->day == 20, "insert(2): элемент в середине");
    ASSERT(vec_get(v, 3)->day == 2,  "insert(2): бывший [2] сдвинулся на [3]");
    /* [99, 1, 55, 2, 3, 4] */

    /* remove начала */
    vec_remove(v, 0);
    ASSERT(vec_get(v, 0)->day == 1, "remove(0): следующий стал первым");
    /* [1, 55, 2, 3, 4] */

    /* remove конца */
    size_t prev_len = vec_len(v);
    vec_remove(v, vec_len(v) - 1);
    ASSERT(vec_len(v) == prev_len - 1, "remove(последний): len--");

    /* remove за пределами */
    ASSERT(vec_remove(v, 9999) != 0, "remove(out-of-bounds) != 0");

    vec_destroy(v);
}

static void test_vector_change(void) {
    SECTION("vector: change");

    vector_t *v = vec_create(2);
    vec_push(v, datatime_create(1, 1, 2000, 0, 0));
    vec_push(v, datatime_create(2, 1, 2000, 0, 0));

    vec_change(v, 0, datatime_create(25, 1, 2000, 0, 0));
    ASSERT(vec_get(v, 0)->day == 25, "change(0): новое значение");
    ASSERT(vec_get(v, 1)->day == 2,  "change(0): другие элементы не тронуты");

    /* out-of-bounds: change не берёт владение при ошибке — нужно уничтожать вручную */
    {
        datatime *tmp = datatime_create(1, 1, 2000, 0, 0);
        int rc = vec_change(v, 99, tmp);
        ASSERT(rc != 0, "change(out-of-bounds) != 0");
        datatime_destroy(tmp); /* vec_change не взял владение при ошибке */
    }

    vec_destroy(v);
}

static void test_vector_copy(void) {
    SECTION("vector: vec_copy (глубокая копия)");

    vector_t *orig = vec_create(3);
    for (int i = 1; i <= 3; i++)
        vec_push(orig, datatime_create(i, i, 2000, i, 0));

    dev_set_brightness(vec_get(orig, 0)->dev, 5);

    vector_t *copy = vec_copy(orig);
    ASSERT(copy != NULL,            "vec_copy не NULL");
    ASSERT(vec_len(copy) == 3,      "copy: len = 3");
    ASSERT(datatime_compare(vec_get(orig, 0), vec_get(copy, 0)) == 0,
           "copy: элементы совпадают");
    ASSERT(dev_get_brightness(vec_get(copy, 0)->dev) == 5,
           "copy: dev->data скопирован");

    /* независимость: изменяем copy, orig не меняется */
    vec_get(copy, 0)->day = 99;
    ASSERT(vec_get(orig, 0)->day == 1,
           "copy: изменение копии не влияет на оригинал");

    dev_set_brightness(vec_get(copy, 0)->dev, 0);
    ASSERT(dev_get_brightness(vec_get(orig, 0)->dev) == 5,
           "copy: изменение dev копии не влияет на оригинал");

    vec_destroy(orig);
    vec_destroy(copy);
}

static void test_vector_merge(void) {
    SECTION("vector: vec_merge");

    vector_t *v1 = vec_create(2);
    vector_t *v2 = vec_create(2);
    vec_push(v1, datatime_create(1, 1, 2000, 0, 0));
    vec_push(v1, datatime_create(2, 1, 2000, 0, 0));
    vec_push(v2, datatime_create(3, 1, 2000, 0, 0));
    vec_push(v2, datatime_create(4, 1, 2000, 0, 0));

    int rc = vec_merge(v1, v2);
    ASSERT(rc == 0,               "merge: возвращает 0");
    ASSERT(vec_len(v1) == 4,      "merge: v1.len = 4");
    ASSERT(vec_get(v1, 2)->day == 3, "merge: v1[2] = первый из v2");
    ASSERT(vec_get(v1, 3)->day == 4, "merge: v1[3] = второй из v2");
    ASSERT(vec_len(v2) == 2,      "merge: v2 не изменился");

    /* независимость: изменяем копию в v1, v2 не меняется */
    vec_get(v1, 2)->day = 99;
    ASSERT(vec_get(v2, 0)->day == 3, "merge: изменение в v1 не влияет на v2");

    vec_destroy(v1);
    vec_destroy(v2);
}

static void test_vector_iterators(void) {
    SECTION("vector: iterators");

    vector_t *v = vec_create(5);
    for (int i = 1; i <= 5; i++)
        vec_push(v, datatime_create(i, 1, 2000, 0, 0));

    vec_iter_t it  = vec_begin(v);
    vec_iter_t end = vec_end(v);

    int count = 0;
    int days_sum = 0;
    while (!vec_isequal(it, end)) {
        ASSERT(vec_iter_belong(it, v), "итератор принадлежит вектору");
        days_sum += vec_get(v, it.ind)->day;
        count++;
        vec_iter_next(&it);
    }
    ASSERT(count == 5,      "итератор прошёл все 5 элементов");
    ASSERT(days_sum == 15,  "сумма day через итератор = 1+2+3+4+5 = 15");
    ASSERT(vec_isequal(it, end), "после прохода it == end");

    /* итератор чужого вектора */
    vector_t *v2 = vec_create(1);
    vec_push(v2, datatime_create(1, 1, 2000, 0, 0));
    vec_iter_t other = vec_begin(v2);
    ASSERT(!vec_iter_belong(other, v), "итератор v2 не принадлежит v");
    ASSERT(!vec_isequal(other, it),    "итераторы разных векторов не равны");
    vec_destroy(v2);

    vec_destroy(v);
}

static void test_vector_null_safety(void) {
    SECTION("vector: NULL-безопасность");

    {
        vector_t *tmp = vec_create(0);
        ASSERT(tmp != NULL, "vec_create(0) не NULL (корректирует capacity до 1)");
        ASSERT(vec_cap(tmp) >= 1, "vec_create(0): cap >= 1 после коррекции");
        vec_destroy(tmp);
    }

    ASSERT(vec_len(NULL) == 0,       "vec_len(NULL) = 0");
    ASSERT(vec_cap(NULL) == 0,       "vec_cap(NULL) = 0");
    ASSERT(vec_get(NULL, 0) == NULL, "vec_get(NULL, 0) = NULL");
    ASSERT(vec_pop(NULL) == NULL,    "vec_pop(NULL) = NULL");
    ASSERT(vec_push(NULL, NULL) != 0,"vec_push(NULL, NULL) = ошибка");
    ASSERT(vec_copy(NULL) == NULL,   "vec_copy(NULL) = NULL");
    ASSERT(vec_merge(NULL, NULL) != 0,"vec_merge(NULL, NULL) = ошибка");
    vec_destroy(NULL);
    ASSERT(1, "vec_destroy(NULL) не падает");
}

/* ══════════════════════════════════════════════════════════════
   LAB 4 — fiovector
   ══════════════════════════════════════════════════════════════ */

static void test_fio_txt(void) {
    SECTION("fiovector: text save / load / get / count");

    srand(42);
    vector_t *orig = vec_create(10);
    int rc = rand_gen_struct_in_container(orig, 10);
    ASSERT(rc == 0,                  "rand_gen: возвращает 0");
    ASSERT(vec_len(orig) == 10,      "rand_gen: 10 элементов");

    rc = save_vec_txt(orig, TMP_TXT);
    ASSERT(rc == 0, "save_vec_txt: возвращает 0");

    /* подсчёт двумя способами */
    int cnt_slow = count_elm_txt(TMP_TXT);
    int cnt_fast = count_elm_txt_fast(TMP_TXT);
    ASSERT(cnt_slow == 10, "count_elm_txt (медленный) = 10");
    ASSERT(cnt_fast == 10, "count_elm_txt_fast = 10");
    ASSERT(cnt_slow == cnt_fast, "slow == fast по количеству");

    /* загрузка */
    vector_t *loaded = load_vec_txt(TMP_TXT);
    ASSERT(loaded != NULL,               "load_vec_txt не NULL");
    ASSERT((int)vec_len(loaded) == 10,   "load_vec_txt: len = 10");

    ASSERT(datatime_compare(vec_get(orig, 0), vec_get(loaded, 0)) == 0,
           "txt roundtrip: [0] совпадает");
    ASSERT(datatime_compare(vec_get(orig, 9), vec_get(loaded, 9)) == 0,
           "txt roundtrip: [9] совпадает");
    ASSERT(vec_get(orig, 0)->dev->data == vec_get(loaded, 0)->dev->data,
           "txt roundtrip: dev_data сохранён");

    /* get_elm_txt_slow vs get_elm_txt_fast */
    datatime *e_slow = get_elm_txt_slow(TMP_TXT, 4);
    datatime *e_fast = get_elm_txt_fast(TMP_TXT, 4);
    ASSERT(e_slow != NULL && e_fast != NULL, "get_elm_txt: оба индекса 4 не NULL");
    if (e_slow && e_fast)
        ASSERT(datatime_compare(e_slow, e_fast) == 0,
               "get_elm_txt_slow == get_elm_txt_fast для индекса 4");
    datatime_destroy(e_slow);
    datatime_destroy(e_fast);

    vec_destroy(orig);
    vec_destroy(loaded);
    remove(TMP_TXT);
}

static void test_fio_bin(void) {
    SECTION("fiovector: binary save / load / get / count");

    srand(123);
    vector_t *orig = vec_create(10);
    rand_gen_struct_in_container(orig, 10);

    int rc = save_vec_bin(orig, TMP_BIN);
    ASSERT(rc == 0, "save_vec_bin: возвращает 0");

    int cnt = count_elm_bin(TMP_BIN);
    ASSERT(cnt == 10, "count_elm_bin = 10");

    vector_t *loaded = load_vec_bin(TMP_BIN);
    ASSERT(loaded != NULL,             "load_vec_bin не NULL");
    ASSERT((int)vec_len(loaded) == 10, "load_vec_bin: len = 10");

    ASSERT(datatime_compare(vec_get(orig, 0), vec_get(loaded, 0)) == 0,
           "bin roundtrip: [0] совпадает");
    ASSERT(datatime_compare(vec_get(orig, 9), vec_get(loaded, 9)) == 0,
           "bin roundtrip: [9] совпадает");
    ASSERT(vec_get(orig, 0)->dev->data == vec_get(loaded, 0)->dev->data,
           "bin roundtrip: dev_data сохранён");

    /* get_elm_bin */
    datatime *e = get_elm_bin(TMP_BIN, 5);
    ASSERT(e != NULL, "get_elm_bin(5) не NULL");
    if (e) {
        ASSERT(datatime_compare(e, vec_get(orig, 5)) == 0,
               "get_elm_bin(5) совпадает с orig[5]");
        datatime_destroy(e);
    }

    vec_destroy(orig);
    vec_destroy(loaded);
    remove(TMP_BIN);
}

static void test_fio_edge_cases(void) {
    SECTION("fiovector: граничные случаи");

    ASSERT(load_vec_txt("_no_such.txt") == NULL,
           "load_vec_txt(несущ. файл) = NULL");
    ASSERT(load_vec_bin("_no_such.bin") == NULL,
           "load_vec_bin(несущ. файл) = NULL");
    ASSERT(get_elm_txt_slow("_no_such.txt", 0) == NULL,
           "get_elm_txt_slow(несущ. файл) = NULL");
    ASSERT(get_elm_txt_fast("_no_such.txt", 0) == NULL,
           "get_elm_txt_fast(несущ. файл) = NULL");
    ASSERT(get_elm_bin("_no_such.bin", 0) == NULL,
           "get_elm_bin(несущ. файл) = NULL");
    ASSERT(count_elm_txt("_no_such.txt") == 0,
           "count_elm_txt(несущ. файл) = 0");
    ASSERT(count_elm_bin("_no_such.bin") == 0,
           "count_elm_bin(несущ. файл) = 0");

    ASSERT(save_vec_txt(NULL, "x.txt") != 0, "save_vec_txt(NULL) = ошибка");
    ASSERT(save_vec_bin(NULL, "x.bin") != 0, "save_vec_bin(NULL) = ошибка");
    ASSERT(get_elm_txt_slow(NULL, 0) == NULL, "get_elm_txt_slow(NULL) = NULL");
    ASSERT(get_elm_bin(NULL, 0) == NULL,      "get_elm_bin(NULL) = NULL");
    ASSERT(get_elm_txt_slow("_no_such.txt", -1) == NULL,
           "get_elm_txt_slow(indx=-1) = NULL");
    ASSERT(get_elm_bin("_no_such.bin", -1) == NULL,
           "get_elm_bin(indx=-1) = NULL");
}

/* ══════════════════════════════════════════════════════════════
   TIMING — производительность
   ══════════════════════════════════════════════════════════════ */

static void test_timing(void) {
    SECTION("TIMING — измерение времени операций");

    const int N = 1000;
    struct timespec t0, t1;
    double ms;

    srand((unsigned)time(NULL));

    /* 1. rand_gen N элементов */
    vector_t *v = vec_create(N);
    t0 = _ts_now();
    rand_gen_struct_in_container(v, N);
    t1 = _ts_now();
    printf("  rand_gen(%d):               %8.3f мс\n", N, _ts_ms(t0, t1));

    /* 2. vec_push 10000 элементов (стресс realloc) */
    {
        vector_t *big = vec_create(1);
        t0 = _ts_now();
        for (int i = 0; i < 10000; i++)
            vec_push(big, datatime_create(1, 1, 2000, 0, 0));
        t1 = _ts_now();
        printf("  vec_push(10000):            %8.3f мс  (realloc каждые x1.41)\n",
               _ts_ms(t0, t1));
        vec_destroy(big);
    }

    /* 3. to_minutes в цикле N раз */
    t0 = _ts_now();
    for (int i = 0; i < N; i++)
        (void)datatime_to_minutes(vec_get(v, i));
    t1 = _ts_now();
    printf("  to_minutes x%d:             %8.3f мс\n", N, _ts_ms(t0, t1));

    /* 4. save txt */
    t0 = _ts_now();
    save_vec_txt(v, TMP_TXT);
    t1 = _ts_now();
    printf("  save_vec_txt(%d):           %8.3f мс\n", N, _ts_ms(t0, t1));

    /* 5. save bin */
    t0 = _ts_now();
    save_vec_bin(v, TMP_BIN);
    t1 = _ts_now();
    printf("  save_vec_bin(%d):           %8.3f мс\n", N, _ts_ms(t0, t1));

    /* 6. load txt */
    t0 = _ts_now();
    vector_t *lt = load_vec_txt(TMP_TXT);
    t1 = _ts_now();
    ms = _ts_ms(t0, t1);
    printf("  load_vec_txt(%d):           %8.3f мс\n", N, ms);
    vec_destroy(lt);

    /* 7. load bin */
    t0 = _ts_now();
    vector_t *lb = load_vec_bin(TMP_BIN);
    t1 = _ts_now();
    ms = _ts_ms(t0, t1);
    printf("  load_vec_bin(%d):           %8.3f мс  (ожидаем быстрее txt)\n", N, ms);
    vec_destroy(lb);

    /* 8. count_elm_txt vs count_elm_txt_fast */
    t0 = _ts_now();
    int cs = count_elm_txt(TMP_TXT);
    t1 = _ts_now();
    double ms_cs = _ts_ms(t0, t1);

    t0 = _ts_now();
    int cf = count_elm_txt_fast(TMP_TXT);
    t1 = _ts_now();
    double ms_cf = _ts_ms(t0, t1);

    printf("  count_elm_txt  (=%d):      %8.3f мс  (сканирует файл)\n", cs, ms_cs);
    printf("  count_elm_txt_fast(=%d):   %8.3f мс  (деление размера файла)\n", cf, ms_cf);

    /* 9. get_elm_txt_slow vs fast vs bin — последний элемент (худший случай) */
    int last = N - 1;

    t0 = _ts_now();
    datatime *es = get_elm_txt_slow(TMP_TXT, last);
    t1 = _ts_now();
    double ms_slow = _ts_ms(t0, t1);
    datatime_destroy(es);

    t0 = _ts_now();
    datatime *ef = get_elm_txt_fast(TMP_TXT, last);
    t1 = _ts_now();
    double ms_fast = _ts_ms(t0, t1);
    datatime_destroy(ef);

    t0 = _ts_now();
    datatime *eb = get_elm_bin(TMP_BIN, last);
    t1 = _ts_now();
    double ms_bin = _ts_ms(t0, t1);
    datatime_destroy(eb);

    printf("  get_elm_txt_slow(idx=%d): %8.3f мс  (O(n))\n",  last, ms_slow);
    printf("  get_elm_txt_fast(idx=%d): %8.3f мс  (O(1), fseek)\n", last, ms_fast);
    printf("  get_elm_bin     (idx=%d): %8.3f мс  (O(1), binary)\n", last, ms_bin);

    if (ms_fast < ms_slow)
        printf("  [OK]  fast < slow: fseek быстрее линейного поиска\n");
    else
        printf("  [WARN] fast >= slow: разница незаметна для N=%d\n", N);

    vec_destroy(v);
    remove(TMP_TXT);
    remove(TMP_BIN);
}

/* ══════════════════════════════════════════════════════════════
   MAIN
   ══════════════════════════════════════════════════════════════ */

int main(void) {
    printf("==========================================\n");
    printf("  ТЕСТЫ prog_s2 (lab_1 – lab_4)\n");
    printf("==========================================\n");

    /* LAB 1/2 — datatime */
    test_datatime_create_destroy();
    test_datatime_compare();
    test_datatime_increment();
    test_datatime_decrement();
    test_datatime_setters();
    test_datatime_string();
    test_datatime_minutes();
    test_datatime_copy();

    /* LAB 2 — device */
    test_device();

    /* LAB 3 — vector */
    test_vector_basic();
    test_vector_pop();
    test_vector_insert_remove();
    test_vector_change();
    test_vector_copy();
    test_vector_merge();
    test_vector_iterators();
    test_vector_null_safety();

    /* LAB 4 — fiovector */
    test_fio_txt();
    test_fio_bin();
    test_fio_edge_cases();

    /* Timing */
    test_timing();

    /* Итог */
    printf("\n==========================================\n");
    printf("  ИТОГ: %d/%d ПРОШЛО", s_pass, s_total);
    if (s_fail > 0)
        printf(", %d ПРОВАЛЕНО", s_fail);
    printf("\n");
    if (s_fail > 0)
        printf("  Строки [FAIL] указывают на баги в коде.\n");
    printf("==========================================\n");

    return (s_fail > 0) ? 1 : 0;
}
