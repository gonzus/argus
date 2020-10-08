NAME = json

# see more log messages
CFLAGS += -DLOG_LEVEL=1

PIZZA = pizza
PIZZA_DIR = ../../$(PIZZA)
PIZZA_LIB = lib$(PIZZA).a

CFLAGS += -std=c99
CFLAGS += -g
# CFLAGS += -O
CFLAGS += -I.
CFLAGS += -I$(PIZZA_DIR)
CFLAGS += -Wall -Wextra -Wshadow -Wpedantic
CFLAGS += -D_DEFAULT_SOURCE -D_SVID_SOURCE -D_XOPEN_SOURCE -D_GNU_SOURCE

LIBRARY = lib$(NAME).a

all: $(LIBRARY)

C_SRC_LIB = \
	stack.c \
	json.c \

C_OBJ_LIB = $(C_SRC_LIB:.c=.o)

$(LIBRARY): $(C_OBJ_LIB)
	ar -crs $@ $^

C_SRC_TEST = $(wildcard t/*.c)
C_OBJ_TEST = $(patsubst %.c, %.o, $(C_SRC_TEST))
C_EXE_TEST = $(patsubst %.c, %, $(C_SRC_TEST))

%.o: %.c
	cc $(CFLAGS) -c -o $@ $^

$(C_EXE_TEST): %: %.o $(LIBRARY)
	cc $(CFLAGS) $(LDFLAGS) -o $@ $^ $(PIZZA_DIR)/$(PIZZA_LIB) -ltap -lpthread

tests: $(C_EXE_TEST)

test: tests
	@for t in $(C_EXE_TEST); do ./$$t; done

valgrind: tests
	@for t in $(C_EXE_TEST); do valgrind -q ./$$t; done

clean:
	rm -f *.o
	rm -f $(LIBRARY)
	rm -f $(C_OBJ_TEST) $(C_EXE_TEST)
