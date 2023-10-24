ifeq ($(OS),Windows_NT)
	$(error The current OS is Windows; please use Makefile.NT instead)
else
	detected_OS := $(shell uname -s)
endif
ifeq ($(detected_OS),Darwin)
	FINDPATH = gfind
else
	FINDPATH = find
endif
CC                = clang
CFLAGS            = -Wall -Wextra -Wpedantic -Werror-implicit-function-declaration -std=c99 -D_POSIX_C_SOURCE=200112L -pthread
LDFLAGS           = -pthread
BUILD_DIR        := build
PLATFORM_LINUX   := src/audio/audio-linux.c
PLATFORM_MACOS   := src/audio/audio-macos.c
PLATFORM_WINDOWS := src/audio/audio-windows.c src/socket/winsockfix.c
SRCFILES         := $(shell $(FINDPATH) src/ -type f -name '*.c' $$(printf "! -wholename %s " $(PLATFORM_LINUX)) $$(printf "! -wholename %s " $(PLATFORM_MACOS)) $$(printf "! -wholename %s " $(PLATFORM_WINDOWS)))

ifeq ($(detected_OS),Linux)
	SRCFILES += $(PLATFORM_LINUX)
	CFLAGS   += -D_DEFAULT_SOURCE
	LDFLAGS  += -lasound
endif
ifeq ($(detected_OS),Darwin)
	SRCFILES += $(PLATFORM_MACOS)
	CFLAGS   += -D_DARWIN_C_SOURCE
	LDFLAGS  += -framework AudioToolbox
endif

OBJFILES       := $(patsubst src/%.c,build/%.o,$(SRCFILES))

ifeq ($(DEBUG),1)
	CFLAGS += -g
endif

$(shell mkdir -p $(BUILD_DIR))

$(BUILD_DIR)/%.o: src/%.c
	@cobj='$@'; mkdir -p $${cobj%/*}
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJFILES)
	ar rcs $(BUILD_DIR)/libturntable.a $^
	cp src/turntable.h build/
ifneq ($(DEBUG),1)
	rm $^ && find $(BUILD_DIR)/* -maxdepth 0 -type d -exec rm -r "{}" \;
endif

demo: all
	$(CC) -o $(BUILD_DIR)/$@ example/turntable-demo.c $(CFLAGS) $(LDFLAGS) -Lbuild/ -lturntable

.PHONY: clean
clean:
	rm -r build/
