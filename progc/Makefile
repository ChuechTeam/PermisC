# The makefile for PermisC. Lean and simple!

# Set a default C compiler: gcc
export CC ?= gcc

# Set the compiler flags:
# -std=c11: Use the C11 standard
# -Wall: Enable all warnings (by default there are like nearly none)
# -Wno-unused-function: Disable warnings for unused functions (that's annoying)
# -D_POSIX_C_SOURCE=200112L: Enable POSIX functions (for profiling)
# -g: Enable debug symbols
# -Isrc: Add the src folder to the include path
export CFLAGS += -std=c11 -Wall -Wno-unused-function -D_POSIX_C_SOURCE=200112L -g -Isrc

# The optimization level, 1 to enable compiler optimizations.
export OPTIMIZE ?= 0
ifeq ($(OPTIMIZE), 1)
	# -Ofast: Enable ALL optimizations
	# -flto: Enable link-time optimizations (functions defined in .c files can be inlined)
	CFLAGS += -Ofast -flto
endif

# Set to 1 to tailor the executable to the current CPU.
export OPTIMIZE_NATIVE ?= 0
ifeq ($(OPTIMIZE_NATIVE), 1)
	CFLAGS += -march=native
endif

# Set to 1 to enable advanced experimental algorithms for some computations
export EXPERIMENTAL_ALGO ?= 0
ifeq ($(EXPERIMENTAL_ALGO), 1)
	CFLAGS += -DEXPERIMENTAL_ALGO=1
endif

# Set to 1 to enable advanced experimental algorithms that use AVX2.
# Requires compilation with AVX2 support, and an AVX2-capable processor (obviously).
export EXPERIMENTAL_ALGO_AVX ?= 0
ifeq ($(EXPERIMENTAL_ALGO_AVX), 1)
	CFLAGS += -DEXPERIMENTAL_ALGO_AVX=1
endif

# Set to 1 to output assembly files in the build folder.
export ASM ?= 0
ifeq ($(ASM), 1)
	CFLAGS := $(filter-out -flto, $(CFLAGS))
	ASM_CFLAGS := $(filter-out -g, $(CFLAGS)) -S -masm=intel -fverbose-asm
endif

# The output directory
OUT ?= build-make

# All the source files, without the src folder.
SRC_FILES := $(patsubst src/%, %, \
		     	$(wildcard src/**/*.c) $(wildcard src/*.c))

# All the object files, with the output directory.
OBJ_FILES := $(addprefix $(OUT)/, $(SRC_FILES:%.c=%.o))

# The full path to all header files
HEADER_FILES_F := $(wildcard src/*.h) $(wildcard src/**/*.h)

# Clean the build folder before building
CLEAN ?= 0

# Show the info banner
BANNER ?= 1
ifeq ($(BANNER), 1)
$(info ------ PermisC Makefile -------)
$(info Compiler: $(CC))
$(info Compiler flags: $(CFLAGS))
ifeq ($(OPTIMIZE), 1)
$(info Compiler optimizations: Enabled)
else
$(info Compiler optimizations: Disabled (Use OPTIMIZE=1 to enable them))
endif
ifeq ($(EXPERIMENTAL_ALGO), 1)
$(info Experimental algorithms: Enabled)
else
$(info Experimental algorithms: Disabled (Use EXPERIMENTAL_ALGO=1 to enable them))
endif
ifeq ($(EXPERIMENTAL_ALGO_AVX), 1)
$(info Experimental AVX2 algorithms: Enabled)
else
$(info Experimental AVX2 algorithms: Disabled (Use EXPERIMENTAL_ALGO_AVX=1 to enable them))
endif
$(info -------------------------------)
endif

.PHONY: make_build_dir
make_build_dir:
	@mkdir -p $(OUT) && mkdir -p $(OUT)/computations

# $< is the first dependency
$(OUT)/%.o: src/%.c $(HEADER_FILES_F) | make_build_dir
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@
ifeq ($(ASM), 1)
	@$(CC) $(ASM_CFLAGS) -c $< -o $(patsubst %.o, %.asm, $@)
endif

$(OUT)/PermisC: $(OBJ_FILES)
	@echo "Linking PermisC..."
	@$(CC) $(CFLAGS) $^ -o $@

.PHONY: build clean check_vars
build: | make_build_dir
	@# Use the build_vars.sh script to know if we need to recompile or not.\
	if [ "$(CLEAN)" = 1 ] || ! bash build_vars.sh check $(OUT)/build_vars; then\
		$(MAKE) --quiet clean BANNER=0;\
	fi
	@bash build_vars.sh update $(OUT)/build_vars
	@$(MAKE) --quiet "$(OUT)/PermisC" BANNER=0

clean:
	@\
	if [ -e "$(OUT)" ]; then\
		# If someone created a "build-make" file that isn't a folder (why would you even do this?!)\
        # then we need to remove it.\
        # Else, use the 'find' command to delete files in the directory WITHOUT removing the directory itself.\
        # Removing it using rm leads to annoying bugs when having build-make as the current directory.\
 		if [ ! -d "$(OUT)" ]; then rm "$(OUT)";\
 		else find "$(OUT)" -mindepth 1 -delete; fi;\
 	fi
	@echo "Build folder cleaned up!"

check_vars:
	@bash build_vars.sh check $(OUT)/build_vars