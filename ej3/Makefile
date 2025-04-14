CC = gcc
CFLAGS = -Wall -fPIC -g -D_REENTRANT -I/usr/include/tirpc
LDFLAGS = -lrt -ltirpc

RPC_X = claves_rpc.x
GENERATED = claves_rpc.h claves_rpc_clnt.c claves_rpc_svc.c claves_rpc_xdr.c claves_rpc_client.c  claves_rpc_server.c Makefile.claves_rpc

BIN_DIR = bin
BUILD_DIR = build

LIBCLAVES = $(BUILD_DIR)/libclaves.so
CLIENTE_BIN = $(BIN_DIR)/cliente
SERVIDOR_BIN = $(BIN_DIR)/servidor

.PHONY: all clean clean-rpc rpcgen

all: $(BUILD_DIR) $(BIN_DIR) rpcgen $(LIBCLAVES) $(CLIENTE_BIN) $(SERVIDOR_BIN)

# Directorios
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Generación de archivos RPC (main en .svc, lógica en tu .c)
rpcgen: clean-rpc
	rpcgen -M -N $(RPC_X)

clean-rpc:
	rm -f $(GENERATED)

# Objetos cliente para librería compartida
$(BUILD_DIR)/proxy-rpc.o: proxy-rpc.c claves.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/claves_rpc_clnt.o: claves_rpc_clnt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/claves_rpc_xdr.o: claves_rpc_xdr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Librería compartida cliente
$(LIBCLAVES): $(BUILD_DIR)/proxy-rpc.o $(BUILD_DIR)/claves_rpc_clnt.o $(BUILD_DIR)/claves_rpc_xdr.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

# Cliente
$(CLIENTE_BIN): app-cliente.c claves.h $(LIBCLAVES) | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ -L$(BUILD_DIR) -lclaves -Wl,-rpath=$(BUILD_DIR)

# Servidor (usa main de .svc y lógica en servidor-rpc.c)
$(SERVIDOR_BIN): claves.c servidor-rpc.c claves_rpc_svc.c claves_rpc_xdr.c claves_rpc_clnt.c claves_rpc.h | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Limpieza
clean: clean-rpc
	rm -rf $(BUILD_DIR) $(BIN_DIR)
