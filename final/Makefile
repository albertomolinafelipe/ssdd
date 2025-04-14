CC = gcc
CFLAGS = -Wall -pthread
BINDIR = bin
TARGET = server

$(BINDIR)/$(TARGET): server.c
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/$(TARGET) server.c

all: $(BINDIR)/$(TARGET)

clean:
	rm -rf $(BINDIR)
