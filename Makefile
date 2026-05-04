SRC = test.c lab_2/datatime.c lab_2/bitstruct.c lab_3/contvector.c lab_4/fiovector.c

gcc:
	gcc -g $(SRC) -o test.o
O1:
	gcc -O1 -g $(SRC) -o test_O1.o
O2:
	gcc -O2 -g $(SRC) -o test_O2.o
O3:
	gcc -O3 -g $(SRC) -o test_O3.o
clean:
	rm -f test.o test_O1.o test_O2.o test_O3.o test_data.txt test_data.bin
run: gcc
	./test.o
