# Отладка с Valgrind, strace и ltrace

Исследование утечек памяти, системных вызовов и библиотечных функций на примере `prog_s2`.

---

## Теория: память, системные вызовы и инструменты

### Модель памяти в C — куча и стек

В C программист управляет памятью вручную. Есть два вида памяти:

**Стек (stack):**
- Выделяется автоматически при входе в функцию
- Освобождается автоматически при выходе из функции
- Быстро (просто сдвиг указателя стека `RSP`)
- Ограничен по размеру (обычно 8 МБ)
- Локальные переменные, параметры функций

```c
void foo(void) {
    int x = 5;     // стек — появляется при входе в foo
    char buf[100]; // стек — 100 байт на стеке
}                  // ← x и buf автоматически исчезают
```

**Куча (heap):**
- Выделяется явно через `malloc`
- Освобождается явно через `free`
- Медленнее (аллокатор ищет свободный блок)
- Практически не ограничена
- Данные живут до явного `free` или конца программы

```c
datatime *dt = malloc(sizeof(datatime)); // куча — явное выделение
// ... dt живёт пока мы не вызовем free ...
free(dt);                                // явное освобождение
```

### Что такое утечка памяти

**Утечка памяти** — ситуация когда выделенная через `malloc` память никогда не освобождается через `free`, и указатель на неё теряется.

```c
datatime *dt1 = datatime_create(...);  // malloc внутри
// ... используем dt1 ...
// datatime_destroy(dt1);  ← забыли!
// dt1 выходит из области видимости — указатель потерян
// блок памяти навсегда "занят" до конца программы
```

Для короткоживущих программ — не критично (ОС освобождает всю память процесса при завершении). Для серверов и долгоживущих программ — катастрофа: память растёт пока система не закончится.

### Что такое double free

**Double free** — вызов `free` дважды для одного и того же указателя.

```c
datatime *df = datatime_create(...);
datatime_destroy(df);   // первый free — OK
datatime_destroy(df);   // второй free — UNDEFINED BEHAVIOR
```

После первого `free` аллокатор (`malloc`) записывает служебные данные в освобождённый блок (связывает его в список свободных блоков). Второй `free` разрушает эту служебную структуру → аллокатор обнаруживает повреждение → `SIGABRT`.

В нашей сессии GDB показал: после double free поля структуры содержат мусор (`day=48622`), потому что glibc перезаписал память своими данными.

### Что такое use-after-free

**Use-after-free** — обращение к памяти после `free`.

```c
datatime *uaf = datatime_create(...);
datatime_destroy(uaf);  // free — память возвращена аллокатору
datatime_print(uaf);    // UNDEFINED BEHAVIOR — читаем освобождённую память
```

Опасность: программа может не упасть сразу. Аллокатор ещё не переиспользовал этот блок → данные там "ещё" лежат. Но это UB (undefined behavior) — в любой момент поведение может измениться. Valgrind надёжно находит этот класс ошибок, GDB — не всегда.

### Что такое неинициализированная память

Когда объявляешь переменную без инициализации, она содержит мусор — то, что было в этой ячейке памяти до неё:

```c
data_ft rec;        // rec содержит случайные байты
rec.day = dt->day;  // заполняем поля...
// НО: padding-байты между полями остаются мусорными
fwrite(&rec, sizeof(rec), 1, fp);  // пишем 24 байта, 2 из которых — мусор
```

Компилятор добавляет **padding** (выравнивание) между полями структуры чтобы адреса были кратны размеру типа. Эти байты существуют в памяти, но ни компилятор, ни программист их не трогает.

### Как работает Valgrind

Valgrind — не просто инструмент, это **платформа для динамического анализа кода**. Основной инструмент — Memcheck.

Valgrind работает через **бинарную инструментацию**: он перехватывает каждую инструкцию программы и добавляет перед ней свой код проверки. Фактически программа выполняется внутри виртуальной машины Valgrind.

Ключевая концепция — **теневая память (shadow memory)**:

```
Реальная память:    [байт данных] [байт данных] ...
Теневая память:     [статус байта] [статус байта] ...
                     ^ "инициализирован?" "выделен?"
```

Для каждого байта реальной памяти Valgrind хранит метаданные:
- **V-bit** — инициализирован ли этот байт
- **A-bit** — доступен ли этот байт (выделен ли блок)

