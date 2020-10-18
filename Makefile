SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,%.o,$(SRC))
target = hello
DEFS   =
CFLAGS = -g
CC     := gcc
INCLUDE =
LIB	    = -lpthread

$(target):$(OBJ)
	$(CC) $(CFLAGS) $(DEFS)  $^ -o $(target) $(LIB)

test:
	echo $(OBJ)
	echo $(SRC)

.PHONY:

clean:
	rm -rf *.o $(target)
