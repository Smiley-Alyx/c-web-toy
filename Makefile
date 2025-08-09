# Compiler & dirs
CC       := gcc
SRC_DIR  := src
OBJ_DIR  := build
BIN      := server

# Base flags
CFLAGS   ?= -Wall -Wextra -std=c11 -Iinclude
LDFLAGS  ?=
LDLIBS   ?= -lpthread

# Sources (base)
SRCS := $(SRC_DIR)/main.c $(SRC_DIR)/server.c $(SRC_DIR)/http.c \
        $(SRC_DIR)/static.c $(SRC_DIR)/mime.c  $(SRC_DIR)/template.c \
        $(SRC_DIR)/session.c $(SRC_DIR)/logger.c $(SRC_DIR)/config.c

# TLS toggle: make TLS=1
ifeq ($(TLS),1)
CFLAGS  += -DENABLE_TLS
LDLIBS  += -lssl -lcrypto
SRCS    += $(SRC_DIR)/tls.c
endif

# AddressSanitizer toggle: make ASAN=1
ifeq ($(ASAN),1)
CFLAGS  += -fsanitize=address -fno-omit-frame-pointer -g
LDFLAGS += -fsanitize=address
endif

# Objects
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Default target
all: $(BIN)

# Link rule — IMPORTANT: use LDFLAGS before objects and LDLIBS at the end
$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

# Compile rule
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Housekeeping
clean:
	rm -rf $(OBJ_DIR) $(BIN)

.PHONY: all clean
