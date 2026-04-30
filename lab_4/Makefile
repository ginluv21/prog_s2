gcc:
	gcc -g main.c fiovector.c ../lab_3/contvector.c ../lab_2/datatime.c ../lab_2/bitstruct.c -o 1.o
clean:
	rm -f *.o
run: gcc
	# ❌ ОШИБКА БЫЛА: /1.o — это путь от корня файловой системы, команда не найдёт файл
	# ✅ ИСПРАВЛЕНО: ./1.o — запуск из текущей директории
	./1.o