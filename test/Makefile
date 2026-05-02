UNAME := $(shell uname)
CC    := gcc
OUT   := test_all

SRCS := main.c \
        ../lab_2/datatime.c \
        ../lab_2/bitstruct.c \
        ../lab_3/contvector.c \
        ../lab_4/fiovector.c

CFLAGS := -g -Wall -Wextra -Wno-unused-result -Wno-unused-parameter

# clock_gettime требует -lrt на старых Linux (glibc < 2.17)
ifeq ($(UNAME), Linux)
    LDFLAGS := -lrt
else
    LDFLAGS :=
endif

# Проверка утечек памяти
ifeq ($(UNAME), Darwin)
    MEMCHECK := leaks --atExit --
else
    MEMCHECK := valgrind --leak-check=full --track-origins=yes \
                         --show-leak-kinds=all --error-exitcode=1
endif

.PHONY: all run memcheck clean

all: $(OUT)

$(OUT): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(OUT) $(LDFLAGS)

run: $(OUT)
	./$(OUT)

memcheck: $(OUT)
	$(MEMCHECK) ./$(OUT)

clean:
	rm -f $(OUT) _test_tmp.txt _test_tmp.bin
