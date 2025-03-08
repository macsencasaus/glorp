.PHONY: all clean debug release

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Wpedantic
DEBUG_FLAGS = -ggdb -DDEBUG
LD_FLAGS =

SRC_DIR = src
OBJ_DIR = obj

TARGET = glorp

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

all: $(OBJ_FILES) $(TARGET)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

release: all

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ_FILES)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
