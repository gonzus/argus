PIZZA = pizza
PIZZA_DIR = ../../$(PIZZA)
PIZZA_LIB = lib$(PIZZA).a

# CFLAGS += -g
CFLAGS += -O
CFLAGS += -I$(PIZZA_DIR)
# CFLAGS += -DLOG_LEVEL=0

LDFLAGS += -L$(PIZZA_DIR) -l$(PIZZA)
# LDFLAGS += -lprofiler

first: all

jv: jv.o $(PIZZA_DIR)/$(PIZZA_LIB)
	cc $(CFLAGS) $(LDFLAGS) -o $@ $^

jv.o: jv.c

all: jv

clean:
	rm -f *.o
	rm -f jv
	rm -fr jv.dSYM/
