## Setup

```sh
pip install ziglang capstone python-xlib evdev
./build.sh
```

Copy `binkw32.dll` to the game folder. `CarsMP.log` there shows what was applied, and any crash address with return addresses from the stack.

## Ways to change the game

Use the smallest one that works. Patches, hooks and traps are checked against the original bytes before anything is written, so a wrong address gives `ABORT` instead of a broken game.

**Byte patch.** Add to `src/patches.h` with the old and new bytes:

```c
P(0x46bb95,"\x33\xc0\xc3","\xeb\x01\x90"),
```

**Hook.** Needs at least 5 bytes of whole instructions. Write a stub in the `__asm__` block in `src/mod.c` that replays the bytes it replaced and jumps back, then add a line to `hooks[]`:

```c
".globl _hook_uidraw\n_hook_uidraw:\n  movl _mp_frameno,%ecx\n  movl %ecx,_mp_uiseen\n  movl 0x779f34,%ecx\n  pushl $0x5046f7\n  ret\n"

{0x5046f1,"\x8b\x0d\x34\x9f\x77\x00",6,hook_uidraw},
```

Use `pushal` and `popal` around C calls. Callbacks are `__cdecl` and need `__attribute__((used))`.

**Trap.** For a read too short to hook. Add the address to `traps.h` or `traps3.h`, which must stay sorted. `trap()` only decodes `8b` loads in those two forms. Each trap is an exception, so don't put them on code that runs thousands of times a frame.

**Per-frame code.** Add to `mp_frame`. It runs on the main thread.

**Setting.** Read it in `worker()` with `GetPrivateProfileIntA`.

## Reading the game

```sh
cd tools/re
python3 -i lib.py
>>> show(0x5567a0, after=60)       # disassemble
>>> xrefs(0x779f34)                # who uses this address
>>> findstr(b'RR_')                # find a string
```

`gen_clamps.py` and the `gen_traps` scripts rebuild the headers in `src/`. They are the starting point if you need to find every read of another field.

## Testing

```sh
python3 tools/harness/vpad.py 3 &        # optional virtual pads
tools/harness/run.sh 600 &         # headless and muted
python3 tools/harness/k.py menu w30 space w2 Return
```

Screenshots land in `tools/harness/out/`. `mem.py` reads live values from a running game. Build with `./build.sh -DTESTAI` to give all cars to the AI 900 frames into a race, and to log frame rate and stack samples.

Don't compare physics by feel. Run the same race at both frame rates and compare lap times.

Don't kill the game with `pkill -f` from a script whose own command line matches the pattern. Use `pkill -x`.

## Other starting points

- **P3 and P4 HUD:** `hook_hudreg` currently skips vehicles past index 1. Start with `0x535f90` and the two-slot storage it writes.
- **Mini-games:** `mp_nextact` picks the modes that get more players. Add a prefix there, then use the crash log.
- **More than 4 players:** the game only has four input objects, and `mp_ext` and `get_views` are sized for four.
- **Another build of the game:** every address will differ. `lib.py` and the generators can be pointed at it, but the hooks must be found again by hand.
