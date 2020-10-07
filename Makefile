first: all

PIZZA = pizza
PIZZA_DIR = ../../$(PIZZA)
PIZZA_LIB = lib$(PIZZA).a

CFLAGS += -g
# CFLAGS += -O
CFLAGS += -I$(PIZZA_DIR)
# CFLAGS += -DLOG_LEVEL=0

# LDFLAGS += -L$(PIZZA_DIR) -l$(PIZZA)
# LDFLAGS += -lprofiler

LIB_SRC = \
	stack.c \
	json.c \

MAIN_SRC = jv.c

LIB_OBJ = $(LIB_SRC:.c=.o)
MAIN_OBJ = $(MAIN_SRC:.c=.o)
MAIN_EXE = $(MAIN_SRC:.c=)

%.o: %.c
	cc $(CFLAGS) -c -o $@ $^

$(MAIN_EXE): $(MAIN_OBJ) $(LIB_OBJ) $(PIZZA_DIR)/$(PIZZA_LIB)
	cc $(LDFLAGS) -o $@ $^

all: $(MAIN_EXE)

clean:
	rm -f *.o
	rm -f jv
	rm -fr jv.dSYM/
