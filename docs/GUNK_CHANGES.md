# Pokémon Gunk's HeartGold Version: change list

Every change in Pokémon Gunk's Platinum Version (`pkplat-dexnav`, branch `dexnav-followers`, 23 commits on top of
pret/pokeplatinum `6eeb1b04`), with a verdict for HeartGold:

- **port**: same feature, rewritten against pokeheartgold's code.
- **adapt**: same goal, but HeartGold needs a different shape.
- **skip**: HeartGold already has it, or the thing it changes doesn't exist in HeartGold.

Compatibility rules carried over from the Platinum hack: append script commands, common scripts, items and text
instead of renumbering; don't change struct layouts unless a feature needs it; grow save blocks only within the
flash budget (see "Save budget" below). No difficulty changes.

Status: ✅ done and tested in the emulator, 🟡 built and passes the save suite but the feature itself not yet exercised in the emulator, ⏳ planned.

## Identity

| # | Change (Platinum commit) | Verdict | HeartGold plan | Status |
|---|---|---|---|---|
| 1 | Name: "Pokémon Gunk's Platinum Version" in the DS menu banner and end credits (8fa30cda) | port | Banner "Pokémon Gunk's" / "HeartGold Version"; staff credits title "Pokémon Gunk’s HeartGold Version". | 🟡 |
| 2 | ROM header title `GUNK PL` (3f119584); game code kept as CPUE so emulators pick the right save type | port | Header title `GUNK HG` (checked in the ROM), game code stays `IPKE`. | ✅ |
| 3 | Title-screen logo "Gunk's Platinum Version" (24f96c51) | adapt | New HeartGold title logo art; needs the HG title screen's graphics format. | ⏳ |

## Save and storage

| # | Change | Verdict | HeartGold plan | Status |
|---|---|---|---|---|
| 4 | 21 PC boxes, up from 18: a living dex through Gen 4 plus 3 spare (7713e967) | port | `NUM_BOXES` 18 → 21. HG's `PCStorage` keeps names and wallpapers after the boxes, and HG tracks modified boxes in a 32-bit mask (21 fits). The box block grows ~3 sectors; HG's main save uses 35 of 64 sectors per flash half with the extra saves (Hall of Fame, Battle Frontier videos) addressed as `SAVE_PAGE_MAX + n`, so `SAVE_PAGE_MAX` has to grow and the extras shift with it. Box names 19-21 appended to the box-name text bank (msg_0024). **Risk:** HG's PC app (overlay 14, `asm/overlay_14.s`) is still undecompiled assembly with ~95 candidate literal 18s and per-box arrays in its working memory (Platinum's had `[18][1024]` icon buffers); every one must be found and the working-memory size grown, or boxes 19-21 overrun memory. Done after the first batch, with a full-box test. | ⏳ |
| 5 | Save table grown for DexNav: per-species search levels stored after the PC boxes in the boxes block (5985a222) | port | New save array appended after `SAVE_PCSTORAGE` (keeps every retail array ID). | ⏳ |
| 6 | Static check that the main save plus extra saves fit in one flash half | port | Same compile-time assert for HG's `SAVE_PAGE_MAX + last extra chunk`. | ⏳ |

## DexNav

