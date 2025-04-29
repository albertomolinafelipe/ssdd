CC = gcc
CFLAGS = -Wall -pthread
BINDIR = bin

# ---- Programas ----
TARGET = server
RPC_TARGET = logger_server

# ---- Fichero de interfaz RPC ----
RPCX = logger.x
RPC_GEN = logger_clnt.c logger_svc.c logger_xdr.c logger.h

# ---- Fuentes ----
SRCS = server.c db.c logger_clnt.c logger_xdr.c
RPC_SRCS = logger_server.c logger_svc.c logger_xdr.c

# ---- Reglas ----
all: $(BINDIR)/$(TARGET) $(BINDIR)/$(RPC_TARGET)

# Compilar el cliente principal (server)
$(BINDIR)/$(TARGET): $(RPC_GEN) $(SRCS)
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/$(TARGET) $(SRCS)

# Compilar el servidor RPC (logger_server)
$(BINDIR)/$(RPC_TARGET): $(RPC_GEN) $(RPC_SRCS)
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/$(RPC_TARGET) $(RPC_SRCS)

# Generar los ficheros RPC automáticamente si no existen
$(RPC_GEN): $(RPCX)
	rpcgen $(RPCX)

# Limpieza
clean:
	rm -rf $(BINDIR)
	rm -f *.o
	rm -f $(RPC_GEN)
