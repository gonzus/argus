# CFLAGS += -g
CFLAGS += -O

# LDFLAGS += -lprofiler

first: all

jv: jv.o
	cc $(CFLAGS) $(LDFLAGS) -o $@ $^

jv.o: jv.c

all: jv

clean:
	rm -f *.o
	rm -f jv
	rm -fr jv.dSYM/