Когда программа обращается к памяти, Valgrind проверяет соответствующие биты теневой памяти. Если байт неинициализирован или блок освобождён — сообщает об ошибке.

**Почему Valgrind медленный:** он обрабатывает каждую инструкцию и ведёт теневую память → программа работает в 10-50 раз медленнее обычного.

### Что такое системные вызовы и зачем strace

**Системный вызов (syscall)** — способ для программы попросить ядро Linux выполнить привилегированную операцию.

Программа не может напрямую записать в файл на диске — это делает только ядро. Программа вызывает `write(fd, buf, n)` → ядро получает управление → записывает данные → возвращает управление программе.

```
Пространство пользователя:    программа → fwrite() → write()
                                                          ↓
Граница пользователь/ядро: ════════════════════════════════════
                                                          ↓
Пространство ядра:                               sys_write() → диск
```

**strace** перехватывает все системные вызовы через `ptrace` (тот же механизм что и GDB) и показывает:
- Какие системные вызовы делает программа
- С какими аргументами
- Что они вернули

Применение: диагностика "почему программа не открывает файл", "почему зависает", "почему нет прав".

### Что такое библиотечные функции и зачем ltrace

**Библиотечные функции** (libc) — это обёртки над системными вызовами и дополнительные утилиты:

```
твой код → fopen()    → open() syscall → ядро
твой код → malloc()   → brk()/mmap() syscall → ядро
твой код → fprintf()  → write() syscall → ядро
```

`fopen` — библиотечная функция, внутри вызывающая системный вызов `openat`. `malloc` — библиотечная функция, управляющая кучей и периодически запрашивающая память у ядра через `mmap`/`brk`.

**ltrace** перехватывает вызовы динамически подключаемых библиотек (`.so` файлов) через подмену таблицы символов (PLT — Procedure Linkage Table).

Применение: диагностика "сколько раз вызывается malloc", "какие файлы открывает программа через fopen", "почему медленно работает I/O".

### Разница между strace и ltrace

```
Программа
    │
    ├── fopen("file.txt")         ← ltrace видит это
    │       │
    │       └── openat(...)       ← strace видит это
    │               │
    │               └── ядро Linux (диск)
    │
    ├── malloc(32)                ← ltrace видит это
    │       │
    │       └── brk() или mmap() ← strace видит это (редко, только при расширении кучи)
    │
    └── printf("hello")          ← ltrace видит это
            │
            └── write(1, "hello", 5) ← strace видит это
```

---

## Быстрый доступ: все команды и флаги

```bash
# Сборка с отладочными символами (обязательно для Valgrind)
make gcc
# gcc -g test.c lab_2/datatime.c lab_2/bitstruct.c lab_3/contvector.c lab_4/fiovector.c -o test.o

# --- VALGRIND ---
valgrind ./test.o                                          # базовый запуск
valgrind --leak-check=full ./test.o                        # полный отчёт об утечках
valgrind --leak-check=full --track-origins=yes ./test.o    # + откуда взялись uninit значения

# фильтр только нужного из вывода:
valgrind --leak-check=full --track-origins=yes ./test.o 2>&1 | grep -A 15 "uninitialised"

# --- STRACE ---
strace ./test.o                                            # все системные вызовы
strace -e trace=open,openat,read,write,close ./test.o      # только файловые операции
strace -e trace=openat ./test.o 2>&1 | grep test_data      # только наши файлы
strace -e trace=open,openat,read,write,close ./test.o 2>&1 | grep -E "test_data|open|fopen"

# --- LTRACE ---
ltrace ./test.o                                            # все библиотечные вызовы
ltrace ./test.o 2>&1 | grep -E "malloc|free"              # только память
ltrace ./test.o 2>&1 | grep -E "fopen|fclose|fread|fwrite" # только файловые функции
ltrace ./test.o 2>&1 | grep -E "malloc|free|fopen|fclose|fread|fwrite" | head -30
```

---

## Что такое эти инструменты и зачем они нужны

| Инструмент | Что делает | Когда использовать |
|---|---|---|
| **Valgrind** | Запускает программу в виртуальной среде, отслеживает каждое обращение к памяти | Утечки, double free, use-after-free, неинициализированные данные |
| **strace** | Перехватывает системные вызовы ядра (`open`, `read`, `write`, `mmap`...) | Посмотреть как программа работает с файлами, сетью, процессами |
| **ltrace** | Перехватывает вызовы библиотечных функций (`malloc`, `free`, `fopen`, `fread`...) | Посмотреть как программа использует libc |

