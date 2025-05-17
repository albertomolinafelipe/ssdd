CC = gcc
CFLAGS = -Wall -pthread
BINDIR = bin


TARGET = server
RPC_TARGET = logger_server

RPCX = logger.x
RPC_GEN = logger_clnt.c logger_svc.c logger_xdr.c logger.h

SRCS = server.c db.c logger_clnt.c logger_xdr.c
RPC_SRCS = logger_server.c logger_svc.c logger_xdr.c



all: $(BINDIR)/$(TARGET) $(BINDIR)/$(RPC_TARGET)

$(BINDIR)/$(TARGET): $(RPC_GEN) $(SRCS)
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/$(TARGET) $(SRCS)

$(BINDIR)/$(RPC_TARGET): $(RPC_GEN) $(RPC_SRCS)
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/$(RPC_TARGET) $(RPC_SRCS)

$(RPC_GEN): $(RPCX)
	rpcgen $(RPCX)



clean:
	rm -rf $(BINDIR)
	rm -f *.o
	rm -f $(RPC_GEN)
