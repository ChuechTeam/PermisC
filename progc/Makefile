# The makefile for PermisC. Lean and simple!

# Set a default C compiler: gcc
CC ?= gcc
# Set the compiler flags:
# -std=c11: Use the C11 standard
# -Wall: Enable all warnings (by default there are like nearly none)
# -Wno-unused-function: Disable warnings for unused functions (that's annoying)
# -D_POSIX_C_SOURCE=200112L: Enable POSIX functions (for profiling)
# -g: Enable debug symbols
# -Isrc: Add the src folder to the include path
CFLAGS += -std=c11 -Wall -Wno-unused-function -D_POSIX_C_SOURCE=200112L -g -Isrc

# The optimization level, 1 to enable compiler optimizations.
OPTIMIZE ?= 0
ifeq ($(OPTIMIZE), 1)
	CFLAGS += -Ofast -flto
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
$(info -------------------------------)
endif

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

.PHONY: build clean
build:
	@# Do a test check to see if the optimized_build file exists.\
    # The test returns 0 when it succeeded, so we need to invert it with the exclamation mark (!)\
	! [ -e $(OUT)/optimized_build ]; PREVIOUS_BUILD_OPTIMIZED=$$?; \
	# Clear the build folder when requested, or when the optimization level changed.\
	if [ "$(CLEAN)" = 1 ] || [ "$(OPTIMIZE)" -ne $$PREVIOUS_BUILD_OPTIMIZED ]; then\
		$(MAKE) --quiet clean BANNER=0;\
	fi
ifeq ($(OPTIMIZE), 1) # Create the file indicating that we've done an optimized build.
	@mkdir -p $(OUT) && touch $(OUT)/optimized_build
endif
	@$(MAKE) --quiet "$(OUT)/PermisC" BANNER=0

clean:
	@find "$(OUT)" -mindepth 1 -delete
	@echo "Build folder cleaned up!"