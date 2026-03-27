gcc:
	gcc -g main.c contvector.c ../lab_2/datatime.c ../lab_2/bitstruct.c -o 1.o
clean:
	rm -f *.o
run: gcc
	leaks --atExit -- ./1.o