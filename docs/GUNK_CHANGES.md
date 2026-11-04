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
| 1 | Name: "Pokémon Gunk's Platinum Version" in the DS menu banner and end credits (8fa30cda) | port | Banner "Pokémon Gunk's" / "HeartGold Version"; staff credits title "Pokémon Gunk’s HeartGold Version". Banner checked in the ROM header (DS menu text); the credits need a finished game to see. | 🟡 |
| 2 | ROM header title `GUNK PL` (3f119584); game code kept as CPUE so emulators pick the right save type | port | Header title `GUNK HG` (checked in the ROM), game code stays `IPKE`. | ✅ |
| 3 | Title-screen logo "Gunk's Platinum Version" (24f96c51) | adapt | The user's art, scaled to 189x118 in the 256x120 logo layer, 127 colours (palette entries 0-127; 128-255 belong to the sunburst behind it). Converter: `C:\Projects\gunk-hg-art\convert_logo.py`. Checked on the title screen. | ✅ |

## Save and storage

| # | Change | Verdict | HeartGold plan | Status |
|---|---|---|---|---|
| 4 | 21 PC boxes, up from 18: a living dex through Gen 4 plus 3 spare (7713e967) | adapt | **24 boxes** (the user chose as many as reasonable; 24 fills the PC's 6-per-page box list). `NUM_BOXES` 24, `SAVE_PAGE_MAX` 41, SaveData's heap +0x6000, the PC app's box wraps and thumbnail buffer, the battle PC-full check, and the GTS / Pokéathlon / Pokéwalker pickers (18 meant the party). See PC_BOXES_PLAN.md. Checked: boxes 19-24 in the PC, the 4-page box list, 720 Pokémon saved and reloaded, save tests. GTS and Pokéwalker can't run in an emulator. | ✅ |
| 5 | Save table grown for DexNav: per-species search levels stored after the PC boxes in the boxes block (5985a222) | adapt | The per-species search levels (and the R species) live in a new 1 KB chunk `SAVE_GUNK` in the normal block (gunk_save.h), not after the PC boxes. Checked by the save suite (save_cycles compares the whole save) and the DexNav play test. | ✅ |
| 6 | Static check that the main save plus extra saves fit in one flash half | port | Build-time checks in src/save_arrays.c: the extra saves end inside the 64-sector half, and the PC block plus the 16-sector normal block fit in `SAVE_PAGE_MAX`. Checked: with 25 boxes the build fails at the PC check. | ✅ |

## DexNav

| # | Change | Verdict | HeartGold plan | Status |
|---|---|---|---|---|
| 7 | DexNav key item: Land/Water species menu, R searches for the selected species, chaining, search levels, top-screen search bar (5985a222) | adapt | **Key item DexNav (539)**, registerable (Y). Its menu lists the current area's grass Pokémon for the time of day, swarm and radio (● caught, ○ seen, ????? unseen; up to 7). Picking one hides it in tall grass 2-4 tiles away (the patch shakes twice a second; after 10 seconds in the field the Pokémon "got away" and the chain resets); stepping in starts the battle. **R** searches again for the last pick. Instead of Platinum's persistent search bar, a message gives level, search level, chain and perfect IVs (no HUD window, so nothing to tear down when the field unloads; the Platinum fix d87b19258 is about that window). The search state lives in the main binary, so chains survive battles and menus; leaving the map or walking 8 tiles away ends it. Only on the normal overworld (not Safari Zone or Bug-Catching Contest). Checked: menu, search message, rustle, battle on stepping in, search level and chain 1 after one encounter. | ✅ |
| 8 | Legal encounters: one Method J RNG sequence (941125b5) | adapt | Method K exactly as HG's wild encounters roll it (slot % 100, nature % 25, PID until the nature matches, IVs); the level is the slot's (HG's grass slots have one level). Best of 1 + level/2 + 2×chain rolls; shiny chance as Platinum. Built cheaply so R never stalls: division-free mods, only the winner's frame built, and a shiny found by searching backwards from shiny PID pairs (same RNG sequence). Worst case at search level 255 / chain 999 on a 1% slot: 24 frames from R to the patch (was over 400). **PKHeX: 30/30 legal** (6 each: wild, DexNav, level 255, chain 999, shiny); an independent Method K check finds the grass frame for every DexNav catch. | ✅ |
| 9 | DexNav Pokétch app: area species, caught in color / seen as silhouette / unseen as "?", tap to target (b5551c4f, 7890b973) | adapt | Plan (b): the key item's own menu (a field list, not a Pokétch grid; no silhouettes). Search levels are saved per species in a new 1 KB save chunk `SAVE_GUNK` (normal block, still 16 sectors; PCStorage moved to id 42; old saves don't load). | ✅ |
| 10 | DexNav gift from Dawn after the Route 202 catching tutorial (b5551c4f) | adapt | Given by the Guide Gent in Cherrygrove with the Running Shoes (with the Endless Repel and Exp. All); checked by story_sweep. | ✅ |

## Followers

| # | Change | Verdict | Status |
|---|---|---|---|
| 11 | Following Platinum's Pokémon followers: 566 sprites, renderer, timing, shiny palettes, spawn exclusions (5985a222) | skip: HeartGold has followers | — |
| 12 | Follower interactions, emotes, hops; 17 follower script commands (d26148e7, 566793ec) | skip | — |
| 13 | Follower cutscene script edits in 100 event scripts and 41 map event files (566793ec) | skip | — |
| 14 | Surf recall/shore wait, battles, bikes, doors, warp panels, escalators, berry watering, reflections (ccbddd93, 211b80ef, 18ed578b) | skip | — |
| 15 | Follower crash/freeze fixes: double battle send-out (3f142499), wait for last walk (96f8afb0) | skip: Following Platinum code only | — |
| 16 | Scenes freezing when a walk ends during a screen effect: the movement-end handler found the script through the *current* field task (c026a1c9) | adapt | Check whether HG's movement-end handler has the same pattern; HG's own follower hops can hit it. Port the fix only if it does. **HG has the same pattern** (`_RunObjectEventMovement` in `src/scrcmd_c.c`); ported: the walk keeps its script environment. Checked: the intro and Lyra's scene outside the house run, suite 4/4. | ✅ |

## Quality of life

| # | Change | Verdict | HeartGold plan | Status |
|---|---|---|---|---|
| 17 | INSTANT text speed in Options; new games start on FAST; Options row fits (c0644cff, 9c1ccbc6) | port | HG's `Options.textSpeed` is a 4-bit field, so a fourth value fits. The row keeps its three drawn buttons as a sliding window: SLOW/MID show SLOW MID FAST, FAST/INSTANT show MID FAST INSTANT. Fixed in passing: Platinum's version leaves the last frame's characters uncopied when a message ends without a button wait. | ✅ |
| 18 | EXP bar fills twice as fast (c0644cff) | port | Two pixels per frame in the same sub-pixel code as #19. Checked (bars_check.py, a Lv. 5 Kadabra beating a Rattata): the EXP bar fills at 0.97 px/frame against vanilla's 0.50. | ✅ |
| 19 | Battle HP bars move one pixel per frame whatever the max HP (566793ec) | port | pokeheartgold already has a `FAST_HP_BARS` switch (sub-pixel steps, 1 px per bar update); turned on. Checked: 48 px in 94 frames, one pixel per update (the bars update at 30 Hz, as in vanilla). | ✅ |
| 20 | Run by default with the Running Shoes, hold B to walk (c0644cff) | skip: HeartGold has it | The Running Shoes button on the touch-screen menu toggles always-run. | — |
| 21 | Repel prompt: when a Repel wears off and another of the same kind is in the bag, offer to use it (c0644cff) | port | Script command 853 `GunkUseRepel` appended; the kind last used is kept in `VAR_GUNK_LAST_REPEL` (`VAR_UNK_416E`, in the unused end of the var array: no save change). Checked: a Super Repel used from the bag, counter set to 1, one step: "Use another Super Repel?" YES used one (200 steps, one fewer in the bag). | ✅ |
| 22 | Endless Repel key item, given by Mom with the Running Shoes (c0644cff) | adapt | Item 537 (Max Repel icon). In HG the Running Shoes come from the **Guide Gent** in Cherrygrove, so he gives it (checked by story_sweep: the scene ends as in vanilla and gives all three key items). USE in the bag switches it; the switch is a bit in `VAR_GUNK_SETTINGS` (`VAR_UNK_416F`). Checked: 120 steps in Route 29 grass with it on and no encounter; switched off, a battle within 3 steps. | ✅ |
| 23 | Reusable TMs (e8f1b80d) | port | | ✅ |
| 24 | Field moves without the move: Pokémon that could learn an HM you own can use it, badges still required; party menu lists them (e8f1b80d) | port | HG's field moves: Cut, Fly, Surf, Strength, Whirlpool, Rock Smash, Waterfall, Rock Climb (+ Headbutt tutor move, not an HM: left alone). Checked: the party menu lists Strength and Rock Smash for an Onix that knows neither (HM04/HM06 owned), the badge is still required, and a Totodile without Surf surfs from New Bark's pond. | ✅ |
| 25 | Move Tutor in every Pokémon Center: remembers and forgets moves for free (e8f1b80d) | port | HG already has the Move Relearner (Blackthorn, Heart Scale) and the Move Deleter; **adapted:** HG's Centers differ in layout, so instead of an NPC the Center PC's menu (`std_pokecenter_pc`, scr_seq_0003) gets MOVE TUTOR: REMEMBER (the Relearner, no Heart Scale) and FORGET (the Deleter). Checked in Cherrygrove's Center: remembered Water Gun, forgot Tackle, one-move refusal, back to the PC menu. The bedroom PC has its own script and no tutor. | ✅ |
| 26 | Summary screen: A on the Skills page cycles stats / IVs / EVs (2957e59d) | adapt | HG's Skills and Battle Moves share a page where A selects moves, so **Select** cycles. Summary is asm: a Select check in `sub_02088B40` and the page draw routed through `src/gunk_summary.c`. Checked on party Pokémon; not yet from the PC (BoxPokemon path). | ✅ |
| 27 | Exp. All key item from Mom: Pokémon that didn't battle get half the EXP and EVs (2957e59d) | adapt | Item 538 (Exp. Share icon), given by the Guide Gent too (checked by story_sweep); switched in the bag. Battle: `gGunkExpAllOn` set from the save in `battle_setup.c`; `Task_GetExp` gives non-battlers half the base EXP and the EVs (as Platinum). Checked: Pidgey Lv4 beaten by Kadabra: Kadabra +31 EXP, the Onix that didn't battle +15 EXP, both +1 Speed EV. | ✅ |
| 28 | Vitamins work up to 252 EVs (2957e59d) | port | | ✅ |
| 29 | Bag: Start sorts the open pocket (2957e59d) | port | Uses HG's existing `SortPocket` (by item number). | ✅ |
| 30 | Two registered items: newest on Y, previous on Select, in the two halves of `Bag.registeredItem` (2957e59d) | adapt | HG already stores two registered items (`registeredItems[2]`, the touch-screen buttons) and Y opens a menu to choose. Add: Select uses item 2 directly; Y keeps HG's behaviour. No save change. (Correction: Y uses item 1 directly in HG; the menu is only on the touch screen.) | ✅ |
| 31 | Honey trees say the hours until Pokémon gather (2957e59d) | skip: no honey trees in HG | — |
| 32 | Poké Radar says the chain length (2957e59d) | skip: no Poké Radar in HG | — |
| 33 | Eggs hatch twice as fast (2957e59d) | port | Egg cycles count down by 2 instead of 1. Checked: an egg's cycles went 10 → 8 where vanilla goes 10 → 9 (egg_check.py). | ✅ |
| 34 | Trade evolutions by level (Kadabra 36, Machoke/Graveler/Haunter 37) and trade items used like evolution stones (2957e59d) | port | Same Pokémon and items exist in HG (data in `evo.json`, `item_data.csv`). An Everstone now also stops Kadabra's level-up evolution (vanilla's Kadabra exception only made sense for trades; Gunk's Platinum still has that hole). | ✅ |
| 39 | Quick Run and Quick Ball (new for HG) | new | In a wild battle's command menu **R** runs and **L** throws the last ball thrown, shown in a small widget with its count. The last ball is a u16 in `SAVE_GUNK`'s reserved bytes (0 before any), so **older Gunk saves carry over**. No widget with no balls (R still runs); L and R do nothing in trainer battles. Checked by quick_ball.py, including after a save and power cycle. | ✅ |
| 35 | UNLOCK FPS option (from Following Platinum, 211b80ef) | skip (ask) | Came with the follower port, not a user request; HG's field already runs at its own rate. Say if you want it. | — |

## Test infrastructure (not in the shipped game)

| # | Change | Verdict | HeartGold plan | Status |
|---|---|---|---|---|
| 36 | `gHeapAllocFails` counter for the emulator suite (9c1ccbc6) | port | | ✅ |
| 37 | Test builds: `gTestSaveMode` start points, warp anywhere, follower off (01b7f2a7, 211b80ef, 18ed578b, acfccac7) | adapt | Done in the test harness instead of the ROM, so the release build carries no test code. hg-tools `milestones.py` starts a run at any point of the story and walks in through the door, so NPCs and entry scripts load. Milestones: Morty, Jasmine, Pryce, boarding the S.S. Aqua, the cabin, stepping off in Kanto, and Lance through the Hall of Fame, the credits and waking up at home. Each is compared flag for flag with vanilla and saved and reloaded: 7/7 pass. No follower to switch off in HG. | ✅ |
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

Play tests (`suite.py --rom gunk play`, build 06070ef25), from a Route 29 grass savestate made from the real intro:

| Test | Result |
|---|---|
| wild_battles: 120 grass steps, Exp. All on, every battle fought | pass: 8 battles; field heaps flat after the first three |
| dexnav_search: R search kept across the Bag, party, Trainer Card; patch battle; walk to New Bark | pass: chain 1 / search level 1 after the battle; search ends on the new map; heaps flat |
| monkey: random walking, A/B, menus, touch, R / Y / Select | pass: 25 rounds (seed 1234) and 40 rounds (seed 99) |
| everstone_kadabra: Rare Candy to 36 | pass: evolves; holding an Everstone it doesn't |

More save tests (`suite.py --rom gunk saves`), ported from Gunk's Platinum suite:

| Test | Result |
|---|---|
| full_boxes: all 720 slots (24 boxes) filled, save, power cycle | pass: boxes identical on the cart and after reloading |
| backup_recovery_older: the backup half damaged | pass: the newest save loads (status 1, as vanilla), the next save repairs both halves |
| torn_save_fine: crash every 30 frames through a save, 24 points | pass: +110 to +500f the previous save, +530f on the new one; always loadable |
| new_game: the real intro, save, power cycle | pass: continued where saved |
| hall_of_fame: Continue into the Hall of Fame room | pass: record valid in both halves, credits, wake up at home, post-game save valid with the record |
| map_sweep: Continue onto 30 + 45 maps spread over the game | pass: Goldenrod Tunnel B2F freezes when entered by save edit, identically on vanilla (not a Gunk bug) |

Story sweep (`suite.py --rom gunk story_sweep`): every story trigger in the maps whose scripts Gunk changed
(Cherrygrove) played on Gunk and on vanilla from the same setup and compared (map, position, every var and flag):
the Guide Gent's tour (value 0) and the north trigger (value 1) end exactly as in vanilla, and the Guide Gent gives
the Endless Repel, Exp. All and DexNav. Value 3 freezes identically on both ROMs from this setup (skipped). No
regressions.

The extra saves (Hall of Fame, Frontier, battle videos) sit at `SAVE_PAGE_MAX` + n, which Gunk hasn't changed, so
unlike Platinum they don't move; the Hall of Fame test covers them.

Found and fixed: the DexNav patch didn't trigger during the encounter-free steps right after a script (the R
search itself), so a patch two steps away could be walked over.

