gcc:
	gcc -g test.c lab_2/datatime.c lab_2/bitstruct.c lab_3/contvector.c lab_4/fiovector.c -o test.o
clean:
	rm -f test.o test_data.txt test_data.bin
run: gcc
	./test.o
