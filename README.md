# Pokémon Gunk's HeartGold Version

A quality-of-life hack of Pokémon HeartGold, built on [pret's HeartGold decompilation](https://github.com/pret/pokeheartgold).
It carries over the changes from Pokémon Gunk's Platinum Version. Difficulty and story stay as they are; the hack
takes out grind and friction for long playthroughs.

**Status: in development, not yet playable start to finish.** There are no releases yet. The full list of
changes, with what is done, tested and planned, is in [docs/GUNK_CHANGES.md](docs/GUNK_CHANGES.md).

## Features

Done:

- **Faster play:** an INSTANT text speed (new games start on FAST), HP and EXP bars that move faster, and eggs
  that hatch twice as fast.
- **Items:** TMs are not used up, vitamins work up to 252 EVs, Start in the bag sorts the open pocket, and
  Select uses the second registered item (Y uses the first, as in HeartGold).
- **Pokémon:** Kadabra, Machoke, Graveler and Haunter also evolve by level. The trade evolution items (King's
  Rock, Metal Coat, Dragon Scale and the rest) can be used like evolution stones.
- **Summary:** Select on the Skills page shows IVs, then EVs.

Planned: HM moves without teaching them, a Move Tutor in every
Pokémon Center, Endless Repel, Exp. All, a prompt to use another Repel, 21 PC boxes, and a DexNav.

Pokémon following you and auto-run (the Running Shoes button) come with HeartGold already.

Save safety comes first. Every build passes an emulator test suite covering saves, power cycles, crashes during
a save and damaged-save recovery.

## Play it

Releases will ship a **BPS patch**, not a ROM. You need your own dump of **Pokémon HeartGold (USA)**:

```
sha1: 4fcded0e2713dc03929845de631d0932ea2b5a37
```

1. Download the `.bps` file from [Releases](../../releases).
2. Apply it to your ROM with [Rom Patcher JS](https://www.marcrobledo.com/RomPatcher.js/) (in the browser),
   Floating IPS, or `py tools/patch/bps.py apply <heartgold.nds> <patch.bps> <out.nds>`.
3. Play the result in any DS emulator or flashcart. The patcher checks your ROM first and refuses the wrong one.

Back up your save before updating.

## Build it

Follow pret's [`INSTALL.md`](INSTALL.md), then run `make`. Because the game differs from retail, the build's
final checksum step reports a mismatch; the ROM (`build/heartgold.us/pokeheartgold.us.nds`) is still produced.
`tools/patch/bps.py make <retail.nds> <built.nds> <out.bps>` makes the patch from it.

## Credits

- [pret](https://github.com/pret/pokeheartgold) for the HeartGold decompilation this hack is built on.
- The DexNav idea comes from later Pokémon games and fan hacks such as Pokémon Unbound.

Pokémon is © Nintendo, Creatures Inc. and GAME FREAK inc. This is a non-commercial fan project and contains no ROM data.