Разница между strace и ltrace: strace — это вызовы **ядра** (низкий уровень), ltrace — вызовы **библиотек** (уровень libc). `fopen` — библиотечная функция, внутри которой ядро вызывает `openat`.

---

## 1. Valgrind — поиск утечек памяти

### Запуск

```bash
valgrind --leak-check=full ./test.o
```

Флаг `--leak-check=full` включает подробный отчёт: для каждой утечки показывает стек вызовов где была выделена память.

### Результат — найдены 3 утечки

```
==40971== HEAP SUMMARY:
==40971==     in use at exit: 102 bytes in 6 blocks
==40971==   total heap usage: 145 allocs, 139 frees, 58,514 bytes allocated
```

`145 allocs` и `139 frees` — разница 6. Это 6 блоков памяти, которые не были освобождены.

```
==40971== 34 (32 direct, 2 indirect) bytes in 1 blocks are definitely lost
==40971==    at 0x4846828: malloc (...)
==40971==    by 0x10A1FB: datatime_create (datatime.c:27)
==40971==    by 0x1094DA: main (test.c:23)      ← dt1

==40971== 34 (32 direct, 2 indirect) bytes in 1 blocks are definitely lost
==40971==    by 0x109500: main (test.c:24)      ← dt2

==40971== 34 (32 direct, 2 indirect) bytes in 1 blocks are definitely lost
==40971==    by 0x1096CF: main (test.c:47)      ← popped (результат vec_pop)
```

**34 байта = 32 (datatime) + 2 (device)** — каждый `datatime` состоит из двух `malloc`:
- 32 байта — сама структура `datatime`
- 2 байта — структура `device` (`uint16_t`) внутри `datatime`

### Объяснение утечек

**LEAK 1** — `test.c:47`, строка `vec_pop`:
```c
datatime *popped = vec_pop(vec);
printf("pop: "); datatime_print(popped);
// popped не освобождается! vec_pop передаёт владение вызывателю
```
`vec_pop` извлекает элемент из вектора и отдаёт указатель — ответственность за `datatime_destroy` переходит к вызывателю. Без явного `datatime_destroy(popped)` объект утекает.

**LEAK 2** — `test.c:23` и `test.c:24`, переменные `dt1` и `dt2`:
```c
datatime *dt1 = datatime_create(15, 3, 2025, 10, 30);
datatime *dt2 = datatime_create(20, 5, 2025, 18, 0);
// ... используются в тестах 1 и 2 ...
// datatime_destroy(dt1);   ← закомментировано намеренно
// datatime_destroy(dt2);   ← закомментировано намеренно
```
`dt1` и `dt2` нигде не уничтожаются до конца программы.

### Итог по утечкам

```
definitely lost: 96 bytes in 3 blocks   ← основные утечки
indirectly lost:  6 bytes in 3 blocks   ← device внутри утёкших datatime
ERROR SUMMARY: 4 errors from 4 contexts
```

---

## 2. Valgrind — неожиданная находка: неинициализированные байты

### Что нашли

```
==40971== Syscall param write(buf) points to uninitialised byte(s)
==40971==    at 0x498B5A4: write (write.c:26)
...
==40971==    by 0x10D0C0: save_vec_bin (fiovector.c:219)
==40971==    by 0x109B14: main (test.c:106)
```

Valgrind обнаружил, что при записи бинарного файла (`save_vec_bin`) программа пишет **неинициализированные байты**.

### Диагностика

Запуск с `--track-origins=yes` не показал точный источник — проблема была в буфере stdio. Это направило нас к самому коду `save_vec_bin` в `lab_4/fiovector.c:219`.

Структура `data_ft`:
```c
typedef struct {
    int day;        // 4 байта
    int month;      // 4 байта
    int year;       // 4 байта
    int hour;       // 4 байта
    int minute;     // 4 байта
    uint16_t dev_data;  // 2 байта
} data_ft;
// итого полей: 22 байта
// sizeof(data_ft): 24 байта ← компилятор добавил 2 байта PADDING
```

