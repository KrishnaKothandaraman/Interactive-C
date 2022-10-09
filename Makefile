all: main.o
	gcc main.o -o output
	./output

main.o: main.c
	gcc -c main.c

clean:
	rm *.o output
