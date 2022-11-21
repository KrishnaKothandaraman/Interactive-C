all: psort.o
		clear
		gcc psort.o -o output

main.o: psort.c
		gcc -g -pthread psort.c

clean:
		rm *.o output