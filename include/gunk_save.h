#ifndef POKEHEARTGOLD_GUNK_SAVE_H
#define POKEHEARTGOLD_GUNK_SAVE_H

#include "constants/species.h"

#include "save.h"

// Gunk's own save chunk (SAVE_GUNK, in the normal block). The block had about 2.5 KB to spare before needing a
// 17th sector; this takes 1 KB and keeps room for later features.
typedef struct GunkSave {
    u8 dexNavSearchLevel[MAX_SPECIES + 1]; // per species, raised by each DexNav encounter
    u8 padding_1EE[2];
    u16 dexNavSpecies; // the species the DexNav searches for with R
    u8 reserved[0x400 - (MAX_SPECIES + 1) - 2 - 2];
} GunkSave;

u32 GunkSave_sizeof(void);
void GunkSave_Init(GunkSave *gunkSave);
GunkSave *Save_GunkSave_Get(SaveData *saveData);

#endif // POKEHEARTGOLD_GUNK_SAVE_H
