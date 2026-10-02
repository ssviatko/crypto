all:
	gcc -c sha2.c -o sha2.o
	gcc -c color_print.c -o color_print.o
	gcc -c main.c -o main.o
	gcc -c aes.c -o aes.o
	gcc main.o aes.o sha2.o color_print.o -o aesctr

clean:
	rm -f aesctr
	rm -f *.o

