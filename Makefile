.PHONY: all test clean

CC = clang
CFLAGS = -fPIC -Wall -Wextra -Wpedantic -Wno-unused-command-line-argument -MMD -MP
LDFLAGS = -ldl -lreadline

GLORP_BUILD ?= release

ifeq ($(GLORP_BUILD),debug)
	CFLAGS += -ggdb -DDEBUG
	BIN_DIR = build/debug
else ifeq ($(GLORP_BUILD),release)
	CFLAGS += -O2 -DNDEBUG
	BIN_DIR = build/release
else
	$(error Invalid build type: $(GLORP_BUILD))
endif

SRC_DIR := src
LIB_DIR := $(BIN_DIR)/lib
DEP_DIR := $(BIN_DIR)/deps
TEST_DIR := tests

SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c, $(DEP_DIR)/%.o, $(SRC))
DEP := $(OBJ:.o=.d)
TARGET := $(BIN_DIR)/glorp
LIB_TARGET = $(LIB_DIR)/libglorp.a

$(shell mkdir -p $(BIN_DIR)/deps $(BIN_DIR)/lib)

all: $(TARGET) $(LIB_TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(LIB_TARGET): $(OBJ)
	ar rcs $@ $^
	cp -r include $(BIN_DIR)

$(DEP_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

-include $(DEP)

test: $(TARGET)
	ln -sf $(realpath $(TARGET)) $(TEST_DIR)
	cd $(TEST_DIR) && ./test.py

clean:
	rm -rf $(BIN_DIR)
	rm -f $(TARGET)
	rm -f $(LIB_TARGET)
	rm -f $(DEP)
