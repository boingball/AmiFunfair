# AmiFunFair

A native Amiga fairground management prototype using the **Ami3D** software
renderer. Build a park, set admission and attraction prices, connect paths,
hire staff, and watch animated rides and visitors.

## Play on an Amiga

Boot `AmiFunFair.adf` or copy `AmiFunFair` to your hard disk and run it from the
Shell. Choose Easy, Medium or Hard, or **Play Showcase Park** for a populated
example. Starting money is exactly **500,000 / 250,000 / 100,000**.

Target: **68020 or better**, ECS/AGA, mouse, roughly 1 MB available memory.
A **68030 with Fast RAM** is the intended first hardware test. The display is
320x200, 32 colours, with a vertical window that fits both PAL and NTSC.
Fast RAM drawing is selected automatically when its allocation succeeds;
otherwise the engine draws directly into chip RAM. No FPU is needed.

The executable and ADF are build-verified. Core gameplay and the 68k rendering
paths have been tested under qemu-m68k; the AmigaOS screen takeover, keyboard,
mouse and exit restoration still need testing in WinUAE and on real hardware.
There is no measured real-Amiga frame-rate claim yet.

## Build a fair

1. Open **Rides** or **Stands** in the bottom bar. The arrows page through the
   catalogue. Items you cannot afford are greyed out.
2. Choose an attraction. **R / Z** rotate the footprint clockwise / anticlockwise.
   Colour and operating speed can be changed in the placement panel.
3. Click a clear area to purchase and place the footprint.
4. For a ride, click a free tile beside its edge for its **entrance**, then a
   different adjacent tile for its **exit**. Both may be on existing paths.
5. Use **Paths** to connect **both gates** back to the front entrance. A stand
   only needs a connected path beside it. Disconnected paths appear darker.
6. Click an attraction to adjust price, colour, speed, open/closed status or sell
   it. The inspector shows condition, visits and connection status.

Right click or Esc cancels a build. Cancelling during either gate stage removes
the unfinished ride and refunds its **entire** purchase price. Completed rides
sell for half price. Paths cost 20; decorative trees cost 50.

Click the front entrance or **Park** to set admission, open/close the park,
pause, or start another park. Expensive admission reduces demand; visitors reject
overpriced attractions. Visitors follow cached shortest paths to attractions,
wait for capacity, spend time riding, emerge at the exit, then eventually leave.

**Staff** hires security and maintenance mechanics: 500 each to hire; daily
wages of 250 and 350 respectively. Security reduces theft losses. Mechanics
repair condition as rides wear out. Broken rides stop operating. A day is 60
seconds of simulation, and wages/upkeep are charged even if the park is closed.

| Control | Action |
| --- | --- |
| Left click | Select, purchase, place, use menus |
| Hold left mouse | Paint paths or plant trees |
| Right click / Esc | Cancel current tool / unfinished build |
| R / Z | Rotate the selected footprint |
| Arrow keys | Pan the park |
| Q / E | Orbit the camera |
| + / - | Zoom in / out |
| Mouse wheel (PC) | Zoom |
| Space | Pause / resume |
| Tab | Cycle bottom categories |
| H / F1 | Handbook |
| F10 | Quit and restore the desktop |

## Catalogue

All 21 requested items have their own footprint and procedural model:

| Rides and amusements | Stands and facilities |
| --- | --- |
| Bouncy castle | Ice cream stand |
| Ghost train | Candy floss stand |
| Wooden mine coaster | Toilets |
| Corkscrew | Coffee shop |
| Mini mouse | Fish and chips stand |
| Tea cups | |
| Pirate ship | |
| Roundabout jets and planes | |
| Dodgem cars | |
| Horse carousel | |
| Helter skelter | |
| Railway with tracks and a moving train | |
| Ski jump | |
| Twister | |
| Waltzer | |
| Amusements: arcade/fruit machine building | |

Coasters and the railway use **prebuilt circuits** in this prototype. There is
no freeform track editor yet. Customisation currently covers colour, speed and
price. Staff effects are park-wide rather than individual walking employees.
Trees are decorative. Save/load, audio, guest needs/happiness, research, terrain
height editing, and a full financial/end-game system are future work. Painter's
order is used, so large ride models can show occasional overlap artefacts.

## Build from source

The engine is pinned as a submodule to
`boingball/Ami3D@ab066ed2b11755b3f12f3a255e250d4c9befe62e`.
The Ami3D repository is private, so the cloning account needs access. Its source
is not duplicated in this public game repository.

```sh
git clone --recursive https://github.com/boingball/AmiFunfair.git
cd AmiFunfair
# For an existing clone:
git submodule update --init --recursive

# Linux PC version (SDL2 development files required):
make pc
build/AmiFunFair-PC

# Native Amiga + bootable disk (stock m68k ELF GCC; no NDK required):
make adf

# Tests and a headless showcase image:
make test
# Optional native 68k C/assembler checks, with qemu-user and cross libc:
make test_68k
```

On Ubuntu, the build packages are `build-essential`, `libsdl2-dev`,
`gcc-m68k-linux-gnu`, `python3` and (for `test_68k`) `qemu-user`.
Override `CC_68K`, `QEMU_68K`, `SDL_CFLAGS` or `SDL_LIBS` when needed.

`make build/test_park && build/test_park` works without fetching Ami3D.
Full rendering tests compare all 21 models at all four rotations pixel for
pixel between chunky and five-plane rendering. They also exercise ground
picking at multiple camera angles, the menus, rotated footprints, gate
validation, refunds, path connectivity, visitors, prices, staff and upkeep.

GitHub Actions always runs the independent park tests. For full PC, 68k and ADF
builds, set the repository Actions secret **AMI3D_READ_TOKEN** to a token with
read access to the engine repository. Full checks are explicitly skipped when
that secret is absent; the workflow does not expose private engine sources as
artifacts. Its only upload is the game executable, ADF and test render.

## Layout

* `src/park.c` / `park.h`: platform-independent simulation and placement rules.
* `src/scene.c`: fixed-point Ami3D models, animation, camera and ground picking.
* `src/ui.c`: bottom menus, difficulty screen, inspectors and mouse interaction.
* `src/platform_amiga.c`: planar display, PAL/NTSC-safe window, native input.
* `src/platform_sdl.c`: interactive PC frontend using the same game/render code.
* `src/platform_host.c`: deterministic, display-free image generation.
* `tests/`: gameplay, input and chunky/planar regression checks.

Amiga startup, OS call wrappers and build conversion tools come from Ami3D;
the fair's platform layer is adapted from its demo platform.
