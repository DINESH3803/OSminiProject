# ─── Smart ICU Monitoring System — Makefile ────────────────────
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude -D_GNU_SOURCE
LIBS    = -lpthread -lrt -lm

# Output directories
BINDIR  = bin

# Server sources
SRV_SRCS = server/server.c \
           server/handler.c \
           server/auth.c    \
           server/filestore.c \
           server/anomaly.c   \
           server/ipc_alert.c

# Client sources
NURSE_SRC  = client/nurse_client.c
DOCTOR_SRC = client/doctor_client.c
ADMIN_SRC  = client/admin_client.c
GUEST_SRC  = client/guest_client.c

# Targets
SERVER     = $(BINDIR)/icu_server
NURSE      = $(BINDIR)/nurse_client
DOCTOR     = $(BINDIR)/doctor_client
ADMIN      = $(BINDIR)/admin_client
GUEST      = $(BINDIR)/guest_client

.PHONY: all clean dirs run_server

all: dirs $(SERVER) $(NURSE) $(DOCTOR) $(ADMIN) $(GUEST)
	@echo ""
	@echo "  ✓ Build complete. Binaries in ./bin/"
	@echo ""
	@echo "  Run order:"
	@echo "    Terminal 1:  ./bin/icu_server"
	@echo "    Terminal 2:  ./bin/nurse_client  [--auto | --manual]"
	@echo "    Terminal 3:  ./bin/doctor_client"
	@echo "    Terminal 4:  ./bin/admin_client"
	@echo "    Terminal 5:  ./bin/guest_client"
	@echo ""

dirs:
	@mkdir -p $(BINDIR) data/patients logs

$(SERVER): $(SRV_SRCS) server/server.h include/*.h
	$(CC) $(CFLAGS) -o $@ $(SRV_SRCS) $(LIBS)
	@echo "  [OK] $@"

$(NURSE): $(NURSE_SRC) include/*.h
	$(CC) $(CFLAGS) -o $@ $(NURSE_SRC) $(LIBS)
	@echo "  [OK] $@"

$(DOCTOR): $(DOCTOR_SRC) include/*.h
	$(CC) $(CFLAGS) -o $@ $(DOCTOR_SRC) $(LIBS)
	@echo "  [OK] $@"

$(ADMIN): $(ADMIN_SRC) include/*.h
	$(CC) $(CFLAGS) -o $@ $(ADMIN_SRC) $(LIBS)
	@echo "  [OK] $@"

$(GUEST): $(GUEST_SRC) include/*.h
	$(CC) $(CFLAGS) -o $@ $(GUEST_SRC) $(LIBS)
	@echo "  [OK] $@"

run_server: $(SERVER)
	./$(SERVER)

clean:
	rm -rf $(BINDIR)
	@echo "  Cleaned."
