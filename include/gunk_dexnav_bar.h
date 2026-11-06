#ifndef POKEHEARTGOLD_GUNK_DEXNAV_BAR_H
#define POKEHEARTGOLD_GUNK_DEXNAV_BAR_H

#include "field_system.h"

// Gunk: the DexNav's search bar on the field (gunk_dexnav_bar.c, in the main binary)
void GunkDexNavBar_Update(FieldSystem *fieldSystem);
void GunkDexNavBar_OnFieldExit(FieldSystem *fieldSystem);

#endif // POKEHEARTGOLD_GUNK_DEXNAV_BAR_H
