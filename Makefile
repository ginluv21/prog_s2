CC     = gcc
CFLAGS = -g -Wall -Wextra -Wno-unused-result

SRC = main.c \
      lab_4/fiovector.c \
      lab_3/contvector.c \
      lab_2/datatime.c \
      lab_2/bitstruct.c

TARGET = main_test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: all
	./$(TARGET)

# macOS — встроенный анализ утечек
leaks: all
	leaks --atExit -- ./$(TARGET)

# Linux — valgrind (полная проверка памяти)
valgrind: all
	valgrind \
		--leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./$(TARGET)

clean:
	rm -f $(TARGET)
