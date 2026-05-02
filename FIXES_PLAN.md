# План исправления ошибок в prog_s2

**Дата анализа:** 2026-05-01

## Критические баги (срочно)

### 1. **lab_1/main.c:91** — Двойное освобождение памяти
- **Ошибка:** Строки 88 и 91 вызывают `datatime_destroy(&dt3)` дважды
- **Последствие:** Undefined behavior, возможен краш при освобождении
- **Статус:** БЫЛ ИСПРАВЛЕН, но вернулся!
- **Как исправить:** Удалить вторую строку 91

### 2. **lab_1/datatime.c:42-43** — `create_empty_datatime()` всё ещё ломается
- **Ошибка:** Функция вызывает `datatime_create(0,0,0,0,0)`, которая не пройдёт проверку формата и вернёт NULL
- **Последствие:** Функция всегда возвращает NULL, а не пустой datatime
- **Статус:** БЫЛ ИСПРАВЛЕН ранее, но кто-то отменил!
- **Как исправить:** 
  ```c
  datatime* create_empty_datatime() {
      datatime* dt = malloc(sizeof *dt);
      if (dt == NULL) return NULL;
      
      dt->day = 1;
      dt->month = 1;
      dt->year = 1;
      dt->hour = 0;
      dt->minute = 0;
      return dt;
  }
  ```

### 3. **lab_2/datatime.c:87-91** — Разыменование NULL-указателя
- **Ошибка:** Проверка `if (dt->dev != NULL)` находится ВНЕ блока `if (dt != NULL)`
  ```c
  if (dt != NULL) {
      printf(...);
  }
  
  if (dt->dev != NULL) {  // ← UB: dt может быть NULL!
      ...
  }
  ```
- **Последствие:** Если dt=NULL, произойдёт UB (разыменование NULL)
- **Как исправить:** Перенести вторую проверку внутрь блока
  ```c
  if (dt != NULL) {
      printf(...);
      if (dt->dev != NULL) {
          ...
      } else {
          printf(" [Девайс отсутствует]\n");
      }
  }
  ```

### 4. **lab_2/datatime.c:76** — Неправильное копирование device
- **Ошибка:** `a->dev->data = b->dev->data` — это shallow copy (просто копирует uint16_t)
- **Последствие:** Теоретически работает, но не достаточно надёжно при будущих изменениях структуры
- **Статус:** БЫЛ ИСПРАВЛЕН (нужен memcpy), но не применен!
- **Как исправить:** Использовать `memcpy`
  ```c
  if (b->dev != NULL) {
      if (a->dev == NULL)
          a->dev = dev_create();
      if (a->dev != NULL)
          memcpy(a->dev, b->dev, sizeof(device));
  }
  ```

### 5. **lab_2/datatime.c:486-494** — `datatimes_switch` использует неинициализированную стековую переменную
- **Ошибка:** `datatime temp;` — неинициализированный стек, а затем `copy_datatime(&temp, dt1)`
- **Последствие:** UB — чтение из мусора, особенно опасно для `temp.dev`
- **Статус:** БЫЛ ИСПРАВЛЕН (нужен bytewise swap), но не применен!
- **Как исправить:** Использовать memcpy для побайтового обмена
  ```c
  void datatimes_switch(datatime *dt1, datatime *dt2) {
      if (dt1 == NULL || dt2 == NULL) return;
      
      datatime temp;
      memcpy(&temp, dt1, sizeof(datatime));
      memcpy(dt1, dt2, sizeof(datatime));
      memcpy(dt2, &temp, sizeof(datatime));
  }
  ```

## Утечки памяти (высокий приоритет)

### 6. **lab_3/contvector.c:59** — `vec_resize` теряет данные при ошибке realloc
- **Ошибка:** Если `realloc` вернёт NULL, оригинальный указатель теряется
  ```c
  vec->data = realloc(vec->data, new_cap * sizeof(datatime*));
  // ← если realloc вернёт NULL, vec->data станет NULL И мы потеряем старые данные
  ```
- **Последствие:** Утечка всех элементов вектора (~100% потеря памяти)
- **Как исправить:**
  ```c
  static void vec_resize(vector_t *vec, size_t new_cap) {
      if (vec == NULL) return;
      
      datatime **tmp = realloc(vec->data, new_cap * sizeof(datatime*));
      if (tmp == NULL) return;  // Оригинальный vec->data остался нетронут
      
      vec->data = tmp;
      vec->cap = new_cap;
  }
  ```

