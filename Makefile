CC=gcc
CFLAGS=-Wall -Wextra -Iinclude
DCFLAGS=-g -fsanitize=address
LIBS=-lncurses -linih
TARGET=tetty

ifeq (${DEBUG}, 1)
    CFLAGS += $(DCFLAGS)
endif

SRC = src
OBJ = build
INC = include

_DEPS = input.h config.h game.h
_OBJS = main.o input.o config.o game.o

DEPS = $(patsubst %,$(INC)/%,$(_DEPS))
OBJS = $(patsubst %,$(OBJ)/%,$(_OBJS))

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS) $(CFLAGS)

$(OBJ)/%.o: $(SRC)/%.c $(DEPS) | $(OBJ)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ):
	mkdir $(OBJ)

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJS) 
