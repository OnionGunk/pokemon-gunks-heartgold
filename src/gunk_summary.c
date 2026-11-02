#include "global.h"

#include "constants/pokemon.h"
#include "constants/sndseq.h"

#include "msgdata/msg/msg_0302.h"

#include "bg_window.h"
#include "font.h"
#include "msgdata.h"
#include "pm_string.h"
#include "pokemon.h"
#include "string_util.h"
#include "text.h"
#include "unk_02005D10.h"
#include "unk_02088288.h"

// Gunk: Select on the summary's Skills page cycles the stat numbers through the stats, the IVs and the EVs.
// The summary screen is still assembly (asm/unk_02088288.s, asm/unk_0208C3E4.s); `work` is its app data:
//   +0x224 Window *pageWindows: [0] HP, [1] Attack, [2] Defense, [3] Sp. Atk, [4] Sp. Def, [5] Speed, [6] ability
//          name, [7] ability description
//   +0x22C PokemonSummaryArgs *args (unk11: 0 Pokemon array, 1 Party, 2 BoxPokemon array)
//   +0x7A0 MsgData *msg_0302, +0x7BC s8 page (1 = Skills and Battle Moves)

enum {
    SKILLS_DISPLAY_STATS,
    SKILLS_DISPLAY_IVS,
    SKILLS_DISPLAY_EVS,
    SKILLS_DISPLAY_MAX,
};

#define SUMMARY_PAGE_SKILLS 1
#define SUMMARY_TEXT_COLOR  MAKE_TEXT_COLOR(1, 2, 0)

void sub_0208D178(u8 *work);                                                // draws the Skills page
void *sub_0208A520(u8 *work);                                               // the Pokemon or BoxPokemon shown
void sub_0208C87C(u8 *work, u32 msgId, u32 value, u32 digits, u32 padMode); // formats a number into work+0x7AC
void sub_0208C778(u8 *work, Window *window, u32 color, u32 align);          // prints work+0x7AC (align 1 right, 2 center)

// called from the summary's assembly
void GunkSummary_DrawSkills(u8 *work);
void GunkSummary_CycleSkillsDisplay(u8 *work);

static u8 sSkillsDisplay;

static u32 GetShownMonData(u8 *work, int attr) {
    PokemonSummaryArgs *args = *(PokemonSummaryArgs **)(work + 0x22C);
    void *mon = sub_0208A520(work);

    if (args->unk11 == 2) {
        return GetBoxMonData(mon, attr, NULL);
    }
    return GetMonData(mon, attr, NULL);
}

void GunkSummary_DrawSkills(u8 *work) {
    // window order: HP, Attack, Defense, Sp. Atk, Sp. Def, Speed
    static const u8 sIVs[6] = { MON_DATA_HP_IV, MON_DATA_ATK_IV, MON_DATA_DEF_IV, MON_DATA_SPATK_IV, MON_DATA_SPDEF_IV, MON_DATA_SPEED_IV };
    static const u8 sEVs[6] = { MON_DATA_HP_EV, MON_DATA_ATK_EV, MON_DATA_DEF_EV, MON_DATA_SPATK_EV, MON_DATA_SPDEF_EV, MON_DATA_SPEED_EV };

    sub_0208D178(work);
    if (sSkillsDisplay == SKILLS_DISPLAY_STATS) {
        return;
    }

    Window *windows = *(Window **)(work + 0x224);
    const u8 *attrs = sSkillsDisplay == SKILLS_DISPLAY_IVS ? sIVs : sEVs;
    for (int i = 0; i < 6; i++) {
        FillWindowPixelBuffer(&windows[i], 0);
        sub_0208C87C(work, msg_0302_00121, GetShownMonData(work, attrs[i]), 3, PRINTING_MODE_LEFT_ALIGN);
        sub_0208C778(work, &windows[i], SUMMARY_TEXT_COLOR, i == 0 ? 2 : 1);
        ScheduleWindowCopyToVram(&windows[i]);
    }

    // the ability description says what the numbers are
    String *label = NewString_ReadMsgData(*(MsgData **)(work + 0x7A0), sSkillsDisplay == SKILLS_DISPLAY_IVS ? msg_0302_00195 : msg_0302_00196);
    FillWindowPixelBuffer(&windows[7], 0);
    AddTextPrinterParameterizedWithColor(&windows[7], FONT_SYSTEM, label, 3, 0, TEXT_SPEED_NOTRANSFER, SUMMARY_TEXT_COLOR, NULL);
    String_Delete(label);
    ScheduleWindowCopyToVram(&windows[7]);
}

// called by the summary's input handler when Select is pressed on the Skills page
void GunkSummary_CycleSkillsDisplay(u8 *work) {
    sSkillsDisplay = (sSkillsDisplay + 1) % SKILLS_DISPLAY_MAX;
    PlaySE(SEQ_SE_DP_SELECT);
    GunkSummary_DrawSkills(work);
}