Компилятор выравнивает структуру до кратного 4 байтам, добавляя **2 байта padding** в конец после `dev_data`. Эти байты существуют в памяти, но никогда не инициализируются.

Проблемный код:
```c
data_ft rec;          // ← объявление без инициализации
rec.day = dt->day;
rec.month = dt->month;
rec.year = dt->year;
rec.hour = dt->hour;
rec.minute = dt->minute;
rec.dev_data = ...;
fwrite(&rec, sizeof(data_ft), 1, fp);  // пишет 24 байта, 2 из которых — мусор
```

### Исправление

```c
// было:
data_ft rec;

// стало:
data_ft rec = {0};    // обнуляет всю структуру включая padding
```

`= {0}` инициализирует все байты структуры нулями перед заполнением полей. После этого Valgrind больше не сообщает об этой ошибке.

---

## 3. strace — системные вызовы при работе с файлами

### Запуск

```bash
strace -e trace=open,openat,read,write,close ./test.o 2>&1 | grep -E "test_data|open|fopen"
```

Флаг `-e trace=...` фильтрует только нужные системные вызовы. `openat` — современный вариант `open` в Linux.

### Результат

```
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3      ← загрузка libc
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY)      ← загрузка libc
openat(AT_FDCWD, "test_data.txt", O_WRONLY|O_CREAT|O_TRUNC) = 3   ← save_vec_txt
openat(AT_FDCWD, "test_data.bin", O_WRONLY|O_CREAT|O_TRUNC) = 3   ← save_vec_bin
openat(AT_FDCWD, "test_data.txt", O_RDONLY) = 3                    ← count_elm_txt
openat(AT_FDCWD, "test_data.txt", O_RDONLY) = 3                    ← count_elm_txt_fast
openat(AT_FDCWD, "test_data.bin", O_RDONLY) = 3                    ← count_elm_bin
openat(AT_FDCWD, "test_data.txt", O_RDONLY) = 3                    ← get_elm_txt_slow
openat(AT_FDCWD, "test_data.txt", O_RDONLY) = 3                    ← get_elm_txt_fast
openat(AT_FDCWD, "test_data.bin", O_RDONLY) = 3                    ← get_elm_bin
openat(AT_FDCWD, "test_data.txt", O_RDONLY) = 3                    ← load_vec_txt (1й раз)
openat(AT_FDCWD, "test_data.txt", O_RDONLY) = 4                    ← load_vec_txt (2й раз!)
openat(AT_FDCWD, "test_data.bin", O_RDONLY) = 3                    ← load_vec_bin (1й раз)
openat(AT_FDCWD, "test_data.bin", O_RDONLY) = 4                    ← load_vec_bin (2й раз!)
```

### Что означают флаги

| Флаг | Значение |
|---|---|
| `O_WRONLY` | открыть только на запись |
| `O_RDONLY` | открыть только на чтение |
| `O_CREAT` | создать файл если не существует |
| `O_TRUNC` | очистить файл при открытии |
| `= 3` | файловый дескриптор (fd=3, т.к. 0,1,2 заняты stdin/stdout/stderr) |

### Наблюдение: двойное открытие файлов

`load_vec_txt` и `load_vec_bin` открывают файл **дважды** (fd=3 и fd=4):
1. Первый раз — для подсчёта элементов (`count_elm_txt_fast` / `count_elm_bin`)
2. Второй раз — для чтения самих данных

Это потенциальное место для оптимизации: можно было бы передавать уже открытый дескриптор в функцию чтения.

---

## 4. ltrace — библиотечные функции

### Запуск

```bash
ltrace ./test.o 2>&1 | grep -E "malloc|free|fopen|fclose|fread|fwrite" | head -30
```

### Результат — работа с памятью

```
malloc(32)  = 0x61c38db6a2b0    ← datatime_create: структура datatime
malloc(2)   = 0x61c38db6a2e0    ← datatime_create: структура device
malloc(32)  = 0x61c38db6a300    ← datatime_create (dt2)
malloc(2)   = 0x61c38db6a330    ← device для dt2
...
free(0x61c38db6a380)            ← datatime_destroy: сначала device
free(0x61c38db6a350)            ← datatime_destroy: потом datatime
malloc(16)  = 0x61c38db6a380    ← vec_create: структура vector_t
malloc(24)  = 0x61c38db6a3a0    ← vec_create: массив data[]
```

