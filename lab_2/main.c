#include "bitstruct.h"
#include "datatime.h"

int main() {

   datatime* d = datatime_create(1, 1, 1970, 0, 0);
   if (d == NULL) {
      return 1;
   }

   uint16_t choice;
   int total_price = 0;

   printf("=== Конструктор часов CASIO ===\n");

   // DISPLAY
   printf("\nВыберите дисплей:\n");
   printf("0 - LCD      (1000 руб)\n");
   printf("1 - LED      (1500 руб)\n");
   printf("2 - OLED     (3000 руб)\n");
   printf("3 - E-Ink    (2500 руб)\n");
   printf("Ваш выбор: ");
   choice = get_choice(0, 3);

   dev_set_display(d->dev, choice);

   total_price += display_price[choice];

   // BRIGHTNESS
   printf("\nЯркость (0-15): ");
   choice = get_choice(0, 15);

   dev_set_brightness(d->dev, choice);
   total_price += choice * 50;  // условно 50 руб за уровень

   // TIME FORMAT
   printf("\nФормат времени:\n");
   printf("0 - 24 часа\n");
   printf("1 - 12 часов\n");
   printf("Ваш выбор: ");
   choice = get_choice(0, 1);

   dev_set_time_format(d->dev, choice);

   // ALARM
   printf("\nБудильник:\n");
   printf("0 - Нет\n");
   printf("1 - Да (+500 руб)\n");
   printf("Ваш выбор: ");
   choice = get_choice(0, 1);

   dev_set_alarm(d->dev, choice);
   if (choice == 1) 
      total_price += 500;

   // MEMORY
   printf("\nПамять:\n");
   printf("0 - 16KB (500 руб)\n");
   printf("1 - 32KB (1000 руб)\n");
   printf("2 - 64KB (2000 руб)\n");
   printf("3 - 128KB (3000 руб)\n");
   printf("Ваш выбор: ");
   choice = get_choice(0, 3);

   dev_set_memory(d->dev, choice);

   total_price += memory_price[choice];

   // CPU
   printf("\nПроцессор:\n");
   printf("0 - ARM     (2000 руб)\n");
   printf("1 - AVR     (1500 руб)\n");
   printf("2 - M-16pro (2500 руб)\n");
   printf("3 - x86     (4000 руб)\n");
   printf("Ваш выбор: ");
   choice = get_choice(0, 3);

   dev_set_cpu(d->dev, choice);

   total_price += cpu_price[choice];

   // WATER
   printf("\nВодостойкость:\n");
   printf("0 - Нет\n");
   printf("1 - IP67 (+800 руб)\n");
   printf("2 - IP68 (+1500 руб)\n");
   printf("Ваш выбор: ");
   choice = get_choice(0, 2);

   dev_set_water(d->dev, choice);

   if (choice == 1) total_price += 800;
   if (choice == 2) total_price += 1500;

   // ФИНАЛ
   printf("\n=== Ваше устройство собрано ===\n");

   printf("Дисплей: %s\n", 
      display_names[dev_get_display(d->dev)]);

   printf("Яркость: %u\n", 
      dev_get_brightness(d->dev));

   printf("Формат времени: %s\n", 
      dev_get_time_format(d->dev) == 0 ? "24 часа" : "12 часов");

   printf("Будильник: %s\n", 
      dev_get_alarm(d->dev) == 0 ? "Выключен" : "Включен");

   printf("Память: %s\n", 
      memory_names[dev_get_memory(d->dev)]);

   printf("Процессор: %s\n", 
      cpu_names[dev_get_cpu(d->dev)]);

   printf("Водостойкость: %s\n", 
      water_names[dev_get_water(d->dev)]);
    
   printf("\nИТОГОВАЯ ЦЕНА: %d руб\n", total_price);

   datatime_destroy(d);
   d = NULL;
   //datatime_destroy(d);

   return 0;
}