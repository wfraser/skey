CFLAGS+=-Wall -Wextra -Wpedantic

all: skey skey_read

skey: skey.o
	$(CC) $(LDFLAGS) -o skey skey.o -lmhash

skey.o: skey.c dict.h
	$(CC) $(CFLAGS) -c -o skey.o skey.c

skey_read: skey_read.o
	$(CC) $(LDFLAGS) -o skey_read skey_read.o

skey_read.o: skey_read.c dict.h
	$(CC) $(CFLAGS) -c -o skey_read.o skey_read.c

clean:
	rm -f skey skey_read *.o *~