| # | Change | Verdict | HeartGold plan | Status |
|---|---|---|---|---|
| 7 | DexNav key item: Land/Water species menu, R searches for the selected species, chaining, search levels, top-screen search bar (5985a222) | adapt | Key item first (registerable). HG's encounter tables have morning/day/night grass slots, Surf, Rock Smash and the radio/swarm overrides: build the species list from HG's `encounter_data`. | ⏳ |
| 8 | Legal encounters: one Method J RNG sequence (941125b5) | adapt | HGSS wild encounters use Method K (nature from `rand % 25`, not `/ 0xA3E`). Reroll with HG's own wild-encounter code so results stay legal. | ⏳ |
| 9 | DexNav Pokétch app: area species, caught in color / seen as silhouette / unseen as "?", tap to target (b5551c4f, 7890b973) | adapt | HeartGold has no Pokétch. Options: (a) a Pokégear tab (the Pokégear's apps are fixed; adding one is a sizeable UI job), (b) the key item's own touch-screen menu showing the same caught/seen/unseen grid. Plan: (b) first, (a) only if you want it. | ⏳ |
| 10 | DexNav gift from Dawn after the Route 202 catching tutorial (b5551c4f) | adapt | Equivalent story beat: Ethan/Lyra's Route 29 catching tutorial, or Professor Elm with the Pokédex. | ⏳ |

## Followers

| # | Change | Verdict | Status |
|---|---|---|---|
| 11 | Following Platinum's Pokémon followers: 566 sprites, renderer, timing, shiny palettes, spawn exclusions (5985a222) | skip: HeartGold has followers | — |
| 12 | Follower interactions, emotes, hops; 17 follower script commands (d26148e7, 566793ec) | skip | — |
| 13 | Follower cutscene script edits in 100 event scripts and 41 map event files (566793ec) | skip | — |
| 14 | Surf recall/shore wait, battles, bikes, doors, warp panels, escalators, berry watering, reflections (ccbddd93, 211b80ef, 18ed578b) | skip | — |
| 15 | Follower crash/freeze fixes: double battle send-out (3f142499), wait for last walk (96f8afb0) | skip: Following Platinum code only | — |
| 16 | Scenes freezing when a walk ends during a screen effect: the movement-end handler found the script through the *current* field task (c026a1c9) | adapt | Check whether HG's movement-end handler has the same pattern; HG's own follower hops can hit it. Port the fix only if it does. | ⏳ |

## Quality of life

| # | Change | Verdict | HeartGold plan | Status |
|---|---|---|---|---|
| 17 | INSTANT text speed in Options; new games start on FAST; Options row fits (c0644cff, 9c1ccbc6) | port | HG's `Options.textSpeed` is a 4-bit field, so a fourth value fits. The row keeps its three drawn buttons as a sliding window: SLOW/MID show SLOW MID FAST, FAST/INSTANT show MID FAST INSTANT. Fixed in passing: Platinum's version leaves the last frame's characters uncopied when a message ends without a button wait. | ✅ |
| 18 | EXP bar fills twice as fast (c0644cff) | port | Two pixels per frame in the same sub-pixel code as #19. | 🟡 |
| 19 | Battle HP bars move one pixel per frame whatever the max HP (566793ec) | port | pokeheartgold already has a `FAST_HP_BARS` switch (sub-pixel steps, 1 px/frame); turn it on. | 🟡 |
| 20 | Run by default with the Running Shoes, hold B to walk (c0644cff) | skip: HeartGold has it | The Running Shoes button on the touch-screen menu toggles always-run. | — |
| 21 | Repel prompt: when a Repel wears off and another of the same kind is in the bag, offer to use it (c0644cff) | port | New script commands appended. | ⏳ |
| 22 | Endless Repel key item, given by Mom with the Running Shoes (c0644cff) | port | New item appended after `ITEM_ENIGMA_STONE` (536); Mom in New Bark gives it with the Running Shoes. | ⏳ |
| 23 | Reusable TMs (e8f1b80d) | port | | ✅ |
| 24 | Field moves without the move: Pokémon that could learn an HM you own can use it, badges still required; party menu lists them (e8f1b80d) | port | HG's field moves: Cut, Fly, Surf, Strength, Whirlpool, Rock Smash, Waterfall, Rock Climb (+ Headbutt tutor move, not an HM: left alone). | ⏳ |
| 25 | Move Tutor in every Pokémon Center: remembers and forgets moves for free (e8f1b80d) | port | HG already has the Move Relearner (Blackthorn, Heart Scale) and the Move Deleter; this adds a free one to every Center as a new common script. | ⏳ |
| 26 | Summary screen: A on the Skills page cycles stats / IVs / EVs (2957e59d) | adapt | HG's Skills and Battle Moves share a page where A selects moves, so **Select** cycles. Summary is asm: a Select check in `sub_02088B40` and the page draw routed through `src/gunk_summary.c`. Checked on party Pokémon; not yet from the PC (BoxPokemon path). | ✅ |
| 27 | Exp. All key item from Mom: Pokémon that didn't battle get half the EXP and EVs (2957e59d) | port | HG has only the held Exp. Share. New item appended. | ⏳ |
| 28 | Vitamins work up to 252 EVs (2957e59d) | port | | ✅ |
| 29 | Bag: Start sorts the open pocket (2957e59d) | port | Uses HG's existing `SortPocket` (by item number). | ✅ |
| 30 | Two registered items: newest on Y, previous on Select, in the two halves of `Bag.registeredItem` (2957e59d) | adapt | HG already stores two registered items (`registeredItems[2]`, the touch-screen buttons) and Y opens a menu to choose. Add: Select uses item 2 directly; Y keeps HG's behaviour. No save change. (Correction: Y uses item 1 directly in HG; the menu is only on the touch screen.) | ✅ |
| 31 | Honey trees say the hours until Pokémon gather (2957e59d) | skip: no honey trees in HG | — |
| 32 | Poké Radar says the chain length (2957e59d) | skip: no Poké Radar in HG | — |
| 33 | Eggs hatch twice as fast (2957e59d) | port | | 🟡 |
| 34 | Trade evolutions by level (Kadabra 36, Machoke/Graveler/Haunter 37) and trade items used like evolution stones (2957e59d) | port | Same Pokémon and items exist in HG (data in `evo.json`, `item_data.csv`). An Everstone now also stops Kadabra's level-up evolution (vanilla's Kadabra exception only made sense for trades; Gunk's Platinum still has that hole). | ✅ |
| 35 | UNLOCK FPS option (from Following Platinum, 211b80ef) | skip (ask) | Came with the follower port, not a user request; HG's field already runs at its own rate. Say if you want it. | — |

## Test infrastructure (not in the shipped game)

| # | Change | Verdict | HeartGold plan | Status |
|---|---|---|---|---|
| 36 | `gHeapAllocFails` counter for the emulator suite (9c1ccbc6) | port | | ✅ |
| 37 | Test builds: `gTestSaveMode` start points, warp anywhere, follower off (01b7f2a7, 211b80ef, 18ed578b, acfccac7) | adapt | HG test build: start in New Bark / post-game save; warp anywhere. | ⏳ |
| 38 | BizHawk harness and suite (`pokemon-platinum-dexnav/emu`) | adapt | Copy to `C:\Projects\hg-tools\emu`, point at HG ROMs and HG linker-map symbols, HG save layout in the checksum validator. | ✅ |

## Script commands and common scripts

Platinum appended 26 script commands and 6 common scripts. HeartGold needs these (the 17 follower commands are skipped):

| Platinum | HeartGold | Notes |
|---|---|---|
| OpenDexNav, DexNavStartSearch, DexNavStartBattle, DexNavPrintMessage | port | appended to `gScriptCmdTable` |
| GetReusableRepel, UseRepel, ToggleEndlessRepel | port | |
| GetHoneyTreeHoursLeft | skip | no honey trees |
| ToggleExpAll | port | |
| 17 follower commands (GetPokemonFollowerPartySlot … GetPokemonFollowerDirFromPlayer) | skip | |
| Common scripts: DexNav, DexNavBattle, DexNavMenu, EndlessRepel, MoveTutor, ExpAll | port | appended after HG's last common script |

## Test results (2026-11-02)

`C:\Projects\hg-tools\emu\suite`, BizHawk, emulator minimized. Both builds start from the real intro (title, Oak,
bedroom, Mom unlocks Save); no test hooks in the ROM.

| Test | vanilla (= retail) | Gunk's HG at 217aa5527 |
|---|---|---|
| save_cycles (3 saves, power cycle, RAM == cart, position) | pass | pass |
| crash_after_save | pass | pass |
| torn_save (crash at 9 points during a save) | pass | pass: +120f nothing written yet; +170 to +560f one torn block, falls back to the previous save; +760f the new save |
| backup_recovery (newest save damaged) | pass | pass |

Session 2 (build daf0120f1, INSTANT text + trade evolutions): the suite passes 4/4 again; with FAST text as the
new-game default the save finishes sooner (torn_save: +120 to +420f fall back to the previous save, +560f and
+760f the new save). Checked in the emulator with `mons.py` (party and bag edited in RAM): INSTANT prints Mom's whole
two-line message within 6 frames; the Metal Coat evolves Onix (Kadabra listed UNABLE); a Rare Candy to 36 evolves
Kadabra, but not while it holds an Everstone; a Protein takes Attack EVs 100 -> 110; teaching TM06 keeps the TM.

Not yet exercised in the emulator: egg hatching speed, the bar speeds in battle, the DS menu banner text.

## Plan for the remaining ports

In order; each is its own local commit, built, and run through the emulator suite (`C:\Projects\hg-tools\emu\suite`)
before the next.

1. **INSTANT text (#17).** `Options.textSpeed` is a 4-bit field, so value 3 fits without a save change. HG's options
   app reads each row's choices from msg_0045 in one sequence (`msg_0045_00009 + msgNum++`) and lays out at most 3
   label columns (`sOptionChoiceLabelXCoords[][3]`): append "INSTANT" at the end of msg_0045 and special-case the
   text-speed row instead of inserting into the sequence. Text printer: a sentinel speed that `AddTextPrinter` turns
   into "render until a button wait" per frame (Platinum's `TEXT_SPEED_INSTANT_TASK`), with a new field appended to
   `TextPrinter` (heap-allocated only in text.c, so appending is safe). New games start on FAST.
2. **Summary IV/EV view (#26).** Find HG's summary-screen Skills page (`src/application/pokemon_summary` or asm);
   A cycles stats / IVs / EVs.
3. **Trade evolutions (#34).** Level evolutions for Kadabra 36, Machoke / Graveler / Haunter 37 in the species
   evolution data; the trade items get the evolution-stone use attribute in `files/itemtool/itemdata/item_data.csv`,
   with the matching `EVO_TRADE_ITEM` entries usable via `EVOCTX_ITEM_USE`.
4. **Bag sorting (#29)** and **Select for the second registered item (#30).** HG's bag app is in
   `src/application/bag`; field input for Select needs locating (overlay 1, partly asm).
5. **Field moves without the move (#24)** and **Move Tutor in every Pokémon Center (#25).** The party menu's field-move
   list (`party_menu*.c`) plus a new common script appended after HG's last one, and an NPC added to each Pokémon
   Center's event file.
6. **New key items (#22 Endless Repel, #27 Exp. All, #21 Repel prompt).** Items appended after `ITEM_ENIGMA_STONE`
   (536) in `items.h` and `item_data.csv` (+ names, descriptions, icons); new script commands appended to
   `gScriptCmdTable`; Mom in New Bark gives them with the Running Shoes. Exp. All needs the battle EXP code
   (src/battle) to give half EXP and EVs to party members that didn't battle.
7. **21 PC boxes (#4).** First decompile-or-map every box-count dependency in overlay 14 (the PC app is assembly):
   loops to 18, wrap-arounds, and per-box arrays in its working memory. Then `NUM_BOXES` 21, `SAVE_PAGE_MAX` 35 -> 38
   (the PC block grows from 19 to 22 sectors; the extra saves shift to 38+ and still end inside the 64-sector half),
   box names 19-21 appended to msg_0024, and a full-boxes test (630 Pokémon saved, power cycled, compared) plus the
   torn-save test. Highest risk to saves, so it goes after the others are stable.
8. **DexNav (#7-#10).** Key item with a touch-screen species grid (caught / seen silhouette / unseen "?"), search,
   chains and levels; legal Method K rolls; levels saved in a new save array appended after `SAVE_PCSTORAGE` (box
   block, after #7 settles that block's size); gift from Lyra/Ethan after the Route 29 catching tutorial.
9. **Title logo (#3)** and **c026a1c9 check (#16).**

## Save budget

HeartGold (US), measured on the cart in the emulator (vanilla build):

- Each flash half is 64 sectors (0x40000). The main save is `SAVE_PAGE_MAX` = 35 sectors: the normal block is
  63,016 bytes (16 sectors) and the PC block 74,512 bytes (19 sectors), which fills all 35. The PC block starts at
  0xF700 (0x100-aligned after the normal block), not on a sector boundary.
- The extra saves sit at sectors `SAVE_PAGE_MAX + 0, 3, 4, 6, 8, 10` (Hall of Fame and Battle Frontier records/videos),
  ending around sector 47, so ~17 sectors per half are unused.
- 21 boxes need ~3 more sectors: `SAVE_PAGE_MAX` 35 → 38, extras move up 3 and still fit. `SaveData.dynamic_region`
  grows with `SAVE_PAGE_MAX`, so the save heap must have the room (checked with the heap counters in the suite).
