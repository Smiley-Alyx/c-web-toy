CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
SRC_DIR = src
OBJ_DIR = build

SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/server.c $(SRC_DIR)/http.c \
       $(SRC_DIR)/static.c $(SRC_DIR)/mime.c $(SRC_DIR)/template.c \
       $(SRC_DIR)/session.c $(SRC_DIR)/logger.c $(SRC_DIR)/config.c \
       $(SRC_DIR)/tls.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

BIN = server

all: $(BIN)

LDLIBS = -lssl -lcrypto

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN)

.PHONY: all clean
