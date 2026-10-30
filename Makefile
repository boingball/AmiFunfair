CC ?= cc
CC_68K ?= m68k-linux-gnu-gcc
PYTHON ?= python3
ENGINE = ami3d/src/render.c ami3d/src/shape.c ami3d/src/fixmath.c ami3d/src/tables.c ami3d/src/text.c ami3d/src/font.c ami3d/src/sort.c
GAME = src/park.c src/scene.c src/ui.c src/game.c src/detail.c src/topdown.c
HEADERS = $(wildcard src/*.h ami3d/src/*.h)
CPPFLAGS += -Isrc -Iami3d/src -DSORT_MAX=4608
CFLAGS ?= -O2 -Wall -Wextra -Wno-misleading-indentation
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS = $(shell sdl2-config --libs)
QEMU_68K ?= qemu-m68k
AMIGA_FLAGS = -m68020-60 -O2 -Wall -Wextra -Wno-misleading-indentation -ffreestanding -fno-builtin -fno-pic -fno-pie -fno-common -fomit-frame-pointer -nostdlib -static -Iami3d/src/amiga -Iami3d/demo -Wl,--emit-relocs -Wl,--no-warn-rwx-segments -Wl,--build-id=none -Wl,-T,ami3d/demo/amiga.ld
AMIGA_SRC = src/platform_amiga.c src/pointer.c src/native_video.c src/vblank_68k.S ami3d/demo/start.S ami3d/src/amiga/render_68k.S ami3d/src/amiga/lmath_68k.S ami3d/src/amiga/lmath.c
.PHONY: all pc host test test_pc test_68k test_asan bench_68k amiga adf clean
all: pc
build:
	mkdir -p build
pc: build/AmiFunFair-PC
build/AmiFunFair-PC: $(GAME) $(ENGINE) src/platform_sdl.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL_CFLAGS) -o $@ $(GAME) $(ENGINE) src/platform_sdl.c $(SDL_LIBS)
build/test_sdl: tests/test_sdl.c $(GAME) $(ENGINE) src/platform_sdl.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL_CFLAGS) -DFAIR_NO_MAIN -o $@ tests/test_sdl.c $(GAME) $(ENGINE) src/platform_sdl.c $(SDL_LIBS)
test_pc: build/test_sdl
	SDL_VIDEODRIVER=dummy build/test_sdl
host: build/fair_host
build/fair_host: $(GAME) $(ENGINE) src/platform_host.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(GAME) $(ENGINE) src/platform_host.c
build/test_park: tests/test_park.c src/park.c src/park.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_park.c src/park.c
build/test_scene: tests/test_scene.c $(GAME) $(ENGINE) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_scene.c $(GAME) $(ENGINE)
build/test_detail: tests/test_detail.c src/detail.c src/native_video.c src/detail.h src/native_video.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_detail.c src/detail.c src/native_video.c
build/test_pointer: tests/test_pointer.c src/pointer.c src/pointer.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_pointer.c src/pointer.c
test: build/test_park build/test_scene build/test_detail build/test_pointer host
	build/test_park
	build/test_scene
	build/test_detail
	build/test_pointer
	build/fair_host build/showcase.ppm
build/test_scene_asan: tests/test_scene.c $(GAME) $(ENGINE) $(HEADERS) | build
	$(CC) $(CPPFLAGS) -O1 -g -fsanitize=address -fno-omit-frame-pointer -o $@ tests/test_scene.c $(GAME) $(ENGINE)
test_asan: build/test_scene_asan
	ASAN_OPTIONS=detect_leaks=0 build/test_scene_asan
build/test_park_68k: tests/test_park.c src/park.c src/park.h | build
	$(CC_68K) -O2 -static -m68020-60 $(CPPFLAGS) -o $@ tests/test_park.c src/park.c
build/test_scene_68k: tests/test_scene.c $(GAME) $(ENGINE) $(HEADERS) | build
	$(CC_68K) -O2 -static -m68020-60 $(CPPFLAGS) -Iami3d/src/amiga -o $@ tests/test_scene.c $(GAME) $(ENGINE) ami3d/src/amiga/render_68k.S ami3d/src/amiga/lmath_68k.S ami3d/src/amiga/lmath.c
build/test_detail_68k: tests/test_detail.c src/detail.c src/native_video.c src/detail.h src/native_video.h | build
	$(CC_68K) -O2 -static -m68020-60 $(CPPFLAGS) -o $@ tests/test_detail.c src/detail.c src/native_video.c
build/test_pointer_68k: tests/test_pointer.c src/pointer.c src/pointer.h | build
	$(CC_68K) -O2 -static -m68020-60 $(CPPFLAGS) -o $@ tests/test_pointer.c src/pointer.c
test_68k: build/test_park_68k build/test_scene_68k build/test_detail_68k build/test_pointer_68k
	$(QEMU_68K) -cpu m68020 build/test_park_68k
	$(QEMU_68K) -cpu m68020 build/test_scene_68k
	$(QEMU_68K) -cpu m68020 build/test_detail_68k
	$(QEMU_68K) -cpu m68020 build/test_pointer_68k
build/bench_render_68k: tools/bench_render.c $(GAME) $(ENGINE) $(HEADERS) | build
	$(CC_68K) -O2 -static -m68020-60 $(CPPFLAGS) -Iami3d/src/amiga -DBENCH_CACHE -o $@ tools/bench_render.c $(GAME) $(ENGINE) ami3d/src/amiga/render_68k.S ami3d/src/amiga/lmath_68k.S ami3d/src/amiga/lmath.c
bench_68k: build/bench_render_68k
	$(QEMU_68K) -cpu m68020 build/bench_render_68k
	$(QEMU_68K) -cpu m68020 build/bench_render_68k 2d
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
