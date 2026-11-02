#include "gunk_save.h"

#include "global.h"

#include "constants/save_arrays.h"

#include "gunk_dexnav.h"

u32 GunkSave_sizeof(void) {
    return sizeof(GunkSave);
}

void GunkSave_Init(GunkSave *gunkSave) {
    MI_CpuClear8(gunkSave, sizeof(GunkSave));
}

GunkSave *Save_GunkSave_Get(SaveData *saveData) {
    return SaveArray_Get(saveData, SAVE_GUNK);
}

GunkDexNavState gGunkDexNav; // gunk_dexnav.h
