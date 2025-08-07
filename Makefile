.PHONY: all test clean

CC = clang
CFLAGS = -fPIC -Wall -Wextra -Wpedantic -Wno-unused-command-line-argument
LDFLAGS = -ldl -lreadline

BUILD ?= release

ifeq ($(BUILD),debug)
	CFLAGS = -Wall -Wextra -Wpedantic -ggdb -DDEBUG
	BINDIR = build/debug
else ifeq ($(BUILD),release)
	CLFAGS = -Wall -Wextra -Wpedantic -O2 -DNDEBUG
	BINDIR = build/release
else
	$(error Invalid build type: $(BUILD))
endif

SRC_DIR := src
LIB_DIR := lib
TEST_DIR := tests
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c, $(BINDIR)/%.o, $(SRC))
DEP := $(OBJ:.o=.d)
TARGET := $(BINDIR)/glorp
LIB_TARGET = $(LIB_DIR)/libglorp.a

$(shell mkdir -p $(BINDIR))

all: $(TARGET) $(LIB_TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(LIB_TARGET): $(OBJ)
	ar rcs $@ $^

$(BINDIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

test: $(TARGET)
	ln -sf $(realpath $(TARGET)) $(TEST_DIR)
	cd $(TEST_DIR) && ./run_tests.sh

clean:
	rm -rf $(BINDIR)
	rm -rf $(TARGET)
	rm -rf $(LIB_TARGET)