## DexNav, Quick Ball and gate tests (2026-11-03)

| Test | Result |
|---|---|
| dexnav_legality.py: 6 catches per kind, each from a different RNG seed, checked by PKHeX (`hg-tools/legality`) | 30 valid, 0 illegal; 6 different PIDs per kind; shinies via repeated level 255 / chain 999 searches |
| dexnav_timing.py: 150 searches at level 255, chain 999 | 15% slot: 5-6 frames R to patch (shiny up to 10); 1% slot: 8-9 (shiny up to 24) |
| dexnav_rules.py | the patch shakes every 30 emulator frames; still searching at 9 s, "got away" by 11 s with the chain 0; a wild Rattata kept a Rattata chain of 5, a Hoothoot reset it |
| quick_ball.py | widget, L throws the last ball, R runs, kept across a save and power cycle; none in trainer battles |
| cherrygrove_gate.py: every row z 388-410 walking west from Route 29 | only z 398-400 get past the tour's column, and they start the tour, which gives the DexNav: nobody reaches Violet's gym without it |

The field runs at 30 frames a second (HG's main loop draws every other vblank), so field timers count half the
emulator's frames. The save layout is unchanged apart from the Quick Ball field in reserved bytes: Gunk saves from
earlier builds load.

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
7. **PC boxes (#4):** done, 24 boxes.
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
