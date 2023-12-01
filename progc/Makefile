# The makefile for PermisC. Lean and simple!

# Set a default C compiler
CC := gcc
CFLAGS := -std=c11 -fPIC -Wall

OUT:=build-make

SRC_FILES:=$(patsubst src/%, %, \
		     $(wildcard src/**/*.c) $(wildcard src/*.c))

OBJ_FILES:=$(addprefix $(OUT)/, $(SRC_FILES:%.c=%.o))

HEADER_FILES_F:=$(wildcard src/*.h) $(wildcard src/**/*.h)

debug:
	@echo Objects: $(OBJ_FILES)
	@echo Src: $(SRC_FILES)
	@echo Headers: $(HEADER_FILES_F)

all: build

.PHONY: make_build_dir
make_build_dir:
	@mkdir -p $(OUT)

# $< is the first dependency
$(OUT)/%.o: src/%.c $(HEADER_FILES_F) | make_build_dir
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/PermisC: $(OBJ_FILES)
	@echo "Linking PermisC..."
	@$(CC) $(CFLAGS) $^ -o $@

build: $(OUT)/PermisC
run: build
	@build-make/PermisC
	