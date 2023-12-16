# The makefile for PermisC. Lean and simple!

# Set a default C compiler: gcc
CC := gcc
# Set the compiler flags:
# -std=c11: Use the C11 standard
# -Wall: Enable all warnings (by default there are like nearly none)
# -Wno-unused-function: Disable warnings for unused functions (that's annoying)
# -D_POSIX_C_SOURCE=200112L: Enable POSIX functions (for profiling)
# -g: Enable debug symbols
# -Isrc: Add the src folder to the include path
CFLAGS := -std=c11 -Wall -Wno-unused-function -D_POSIX_C_SOURCE=200112L -g -Isrc
ifeq ($(OPTIMIZE), 1)
	CFLAGS += -Ofast -flto
endif

# Use many CPU cores for parallel compilation
MAKEFLAGS += '-j 4'

# The output directory
OUT:=build-make

# All the source files, without the src folder.
SRC_FILES:=$(patsubst src/%, %, \
		     $(wildcard src/**/*.c) $(wildcard src/*.c))

# All the object files, with the output directory.
OBJ_FILES:=$(addprefix $(OUT)/, $(SRC_FILES:%.c=%.o))

# The full path to all header files
HEADER_FILES_F:=$(wildcard src/*.h) $(wildcard src/**/*.h)

# Where the data.csv file is, used for running the program.
FILE=../data.csv

.PHONY: debug
debug:
	@echo Objects: $(OBJ_FILES)
	@echo Src: $(SRC_FILES)
	@echo Headers: $(HEADER_FILES_F)

.PHONY: make_build_dir
make_build_dir:
	@mkdir -p $(OUT) && mkdir -p $(OUT)/computations

# $< is the first dependency
$(OUT)/%.o: src/%.c $(HEADER_FILES_F) | make_build_dir
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/PermisC: $(OBJ_FILES)
	@echo "Linking PermisC..."
	@$(CC) $(CFLAGS) $^ -o $@

.PHONY: build run clean
build: $(OUT)/PermisC
run: build
	@build-make/PermisC $(FILE)

clean:
	rm -r $(OUT)