# Super Mario 64 on the NumWorks calculator: the game's code from the n64decomp/sm64 decompilation (decomp/, not
# NumPlay's: see README.md) with the calculator's port, renderer, screens and storage (src/, tools/).
#   make            output/sm64.nwa
#   make check      link it like the calculator does (output/sm64.bin)
#   make host       build/play: the game on a computer, for tests and screenshots
#   make test       the game played on a computer (tests/check.py, with the sanitizers)
#   make media      the pictures of README.md and NumPlay's GIFs (tools/media.py)
#   make data       src/data.bin, src/data_ids.h and src/port/gen/ from the decompilation tools/fetch.py
#                   downloads (REF=build/ref/decomp, Python + Pillow + NumPy)
Q ?= @
# the ARM toolchain on the PATH, or in TOOLCHAIN=<its bin folder>
TOOLCHAIN ?=
BIN = $(if $(TOOLCHAIN),$(TOOLCHAIN)/)
CC = $(BIN)arm-none-eabi-gcc
NWLINK ?= npx --yes -- nwlink@1.0.0
STRIP ?= $(BIN)arm-none-eabi-strip --strip-unneeded
SIZE = $(BIN)arm-none-eabi-size
BUILD_DIR = output
REF ?=

# what is compiled: the decompilation's files and the port's (src/port/files.txt), the calculator's main
GAME_SRC = $(filter-out tests/play.c src/plat_host.c src/data.S,$(shell grep -v '^\#' src/port/files.txt)) src/main.c \
  src/plat_eadk.c
# the drawing runs for every pixel at -O2; the rest is small at -Os
HOT = src/render.c src/port/gfx.c
COLD = $(filter-out $(HOT),$(GAME_SRC))
OBJ = $(patsubst %.c,$(BUILD_DIR)/obj/%.o,$(GAME_SRC))
HEADERS = $(wildcard src/*.h src/port/*.h) ../common/epsilon_app.h ../common/epsilon_files.h

DECOMP_FLAGS = -DNON_MATCHING -DAVOID_UB -DVERSION_US -D_LANGUAGE_C -DNO_SEGMENTED_MEMORY -DSM_PORT \
  -include src/port/port.h -Isrc/port -Idecomp/include -Idecomp/src -Idecomp -Isrc -Isrc/port/gen
CFLAGS = -std=gnu11 $(shell $(NWLINK) eadk-cflags-device) -DDATA_BIN='"src/data.bin"' $(DECOMP_FLAGS)
CFLAGS += -fno-math-errno -fsingle-precision-constant -fno-strict-aliasing -fno-tree-loop-distribute-patterns
CFLAGS += -ffunction-sections -fdata-sections -flto -fno-fat-lto-objects -MMD -MP $(EXTRA)
WARN = -Wall -Wextra -Wno-unused-parameter
LDFLAGS = -Wl,--relocatable -nostartfiles --specs=nano.specs
LDFLAGS += -Wl,-e,main -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level
LDFLAGS += -Wl,--gc-sections -flinker-output=nolto-rel

.PHONY: build check clean host test media data
build: $(BUILD_DIR)/sm64.nwa

# apps get 133652 bytes of RAM on custom builds of Epsilon 25.2 (148928 or more on official ones)
check: $(BUILD_DIR)/sm64.nwa
	$(Q) $(NWLINK) nwa-bin --ram-length 133652 $< $(BUILD_DIR)/sm64.bin
	@echo "BIN     $(BUILD_DIR)/sm64.bin: $$(wc -c < $(BUILD_DIR)/sm64.bin) bytes"

# (the decompilation's files keep their own style: its warnings are not the calculator's)
$(BUILD_DIR)/obj/decomp/%.o: decomp/%.c src/port/port.h
	@mkdir -p $(@D)
	$(Q) $(CC) $(CFLAGS) -Os -w -c $< -o $@
$(BUILD_DIR)/obj/src/port/gen/%.o: src/port/gen/%.c src/port/port.h
	@mkdir -p $(@D)
	$(Q) $(CC) $(CFLAGS) -Os -w -c $< -o $@
$(patsubst %.c,$(BUILD_DIR)/obj/%.o,$(HOT)): $(BUILD_DIR)/obj/%.o: %.c $(HEADERS)
	@mkdir -p $(@D)
	$(Q) $(CC) $(CFLAGS) -O2 $(WARN) -c $< -o $@
$(BUILD_DIR)/obj/src/%.o: src/%.c $(HEADERS)
	@mkdir -p $(@D)
	$(Q) $(CC) $(CFLAGS) -Os $(WARN) -c $< -o $@

$(BUILD_DIR)/data.o: src/data.S src/data.bin | $(BUILD_DIR)
	$(Q) $(CC) -c -DDATA_BIN='"src/data.bin"' $< -o $@

$(BUILD_DIR)/sm64.nwa: $(OBJ) $(BUILD_DIR)/icon.o $(BUILD_DIR)/data.o
	@echo "LD      $@"
	$(Q) $(CC) $(CFLAGS) -Os $(LDFLAGS) $(OBJ) $(BUILD_DIR)/data.o $(BUILD_DIR)/icon.o -lm -o $@
	$(Q) $(STRIP) $@
	$(Q) $(SIZE) $@

$(BUILD_DIR)/icon.o: src/icon.png | $(BUILD_DIR)
	@echo "ICON    $<"
	$(Q) $(NWLINK) png-icon-o $< $@

$(BUILD_DIR):
	$(Q) mkdir -p $@

# on a computer: tests/play.c plays it with scripted keys and saves screenshots
HOST_SRC = $(filter-out src/data.S,$(shell grep -v '^\#' src/port/files.txt))
HOST_OBJ = $(patsubst %.c,build/obj/%.o,$(HOST_SRC))
HOST_FLAGS = -std=gnu11 -O1 -g -DHOST -DDATA_BIN='"src/data.bin"' $(DECOMP_FLAGS) -fno-strict-aliasing -MMD -MP
HOST_SAN = -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-sanitize=alignment
host: build/play
build/obj/%.o: %.c src/port/port.h
	@mkdir -p $(@D)
	$(Q) cc $(HOST_FLAGS) $(HOST_SAN) -w -c $< -o $@
build/play: $(HOST_OBJ) src/data.S src/data.bin
	$(Q) cc -c -DDATA_BIN='"src/data.bin"' src/data.S -o build/obj/data.o
	$(Q) cc $(HOST_SAN) $(HOST_OBJ) build/obj/data.o -lm -o $@
	@echo "HOST    $@"

-include $(OBJ:.o=.d) $(HOST_OBJ:.o=.d)

test: build/play
	python3 tests/check.py build/play

media: build/play
	python3 tools/media.py build/play

data:
	@test -n "$(REF)" || { echo "make data REF=<the folder tools/fetch.py filled>"; exit 1; }
	python3 -I tools/pack.py $(REF)
	python3 -I tools/port.py $(REF)

clean:
	$(Q) rm -rf $(BUILD_DIR) build
