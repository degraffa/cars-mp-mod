All addresses are for the Steam build, where `Cars.exe` loads at `0x400000`.

## Globals

| Address | Meaning |
| --- | --- |
| `0x779f34` | game object `g` |
| `0x80c5c0` | pad list `pads` |
| `0x7b5b6c` | engine timing and input root |
| `0x7b5b60` | `ForceMin30Hz` |
| `0x7b5c18` | frame timer. `+0x34` is the limiter interval, `+0x38` the longest step |
| `0x7c6414` / `0x7c641c` | P1 device type and pad index |
| `0x7c644c` / `0x7c6454` | P2 device type and pad index |

## Config: `[g+0x468]`

| Offset | Meaning |
| --- | --- |
| `+0x10` | mode, 2 is Versus |
| `+0x24` | player count |
| `+0x25` | AI count |
| `+0x27` | spare byte used by the mod, `min(count, 2)` |
| `+0x284+4i` | input index for player `i` |

## World: `[g+0x450]`

| Offset | Meaning |
| --- | --- |
| `+0x30`, `+0x34` | cameras 0 and 1 |
| `+0x3c`, `+0x40` | views 0 and 1 |
| `+0x44` | view manager |
| `+0x54` | layout |

Views are 2 slots wide and sit 12 bytes after the cameras. `mp_ext[0..1]` holds cameras 2 and 3, `mp_ext[3..4]` holds views 2 and 3, with the same 12-byte gap. That lets the game's creation loop write into `mp_ext` unchanged.

View fields: `+0x1a4` width, `+0x1a8` height. `0x433d10(view, x, y, w, h)` sets the rectangle. `g` vtable `+0x68` takes the list of active views.

## Input

- Input objects: `[[0x7b5b6c]+0x20]+0x10+4i`. Four exist. `+0x718` is the device type (1 is pad), `+0x71c` the pad index.
- Port count: `pads+0x12a74`. Reset to 2 at `0x6574ad`.
- P2's mapper: `pads+0x12a80`.
- `0x6a1210` allocates a mapper. `0x6a0e90(port, mapper)` assigns one.
- Bindings are 12 bytes. `type = 3 + pad index` for pads.

## Cameras

The camera target is `cam+0x30`, the controller `cam+0x38`. Let `sub = [veh+0x6a4]-0x74`. The vehicle's `CameraInterface` is `sub+0xd0`. To bind it, set `ctl+0x128` to that interface, copy `iface+0xb8` to `ctl+0x26c`, and OR 2 into `sub+0x30`.

## Functions

| Address | What it does |
| --- | --- |
| `0x440350` | load activity by name, such as `RR_..._VS` |
| `0x4395ea` | sets AI count during race setup |
| `0x4396bb`, `0x43970b` | copies player name and skin |
| `0x46bb95` | race HUD name lookup |
| `0x5046e0` | full-screen widget draw, only for layouts 0 to 2 |
| `0x535f90` | HUD vehicle registration, two slots |
| `0x555d40` | scene attach |
| `0x5567a0` | render setup |
| `0x556e40` | world build |
| `0x556fd8` | camera and view creation loop |
| `0x58c35d` | main loop call to the input tick `0x657c50` |
| `0x58f080` | `AddChild(parent, node, priority)` |
