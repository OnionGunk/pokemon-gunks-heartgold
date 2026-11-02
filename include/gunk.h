#ifndef POKEHEARTGOLD_GUNK_H
#define POKEHEARTGOLD_GUNK_H

#include "constants/vars.h"

#include "save.h"
#include "save_vars_flags.h"

// Gunk's switches, kept in VAR_GUNK_SETTINGS (an unused var, so the save layout is unchanged)
#define GUNK_SETTING_ENDLESS_REPEL (1 << 0) // the Endless Repel is switched on
#define GUNK_SETTING_EXP_ALL       (1 << 1) // the Exp. All is switched on

static inline BOOL Gunk_IsSettingOn(SaveData *saveData, u16 setting) {
    return (*Save_VarsFlags_GetVarAddr(Save_VarsFlags_Get(saveData), VAR_GUNK_SETTINGS) & setting) != 0;
}

// switches a setting over and returns whether it is now on
static inline BOOL Gunk_ToggleSetting(SaveData *saveData, u16 setting) {
    u16 *settings = Save_VarsFlags_GetVarAddr(Save_VarsFlags_Get(saveData), VAR_GUNK_SETTINGS);
    *settings ^= setting;
    return (*settings & setting) != 0;
}

// set from the save before each battle (battle_setup.c); the Exp. All's share is given in battle_command.c
extern BOOL gGunkExpAllOn;

#endif // POKEHEARTGOLD_GUNK_H
