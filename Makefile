all: skey skey_read

skey: skey.o
	gcc -o skey skey.o -lmhash

skey.o: skey.c dict.h
	gcc -c -o skey.o -Wall skey.c

skey_read: skey_read.o
	gcc -o skey_read skey_read.o

skey_read.o: skey_read.c dict.h
	gcc -c -o skey_read.o -Wall skey_read.c

clean:
	rm -f skey skey_read *.o *~