### 7. **lab_3/contvector.c:190-210** — `vec_copy` теряет вектор при ошибке malloc
- **Ошибка:** Если malloc на строке 197 или 200 вернёт NULL, функция возвращает NULL БЕЗ уничтожения частично заполненного temp
  ```c
  vector_t *temp = vec_create(vec->cap);  // ← частичный вектор
  for (size_t i = 0; i < vec->len; i++) {
      datatime *new_dt = malloc(sizeof(datatime));
      if (new_dt == NULL) return NULL;  // ← УТЕЧКА! temp остался неосвобождённым
  }
  ```
- **Последствие:** Утечка 1+ элементов + памяти самого вектора
- **Как исправить:**
  ```c
  vector_t *vec_copy(vector_t *vec) {
      if (vec == NULL) return NULL;
      
      vector_t *temp = vec_create(vec->cap);
      if (temp == NULL) return NULL;
      
      for (size_t i = 0; i < vec->len; i++) {
          datatime *new_dt = malloc(sizeof(datatime));
          if (new_dt == NULL) {
              vec_destroy(temp);  // ← очистить partial vector
              return NULL;
          }
          
          new_dt->dev = dev_create();
          if (new_dt->dev == NULL) {
              free(new_dt);
              vec_destroy(temp);  // ← очистить partial vector
              return NULL;
          }
          
          copy_datatime(new_dt, *(vec->data + i));
          *(temp->data + i) = new_dt;
          temp->len++;
      }
      return temp;
  }
  ```

### 8. **lab_3/contvector.c:213-237** — `vec_merge` имеет race-condition в изменении cap
- **Ошибка:** Если `vec_reserve` на строке 220 вернёт ошибку (1), то v1->cap уже был изменён на строке 218, но буфер не был переалоцирован → несоответствие
- **Последствие:** Вектор в нестабильном состоянии, vec->cap > реального размера буфера
- **Как исправить:**
  ```c
  int vec_merge(vector_t *v1, vector_t *v2) {
      if (v1 == NULL || v2 == NULL) return 1;
      
      size_t total_len = v1->len + v2->len;
      size_t old_cap = v1->cap;  // ← сохранить старое значение
      
      if (total_len > v1->cap) {
          v1->cap = total_len;
          if (vec_realloc_res(v1)) {
              v1->cap = old_cap;  // ← восстановить при ошибке
              return 1;
          }
          if (vec_reserve(v1)) {
              v1->cap = old_cap;  // ← восстановить при ошибке
              return 1;
          }
      }
      
      for (size_t i = 0; i < v2->len; i++) {
          datatime *new_dt = malloc(sizeof(datatime));
          if (new_dt == NULL) return 1;
          
          new_dt->dev = dev_create();
          if (new_dt->dev == NULL) {
              free(new_dt);
              return 1;
          }
          
          copy_datatime(new_dt, *(v2->data + i));
          vec_push(v1, new_dt);
      }
      
      return 0;
  }
  ```

## Нарушение спецификации (функциональные баги)

### 9. **lab_4/fiovector.c:190-219** — `save_vec_bin` не пишет количество элементов
- **Ошибка:** Спецификация (CLAUDE.md): *"Binary file format: first `int` = element count, then sequential raw structs"*
  - Но текущий код просто пишет structs БЕЗ count
  - При загрузке нужно угадывать кол-во через `count_elm_bin` (деление размера файла на размер struct)
  - Если в файл добавится хоть что-то лишнее → вычисленный count будет неправильным
- **Последствие:** Хрупкий формат, нет явного контроля границ, может привести к чтению мусора
- **Как исправить:**
  ```c
  int save_vec_bin(vector_t *vec, const char *file) {
      if (vec == NULL || file == NULL) return 1;
      
      FILE *fp = fopen(file, "wb");
      if (fp == NULL) {
          perror("Ошибка открытия файла для записи (bin)");
          return 1;
      }
      
      int count = (int)vec->len;
      if (fwrite(&count, sizeof(int), 1, fp) != 1) {  // ← НОВОЕ: пишем count
          perror("Ошибка записи count");
          fclose(fp);
          return 1;
      }
      
      for (size_t i = 0; i < vec->len; i++) {
          datatime *dt = vec->data[i];
          if (dt == NULL) {
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
          
          if (fwrite(&rec, sizeof(data_ft), 1, fp) != 1) {
              perror("Ошибка записи record");
              fclose(fp);
              return 1;
          }
      }
      
      fclose(fp);
      return 0;
  }
  ```

