#ifndef POKEHEARTGOLD_GUNK_DEXNAV_APP_H
#define POKEHEARTGOLD_GUNK_DEXNAV_APP_H

#include "overlay_manager.h"
#include "save.h"
#include "script.h"

// Gunk: the DexNav screen (laid out like Gunk's Platinum's, in HeartGold's touch-screen style). The top screen
// shows the highlighted Pokémon and the search target; the touch screen has the area's LAND and WATER Pokémon in
// two 6x2 grids. Choosing a Pokémon seen in the grass or while surfing closes the screen and starts a search.

#define DEXNAV_GRID_COLS     6
#define DEXNAV_GRID_ROWS     2
#define DEXNAV_SECTION_SLOTS (DEXNAV_GRID_COLS * DEXNAV_GRID_ROWS)
#define DEXNAV_TOTAL_SLOTS   (DEXNAV_SECTION_SLOTS * 2)

enum GunkDexNavMethod {
    DEXNAV_METHOD_GRASS,
    DEXNAV_METHOD_SWARM,
    DEXNAV_METHOD_HOENN_SOUND,
    DEXNAV_METHOD_SINNOH_SOUND,
    DEXNAV_METHOD_SURF,
    DEXNAV_METHOD_SURF_SWARM,
    DEXNAV_METHOD_OLD_ROD,
    DEXNAV_METHOD_GOOD_ROD,
    DEXNAV_METHOD_SUPER_ROD,
};

typedef struct GunkDexNavEntry {
    u16 species;
    u8 method;
    u8 minLevel;
    u8 maxLevel;
} GunkDexNavEntry;

// Filled by the field before the screen opens: slots 0-11 are land, 12-23 water.
typedef struct GunkDexNavArgs {
    SaveData *saveData;
    GunkDexNavEntry slots[DEXNAV_TOTAL_SLOTS];
    u16 chosen; // out: the Pokémon to search for, or SPECIES_NONE
    // info (HG's swarms and radio, as Gunk's Platinum shows honey trees and the Poké Radar)
    u16 swarmSpecies; // today's swarm, once a phone call has started one; SPECIES_NONE if none
    u16 swarmMapId;
    u8 radio;         // DEXNAV_RADIO_*: a sound show the Pokégear is playing, which changes some grass Pokémon
} GunkDexNavArgs;

enum {
    DEXNAV_RADIO_OFF,
    DEXNAV_RADIO_HOENN,
    DEXNAV_RADIO_SINNOH,
};

extern const OverlayManagerTemplate gOverlayTemplate_GunkDexNav;

BOOL GunkDexNavApp_ScriptWait(ScriptContext *ctx);

#endif // POKEHEARTGOLD_GUNK_DEXNAV_APP_H
