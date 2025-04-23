CC = gcc
CFLAGS = -Wall -pthread
BINDIR = bin
TARGET = server
SRCS = server.c db.c
OBJS = $(SRCS:.c=.o)

$(BINDIR)/$(TARGET): $(SRCS)
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/$(TARGET) $(SRCS)

all: $(BINDIR)/$(TARGET)

clean:
	rm -rf $(BINDIR)
