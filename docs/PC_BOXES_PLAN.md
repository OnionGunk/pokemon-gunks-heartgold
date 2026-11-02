# More PC boxes: what depends on 18

Gunk's Platinum has 21 boxes. This is the map of every place in HeartGold that assumes 18, made on 2026-11-02
(not yet changed). Each box is `PC_BOX`: 30 `BoxPokemon` + 16 bytes = 0x1000 bytes.

## Open question: 21 or 24 boxes?

The PC's "jump to box" list shows 6 boxes per page (about 40 divide-by-6 sites in overlay 14). 21 boxes leave
the fourth page half empty: columns 3-5 would point at boxes 21-23, so they need clamping and greying out. 24 boxes
fill four pages exactly, and the save still fits (below). 21 matches Gunk's Platinum.

## Changes

**C (easy)**
- `include/constants/pokemon.h`: `NUM_BOXES`. `include/pokemon_storage_system.h` and
  `src/pokemon_storage_system.c` follow the define; `BOX_ALL_MODIFIED_FLAG` still fits a u32.
- `src/battle/battle_command.c:5677` (Safari "PC full"): literal `18` -> `NUM_BOXES`.
- Box names: "BOX 1"-"BOX 18" are msg_0024 ids 6-23, and overlay 14 reads later ids by number, so append the new
  names at the end (ids 94+) and split the loop at `pokemon_storage_system.c:45-47`.
- No assembly hard-codes a `PCStorage` field offset; everything goes through `PCStorage_*`.

**Save (moderate; breaks old saves, fine for a new playthrough)**
- The PC block is save slot 1 (`SAVE_PCSTORAGE`), written box by box (`Save_WriteNextPCBox`). 21 boxes: slot 1
  grows from 19 to 22 sectors, `SAVE_PAGE_MAX` 35 -> 38. 24 boxes: 25 sectors, `SAVE_PAGE_MAX` 41.
- The extra saves (Hall of Fame, battle videos) sit at `SAVE_PAGE_MAX + 0..11`: 38 -> sectors 38-49, 41 -> 41-52,
  both inside the 64-sector half that `Save_DeleteAllData` wipes.
- `SaveData.dynamic_region` grows by 0x1000 per sector: `HEAP_ID_1` (0x23600, `src/system.c:88`) must grow to
  hold `sizeof(SaveData)` (take it from the 0x11D000 heap).
- `boxModifiedFlags` is a u32 counted up to `PCStorage_GetNumBoxes()`; nothing else uses bits 18+.

**Assembly**
- `asm/overlay_08.s:13624` (battle, PC full): `cmp r0, #0x12` -> box count.
- `asm/overlay_14.s` (PC app), 9 box-count sites: 21416 and 21479 (wrap of the current box), 23059-23064 (box
  list cursor +-6), 30401 (thumbnail loop), 35026/35069, 35632/35675, 36328/36371 (box list cursor wrap).
- overlay 14's work struct (0x88E0 bytes, allocated at 11420) holds one 0x400-byte thumbnail per box at
  `+0x40C8 + box * 0x400`; the fields after it (0x88C8-0x88E0, about 56 literal-pool words) shift by 0x400 per
  added box, and the allocation grows to match.
- The box/party pickers use 18 as "party": GTS `asm/overlay_70.s` (2912, 11701, 11724, 11896, 12647, 13158, 13400,
  18289, 18448/18454, 18570/18576, 20165), the shared picker `asm/unk_020932E0.s` (541, 2103, 3204, 4323, 4405),
  Pokéathlon `asm/overlay_97.s` (201, 228, 518, 838), Pokéwalker `asm/overlay_112.s` (18657, 19116, 19228).

## Tests

Fill every box (630 or 720 Pokémon via `mons.py`), save, power cycle, compare every box with RAM; the torn-save
test with a multi-box save; open the PC, scroll and jump through every box and page; GTS, Pokéathlon and
Pokéwalker pickers on the last box and on the party.
