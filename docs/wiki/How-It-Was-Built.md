The mod was written by Claude Code over one session on CachyOS with Proton Experimental. There was no source code and no debug symbols to work from, only the binary.

## Starting point

`Cars.exe` is wrapped by SteamStub, so its code is encrypted on disk. The game folder also has `bak/Cars.exe`, an unwrapped retail build with the same code. That build is good for reading but it has a disc check, so it is never run.

Patching the file on disk was out: it would mean removing the DRM. A proxy DLL avoids that. The game loads `binkw32.dll` for video. The mod takes that name, forwards all 86 exports to the original (renamed `binkw32_o.dll`), and starts a worker thread.

The worker polls until every patch site holds the bytes it expects, which only happens after the stub has decrypted the code. If the bytes never match, it logs `ABORT` and changes nothing. Patches go in all at once or not at all.

## Finding the code

The exe was disassembled with Capstone into a pickled instruction list. `tools/re/lib.py` wraps it with `show`, `xrefs` and `findstr`. Most of the work was following one value through the code: start from a known string or global, find who reads it, repeat.

The first useful thread was the player count, a byte at `cfg+0x24`. Writing 3 to it did nothing good, but it led to everything that assumes two players.

## What was in the way

**Menus and HUD.** 75 reads of the player count index two-slot arrays. Instead of hooking each one, the mod changes the offset in each instruction from `0x24` to `0x27`, a spare byte that always holds `min(count, 2)`. The menus keep seeing 2 and the race sees 4.

**Camera and view arrays.** The world object stores cameras at `+0x30` and views at `+0x3c`, two slots each, right next to each other. Index 2 of the cameras lands on the views. 125 instructions read these arrays. They are 3 or 4 bytes long, too short for a 5-byte jump, so each one gets an `INT3`. A vectored exception handler decodes the instruction, returns the value from a side table (`mp_ext`) and moves on.

**Rendering.** The render switch at `0x5567a0` handles the 1 and 2 player layouts and has no case for 3 or 4. `mp_render` adds it.

**Scene graph.** New cameras and views have to be attached next to their siblings, or they never update.

**Input.** Four input objects already exist, but the port count is reset to 2. The mod sets it to 4, gives ports 2 and 3 their own action mappers and copies P2's bindings with the pad index rewritten. A binding's type field is `3 + pad index`.

**Timing.** Retail runs a fixed 0.033 s step each frame behind a 30 Hz limiter. Raising the limiter alone runs the game nearly 5x too fast. The mod clears `ForceMin30Hz` so the engine uses measured time, then raises the limiter. The step is still capped at 1/30 s, so a slow frame behaves as it did in stock.

AI best laps at 30 and 144 fps came out at 59.82/59.80, 1:01.99/1:01.54 and 1:02.99/1:02.88. That is within normal AI variation. Those laps were AI-driven, not human-driven.

## Bugs that took the longest

| Symptom | Cause |
| --- | --- |
| Mod did nothing, no error | Patched before the stub finished decrypting. Now waits and logs. |
| P3 and P4 had Dinoco skins | Name and skin slots overlap past index 1. They now have their own strings. |
| P3 and P4 copied P2's input | `cfg+0x284+4i` has to hold `i`. |
| P3 and P4 cameras were frozen | Their `CameraInterface` was never bound, and the connect bit was unset. |
| Pads ignored | Bindings were assumed to use type 4 and up. The type is `3 + pad index`. |
| Pause menu invisible | Full-screen widgets only draw on layouts 0 to 2. The mod drops to one view while a widget is up. |
| Crash on quit to menu | Old camera pointers were reattached. `mp_worldstart` now clears them for each new world. |
| Doubled menu text | The reset forced the count to 2 when the game had set it to 1. It now only lowers counts above 2. |
| Crash in mini-games at 4 | Not fixed. Mini-games stay at 2 players. See the issues. |

## Testing without a screen

All testing ran in a headless gamescope, so it needed no display and no one at the keyboard:

- XTEST sent menu input to gamescope's Xwayland.
- `gamescopectl screenshot` captured what the game drew.
- uinput created four virtual pads.
- `-DTESTAI` builds hand every car to the AI, so a whole race can run unattended.

Independent input was proven by pressing one pad at a time and logging each car's position. No real controller or human driver has tested the mod yet.
