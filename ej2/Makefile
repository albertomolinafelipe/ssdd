CC=gcc
CFLAGS=-Wall -fPIC -D_REENTRANT
LDFLAGS=-lrt -lcjson

BIN_DIR=bin
BUILD_DIR=build


CJSON_CHECK=$(shell ldconfig -p | grep libcjson.so || echo "not found")

ifeq ($(CJSON_CHECK),not found)
$(error "--> libcjson is not installed (apt install libcjson-dev)")
endif


all: $(BIN_DIR)/servidor $(BIN_DIR)/cliente $(BUILD_DIR)/libclaves.so

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR)/proxy-sock.o: proxy-sock.c claves.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c proxy-sock.c -o $(BUILD_DIR)/proxy-sock.o

$(BUILD_DIR)/libclaves.so: $(BUILD_DIR)/proxy-sock.o | $(BUILD_DIR)
	$(CC) -shared -o $(BUILD_DIR)/libclaves.so $(BUILD_DIR)/proxy-sock.o $(LDFLAGS)

$(BIN_DIR)/servidor: servidor-sock.c claves.c claves.h | $(BIN_DIR)
	$(CC) $(CFLAGS) servidor-sock.c claves.c -o $(BIN_DIR)/servidor $(LDFLAGS)

$(BIN_DIR)/cliente: app-cliente.c claves.h $(BUILD_DIR)/libclaves.so | $(BIN_DIR)
	$(CC) $(CFLAGS) app-cliente.c -o $(BIN_DIR)/cliente -L$(BUILD_DIR) -lclaves -lcjson -Wl,-rpath=$(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
