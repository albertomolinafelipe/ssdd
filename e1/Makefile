CC = gcc
CFLAGS = -c
LDFLAGS =   # Add any necessary linker flags here
SRCS = p1.c p2.c p3.c p4.c p5.c p6.c
OBJS = $(SRCS:.c=.o)
BIN_DIR = ./bin
EXEC = p1 p2 p3 p4 p5 p6

all: $(EXEC)

$(EXEC): %: %.o
	$(CC) $< -o $(BIN_DIR)/$@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

p5.o: p5.c lista.h
	$(CC) $(CFLAGS) p5.c -o p5.o

clean:
	rm -f $(BIN_DIR)/*.o $(BIN_DIR)/*