**Закономерность:**
- Каждый `datatime_create` → 2 `malloc` (32 + 2 байта)
- Каждый `datatime_destroy` → 2 `free` (сначала `device`, потом `datatime`)
- `vec_create` → 2-3 `malloc` (сам вектор + `data[]` + `res[]`)

### Результат — работа с файлами

```
fopen("test_data.txt", "w")  = 0x57d9f40537d0    ← save_vec_txt открывает
fclose(0x57d9f40537d0)       = 0                 ← save_vec_txt закрывает
fopen("test_data.bin", "wb") = 0x57d9f40537d0    ← save_vec_bin открывает
fwrite("\v", 24, 1, 0x...) = 1    ← элемент 1 (день=11=\v)
fwrite("\a", 24, 1, 0x...) = 1    ← элемент 2 (день=7=\a)
fwrite("\032", 24, 1, 0x...) = 1  ← элемент 3
... (10 записей для 10 элементов)
fclose(0x57d9f40537d0) = 0        ← save_vec_bin закрывает
```

**Почему `fopen("w")` для txt без `fwrite`?** — `save_vec_txt` использует `fprintf`, который является надстройкой над `fwrite`. ltrace показывает его отдельно, но в нашем grep он не попал.

**Что значат `"\v"`, `"\a"`, `"\032"`?** — ltrace показывает первый байт структуры в escape-нотации. Это первый байт поля `day`:
- `\v` = 0x0b = 11 (день)
- `\a` = 0x07 = 7 (день)
- `\032` = 0x1a = 26 (день)

Каждый `fwrite` пишет 24 байта (`sizeof(data_ft)`).

```
fread(0x7ffc..., 24, 1, fp) = 1   ← читает одну запись
fread(0x7ffc..., 24, 1, fp) = 1
... (10 раз)
fread(0x7ffc..., 24, 1, fp) = 0   ← конец файла
fclose(fp) = 0
```

`fread` возвращает количество прочитанных элементов: `1` — успешно, `0` — конец файла. Программа читает в цикле пока не получит `0`.

---

## 5. Сравнение strace vs ltrace

| | strace | ltrace |
|---|---|---|
| Что показывает | Системные вызовы ядра | Вызовы библиотечных функций |
| Файловый вызов | `openat(...)` | `fopen(...)` |
| Уровень | Ядро Linux | libc |
| Когда использовать | Проблемы с правами, файлами, сетью | Проблемы с памятью, I/O через libc |
| Накладные расходы | Меньше | Больше |

`fopen` внутри вызывает `openat` — поэтому оба инструмента показывают открытие файла, но на разных уровнях.

---

## 6. После исправления — чистые выводы

### Что исправили

| Проблема | Файл | Исправление |
|---|---|---|
| Утечка `dt1` | `test.c:71` | раскомментировали `datatime_destroy(dt1)` |
| Утечка `dt2` | `test.c:72` | раскомментировали `datatime_destroy(dt2)` |
| Утечка `popped` | `test.c:55` | раскомментировали `datatime_destroy(popped)` |
| Padding в `data_ft` | `fiovector.c:210` | `data_ft rec;` → `data_ft rec = {0}` |

### Valgrind — чистый вывод

