# CFLAGS += -g

first: all

jv: jv.o

jv.o: jv.c

all: jv

clean:
	rm -f *.o
	rm -f jv
	rm -fr jv.dSYM/
