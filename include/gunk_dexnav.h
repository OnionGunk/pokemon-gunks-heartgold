#ifndef POKEHEARTGOLD_GUNK_DEXNAV_H
#define POKEHEARTGOLD_GUNK_DEXNAV_H

#include "field_system.h"

// Gunk: the DexNav key item. Its menu lists the current area's grass Pokémon; picking one (or R, for the last
// one picked) hides it in tall grass near the player, where a patch rustles. Stepping into the patch starts the
// battle. The Pokémon is rolled as a real HeartGold grass encounter (Method K), keeping the best of several rolls:
// higher search levels (per species, saved) and longer chains (same species and map) give better IVs and more
// shiny rolls.

#define DEXNAV_MENU_MAX 7 // species in the menu (the field menu has no scrolling)
#define DEXNAV_SEARCH_FRAMES 300 // a search lasts 10 seconds in the field (30 frames a second); then the Pokémon gets away

enum {
    DEXNAV_SEARCH_STARTED,
    DEXNAV_SEARCH_NOT_HERE,  // the species isn't in this area's grass or water right now
    DEXNAV_SEARCH_NO_GRASS,  // no tall grass close enough
    DEXNAV_SEARCH_NONE_CHOSEN, // R before choosing a Pokémon on the DexNav screen
    DEXNAV_SEARCH_NO_WATER,    // surfing, no water close enough
    DEXNAV_SEARCH_SURF_ONLY,   // on land: the species is only met by surfing here
    DEXNAV_SEARCH_GRASS_ONLY,  // surfing: the species is only met in the grass here
};

// Kept in the main binary so it survives the field overlays unloading for menus and battles.
typedef struct GunkDexNavState {
    u16 species;
    u8 level;
    u8 searchLevel;
    BOOL active;
    int x;
    int z;
    u32 mapId;
    u32 personality;
    u8 ivs[6];
    u8 perfectIVs;
    u8 shiny;
    u16 chainSpecies;
    u16 chain;
    u32 chainMapId;
    u16 frames;       // since the patch last shook
    u16 searchFrames; // since the search started, in the field (it ends at DEXNAV_SEARCH_FRAMES)
    BOOL inBattle;    // the wild battle running is the DexNav's (running from it breaks the chain: encounter.c)
    BOOL water;       // a Surf search: the patch is on water and the Pokémon comes from the surf table
} GunkDexNavState;

extern GunkDexNavState gGunkDexNav;

// encounter_check.c (overlay 2, loaded with the field)
int GunkDexNav_ListSpecies(FieldSystem *fieldSystem, u16 *species, int max);
struct GunkDexNavArgs;
void GunkDexNav_BuildArea(FieldSystem *fieldSystem, struct GunkDexNavArgs *args);
int GunkDexNav_TryStart(FieldSystem *fieldSystem, u16 species);
void GunkDexNav_Update(FieldSystem *fieldSystem);

#endif // POKEHEARTGOLD_GUNK_DEXNAV_H
