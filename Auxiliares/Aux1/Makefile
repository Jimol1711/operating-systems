
INCLUDE=
HDRS= A1P2.h
SRCS= A1P2.c esPrimo.c
CFLAGS=-Wall -Werror -std=c18 $(INCLUDE)
LDLIBS=-lm -lpthread  -D_POSIX_C_SOURCE=199309L


%.o: %.c $(HDRS)
	gcc -c -o $@ $< $(CFLAGS) $(LDLIBS)

ejemplo_1a: ejemplo_1a.o

ejemplo_1b: ejemplo_1b.o

esPrimo: esPrimo.o A1P2.o
	gcc -o $@ $^ $(CFLAGS) $(LDLIBS)

clean:
	rm -f *.o esPrimo ejemplo_1a ejemplo_1b