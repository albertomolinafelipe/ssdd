CC=gcc
CFLAGS=-Wall -fPIC -D_REENTRANT
LDFLAGS=-lrt

BIN_DIR=bin
BUILD_DIR=build

all: $(BIN_DIR)/servidor $(BIN_DIR)/cliente $(BUILD_DIR)/libclaves.so

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR)/proxy-mq.o: proxy-mq.c claves.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c proxy-mq.c -o $(BUILD_DIR)/proxy-mq.o

$(BUILD_DIR)/libclaves.so: $(BUILD_DIR)/proxy-mq.o | $(BUILD_DIR)
	$(CC) -shared -o $(BUILD_DIR)/libclaves.so $(BUILD_DIR)/proxy-mq.o $(LDFLAGS)

$(BIN_DIR)/servidor: servidor-mq.c claves.c claves.h | $(BIN_DIR)
	$(CC) $(CFLAGS) servidor-mq.c claves.c -o $(BIN_DIR)/servidor $(LDFLAGS)

$(BIN_DIR)/cliente: app-cliente.c claves.h $(BUILD_DIR)/libclaves.so | $(BIN_DIR)
	$(CC) $(CFLAGS) app-cliente.c -o $(BIN_DIR)/cliente -L$(BUILD_DIR) -lclaves -Wl,-rpath=$(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
