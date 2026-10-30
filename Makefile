CC ?= cc
CC_68K ?= m68k-linux-gnu-gcc
PYTHON ?= python3
ENGINE = ami3d/src/render.c ami3d/src/shape.c ami3d/src/fixmath.c ami3d/src/tables.c ami3d/src/text.c ami3d/src/font.c ami3d/src/sort.c
GAME = src/park.c src/scene.c src/ui.c src/game.c
HEADERS = $(wildcard src/*.h ami3d/src/*.h)
CPPFLAGS += -Isrc -Iami3d/src -DSORT_MAX=1536
CFLAGS ?= -O2 -Wall -Wextra -Wno-misleading-indentation
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS = $(shell sdl2-config --libs)
QEMU_68K ?= qemu-m68k
AMIGA_FLAGS = -m68020-60 -O2 -Wall -Wextra -Wno-misleading-indentation -ffreestanding -fno-builtin -fno-pic -fno-pie -fno-common -fomit-frame-pointer -nostdlib -static -Iami3d/src/amiga -Iami3d/demo -Wl,--emit-relocs -Wl,--no-warn-rwx-segments -Wl,--build-id=none -Wl,-T,ami3d/demo/amiga.ld
AMIGA_SRC = src/platform_amiga.c ami3d/demo/start.S ami3d/src/amiga/render_68k.S ami3d/src/amiga/lmath_68k.S ami3d/src/amiga/lmath.c
.PHONY: all pc host test test_68k amiga adf clean
all: pc
build:
	mkdir -p build
pc: build/AmiFunFair-PC
build/AmiFunFair-PC: $(GAME) $(ENGINE) src/platform_sdl.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL_CFLAGS) -o $@ $(GAME) $(ENGINE) src/platform_sdl.c $(SDL_LIBS)
host: build/fair_host
build/fair_host: $(GAME) $(ENGINE) src/platform_host.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(GAME) $(ENGINE) src/platform_host.c
build/test_park: tests/test_park.c src/park.c src/park.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_park.c src/park.c
build/test_scene: tests/test_scene.c $(GAME) $(ENGINE) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_scene.c $(GAME) $(ENGINE)
test: build/test_park build/test_scene host
	build/test_park
	build/test_scene
	build/fair_host build/showcase.ppm
build/test_park_68k: tests/test_park.c src/park.c src/park.h | build
	$(CC_68K) -O2 -static -m68020-60 $(CPPFLAGS) -o $@ tests/test_park.c src/park.c
build/test_scene_68k: tests/test_scene.c $(GAME) $(ENGINE) $(HEADERS) | build
	$(CC_68K) -O2 -static -m68020-60 $(CPPFLAGS) -Iami3d/src/amiga -o $@ tests/test_scene.c $(GAME) $(ENGINE) ami3d/src/amiga/render_68k.S ami3d/src/amiga/lmath_68k.S ami3d/src/amiga/lmath.c
test_68k: build/test_park_68k build/test_scene_68k
	$(QEMU_68K) -cpu m68020 build/test_park_68k
	$(QEMU_68K) -cpu m68020 build/test_scene_68k
amiga: build/AmiFunFair
build/AmiFunFair.elf: $(GAME) $(ENGINE) $(AMIGA_SRC) $(HEADERS) | build
	$(CC_68K) $(CPPFLAGS) $(AMIGA_FLAGS) -o $@ $(GAME) $(ENGINE) $(AMIGA_SRC) -lgcc
build/AmiFunFair: build/AmiFunFair.elf ami3d/tools/elf2hunk.py
	$(PYTHON) ami3d/tools/elf2hunk.py $< $@
adf: build/AmiFunFair.adf
build/AmiFunFair.adf: build/AmiFunFair ami3d/tools/mkadf.py
	$(PYTHON) ami3d/tools/mkadf.py $@ AmiFunFair $<
clean:
	rm -rf build
