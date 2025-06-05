SRC_DIR=src
TEST_DIR=test
BUILD_DIR=build

CC=gcc
CFLAGS=-c -g -Wall -std=gnu17 -I$(SRC_DIR)
#LDFLAGS=-pthread

CACHE ?= 0
CACHE_SIZE ?= 3
DEBUG ?= 0
DEBUG_OUTPUT ?= stderr
DFLAGS = -DCACHE=$(CACHE) -DCACHE_SIZE=$(CACHE_SIZE) -DDEBUG_LVL=$(DEBUG) -DDEBUG_OUTPUT=$(DEBUG_OUTPUT)

SOURCES=$(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/**/*.c)
INCLUDES=$(wildcard $(SRC_DIR)/*.h $(SRC_DIR)/**/*.h)
LIBRARIES=$(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(wildcard $(SRC_DIR)/**/*.c))
PROGRAMS=$(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%,$(wildcard $(SRC_DIR)/*.c))
OBJS=$(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

all: $(BUILD_DIR) $(OBJS) $(PROGRAMS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DFLAGS) -o $@ -c $<

$(BUILD_DIR)/%: $(BUILD_DIR)/%.o $(LIBRARIES) $(INCLUDES)
	$(CC) $(LDFLAGS) $(LIBRARIES) $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) *~ disco* ext* *res