### 10. **lab_4/fiovector.c:222-255** — `load_vec_bin` должен читать count из файла
- **Ошибка:** Нужно читать count первым, а не вычислять его
- **Как исправить:**
  ```c
  vector_t* load_vec_bin(const char *file) {
      if (file == NULL) return NULL;
      
      FILE *fp = fopen(file, "rb");
      if (fp == NULL) {
          perror("Ошибка открытия файла для чтения (bin)");
          return NULL;
      }
      
      int count;
      if (fread(&count, sizeof(int), 1, fp) != 1) {  // ← НОВОЕ: читаем count
          perror("Ошибка чтения count");
          fclose(fp);
          return NULL;
      }
      
      if (count <= 0) {
          fclose(fp);
          return vec_create(1);  // Или другой размер по умолчанию
      }
      
      vector_t *vec = vec_create(count);
      if (vec == NULL) {
          fclose(fp);
          return NULL;
      }
      
      data_ft rec;
      for (int i = 0; i < count; i++) {
          if (fread(&rec, sizeof(data_ft), 1, fp) != 1) {
              perror("Ошибка чтения record");
              vec_destroy(vec);
              fclose(fp);
              return NULL;
          }
          
          datatime *dt = datatime_create(rec.day, rec.month,
                  rec.year, rec.hour, rec.minute);
          
          if (dt != NULL) {
              if (dt->dev != NULL)
                  dev_set_raw_data(dt->dev, rec.dev_data);
              
              vec_push(vec, dt);
          }
      }
      
      fclose(fp);
      return vec;
  }
  ```

### 11. **lab_4/fiovector.c:288-300** — `count_elm_bin` больше не нужна
- **Ошибка:** Функция основана на хрупком вычислении через размер файла
- **Как исправить:** Удалить или переделать, т.к. count теперь в файле

---

## Потенциальные регрессии (без исправлений, но нужно проверить)

### 12. **lab_2/datatime.c:19-37** — `datatime_from_minutes` требует инициализации dev
- **Статус:** ✓ ПРАВИЛЬНО в текущей версии (строка 34 выше)

### 13. **lab_3/contvector.c:71** — `vec_reserve` использует ручной цикл копирования
- **Статус:** ⚠ Может быть оптимизировано, но работает. Можно заменить на `memcpy`:
  ```c
  memcpy(vec->res, vec->data, vec->len * sizeof(datatime*));
  ```

### 14. **lab_4/main.c** — Проверить, что destroy вызывается правильно
- **Статус:** Требует проверки в исходном файле

---

## Итоговый чек-лист

### Критические (ОБЯЗАТЕЛЬНО):
- [ ] Удалить двойной destroy в lab_1/main.c:91
- [ ] Переписать create_empty_datatime() в lab_1
- [ ] Исправить проверку NULL в lab_2/datatime.c:datatime_print
- [ ] Использовать memcpy в lab_2/datatime.c:copy_datatime
- [ ] Переписать lab_2/datatime.c:datatimes_switch с memcpy
- [ ] Исправить vec_resize в lab_3
- [ ] Исправить vec_copy в lab_3
- [ ] Исправить vec_merge в lab_3

### Функциональные:
- [ ] Переписать save_vec_bin в lab_4 (добавить count)
- [ ] Переписать load_vec_bin в lab_4 (читать count)
- [ ] Удалить count_elm_bin или переделать

### Оптимизации (опционально):
- [ ] Заменить ручной цикл на memcpy в vec_reserve

### Тестирование:
- [ ] `make clean && make` в каждой lab
- [ ] `leaks --atExit -- ./1.o` для каждой лабы
- [ ] Проверить lab_4 с текстовым и бинарным форматом

---

## Команды для применения исправлений

```bash
cd /Users/gabikgabibullaev/c/prog_s2

# Перепроверить память после исправлений (для каждой лабы)
cd lab_1 && make clean && make && leaks --atExit -- ./1.o
cd ../lab_2 && make clean && make && leaks --atExit -- ./1.o
cd ../lab_3 && make clean && make && leaks --atExit -- ./1.o
cd ../lab_4 && make clean && make && leaks --atExit -- ./1.o save txt test.txt 5
```
