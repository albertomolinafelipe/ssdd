CC = gcc
CFLAGS = -Wall -Wextra -fPIC
LDFLAGS = -Lbuild -lclaves -lrt -lpthread
TARGET_LIB = build/libclaves.so
TARGET_SERVER = bin/servidor
TARGET_CLIENT = bin/cliente

SRCS_LIB = claves.c
OBJS_LIB = $(SRCS_LIB:.c=.o)

SRCS_SERVER = servidor-mq.c
OBJS_SERVER = $(SRCS_SERVER:.c=.o)

SRCS_CLIENT = app-cliente.c proxy-mq.c
OBJS_CLIENT = $(SRCS_CLIENT:.c=.o)

all: build bin $(TARGET_LIB) $(TARGET_SERVER) $(TARGET_CLIENT)

build:
	mkdir -p build

bin:
	mkdir -p bin

$(TARGET_LIB): $(OBJS_LIB)
	$(CC) -shared -o $@ $^

$(TARGET_SERVER): $(OBJS_SERVER) $(TARGET_LIB)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET_CLIENT): $(OBJS_CLIENT) $(TARGET_LIB)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET_LIB) $(TARGET_SERVER) $(TARGET_CLIENT)
	rm -rf build bin