```
==42285== HEAP SUMMARY:
==42285==     in use at exit: 0 bytes in 0 blocks
==42285==   total heap usage: 60,085 allocs, 60,085 frees, 1,655,318 bytes allocated

==42285== All heap blocks were freed -- no leaks are possible

==42285== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

- `0 bytes in 0 blocks` — нет ни одного утёкшего блока
- `60,085 allocs == 60,085 frees` — каждому выделению соответствует освобождение
- `0 errors` — никаких проблем с памятью

### strace — чистый вывод (только наши файлы)

```
openat(AT_FDCWD, "test_data.txt", O_WRONLY|O_CREAT|O_TRUNC, 0666) = 3  ← save_vec_txt
openat(AT_FDCWD, "test_data.bin", O_WRONLY|O_CREAT|O_TRUNC, 0666) = 3  ← save_vec_bin
openat(AT_FDCWD, "test_data.txt", O_RDONLY) = 3   ← count_elm_txt
openat(AT_FDCWD, "test_data.txt", O_RDONLY) = 3   ← count_elm_txt_fast
openat(AT_FDCWD, "test_data.bin", O_RDONLY) = 3   ← count_elm_bin
openat(AT_FDCWD, "test_data.txt", O_RDONLY) = 3   ← get_elm_txt_slow
openat(AT_FDCWD, "test_data.txt", O_RDONLY) = 3   ← get_elm_txt_fast
openat(AT_FDCWD, "test_data.bin", O_RDONLY) = 3   ← get_elm_bin
openat(AT_FDCWD, "test_data.txt", O_RDONLY) = 3   ← load_vec_txt (count внутри)
openat(AT_FDCWD, "test_data.txt", O_RDONLY) = 4   ← load_vec_txt (чтение данных)
openat(AT_FDCWD, "test_data.bin", O_RDONLY) = 3   ← load_vec_bin (count внутри)
openat(AT_FDCWD, "test_data.bin", O_RDONLY) = 4   ← load_vec_bin (чтение данных)
```

Вывод strace не изменился — исправление утечек не влияет на системные вызовы. Двойное открытие файлов в `load_vec_*` осталось — это особенность реализации, не ошибка.

### ltrace — чистый вывод (файловые операции, N=10)

```
fopen("test_data.txt", "w")   = 0x633174c846e0    ← save_vec_txt
fclose(0x633174c846e0)        = 0
fopen("test_data.bin", "wb")  = 0x633174c846e0    ← save_vec_bin
fwrite("\023", 24, 1, ...)    = 1                 ← элемент 1
fwrite("\031", 24, 1, ...)    = 1                 ← элемент 2
... (10 fwrite, по одному на элемент)
fclose(0x633174c846e0)        = 0
fopen("test_data.txt", "r")   = 0x633174c846e0    ← count_elm_txt
fclose(0x633174c846e0)        = 0
fopen("test_data.txt", "r")   = 0x633174c846e0    ← count_elm_txt_fast
fclose(0x633174c846e0)        = 0
fopen("test_data.bin", "rb")  = 0x633174c846e0    ← count_elm_bin
fclose(0x633174c846e0)        = 0
fopen("test_data.txt", "r")   = 0x633174c846e0    ← get_elm_txt_slow
fclose(0x633174c846e0)        = 0
fopen("test_data.txt", "r")   = 0x633174c846e0    ← get_elm_txt_fast
fclose(0x633174c846e0)        = 0
fopen("test_data.bin", "rb")  = 0x633174c846e0    ← get_elm_bin
fread(0x7ffe..., 24, 1, ...)  = 1
fclose(0x633174c846e0)        = 0
fopen("test_data.txt", "r")   = 0x633174c846e0    ← load_vec_txt (1й fopen)
fopen("test_data.txt", "r")   = 0x633174c848c0    ← load_vec_txt (2й fopen)
fclose(0x633174c848c0)        = 0
fclose(0x633174c846e0)        = 0
fopen("test_data.bin", "rb")  = 0x633174c846e0    ← load_vec_bin (1й fopen)
fopen("test_data.bin", "rb")  = 0x633174c848c0    ← load_vec_bin (2й fopen)
fclose(0x633174c848c0)        = 0
fread(0x7ffe..., 24, 1, ...)  = 1                 ← чтение 10 элементов
... (10 fread = 1)
fread(0x7ffe..., 24, 1, ...)  = 0                 ← конец файла
fclose(0x633174c846e0)        = 0
```

---

## 7. Выводы

1. **Valgrind нашёл 3 намеренные утечки** — `dt1`, `dt2` и результат `vec_pop`. Все подтверждены с точностью до строки исходного кода.

2. **Valgrind обнаружил скрытую ошибку** — неинициализированные padding-байты в `data_ft` при записи в бинарный файл. Ошибка не приводила к крашу, но данные в файле содержали мусор. Исправлено заменой `data_ft rec;` на `data_ft rec = {0};`.

3. **strace показал двойное открытие файлов** в `load_vec_txt` и `load_vec_bin` — функции открывают файл дважды: сначала для подсчёта элементов, потом для чтения. Это лишний системный вызов.

4. **ltrace подтвердил правильную работу с памятью** — каждому `malloc(32)+malloc(2)` соответствует пара `free+free`. Память управляется корректно везде кроме намеренно оставленных утечек.

5. **Valgrind незаменим** — GDB не нашёл бы padding-ошибку, потому что программа не падала. Только Valgrind отслеживает каждый байт.
