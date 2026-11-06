#include "gunk_dexnav_bar.h"

#include "global.h"

#include "constants/charcode.h"
#include "constants/field/map_load.h"
#include "constants/abilities.h"
#include "constants/pokemon.h"

#include "bg_window.h"
#include "field_system.h"
#include "font.h"
#include "gunk_dexnav.h"
#include "heap.h"
#include "msgdata.h"
#include "options.h"
#include "player_data.h"
#include "pm_string.h"
#include "pokemon.h"
#include "render_window.h"
#include "string_util.h"
#include "text.h"

#include "msgdata/msg.naix"

// Gunk: the DexNav's search bar (as Gunk's Platinum's). While a search runs, the field's message box shows the
// hidden Pokémon: species, level and a shiny star, the search level; its nature and ability, the chain and one
// star per perfect IV. The bar borrows the dialog box's place, tiles and frame (dialog_box.c), so it costs no
// VRAM: a script's message draws over it, and once the field is free again the bar is copied back from its own
// pixel buffer. It lives in the field's heap, so FieldMap_Exit takes it down with the field.

#define BAR_X           2
#define BAR_Y           19
#define BAR_W           27
#define BAR_H           4
#define BAR_PALETTE     12    // the dialog box's text palette
#define BAR_BASE_TILE   0x237 // the dialog box's tiles
#define BAR_FRAME_TILE  0x3E2 // the dialog box's frame
#define BAR_FRAME_PAL   10
#define BAR_PERFECT_IVS 6

#define BAR_TEXT  MAKE_TEXT_COLOR(1, 2, 15)
#define BAR_RED   MAKE_TEXT_COLOR(3, 4, 15)

static Window sBar;
static BOOL sBarShown;
static u32 sBarKey; // what the bar was drawn for: a new roll or chain redraws the text

static void AppendAscii(String *dst, const char *src) {
    for (; *src != '\0'; src++) {
        char c = *src;
        if (c >= 'A' && c <= 'Z') {
            String_AddChar(dst, CHAR_A + (c - 'A'));
        } else if (c >= 'a' && c <= 'z') {
            String_AddChar(dst, CHAR_a + (c - 'a'));
        } else if (c >= '0' && c <= '9') {
            String_AddChar(dst, CHAR_0 + (c - '0'));
        } else if (c == '.') {
            String_AddChar(dst, CHAR_PERIOD);
        } else {
            String_AddChar(dst, CHAR_SPACE);
        }
    }
}

static void AppendNumber(String *dst, int num) {
    String *numStr = String_New(8, HEAP_ID_FIELD1);
    String16_FormatInteger(numStr, num, 3, PRINTING_MODE_LEFT_ALIGN, TRUE);
    String_Cat(dst, numStr);
    String_Delete(numStr);
}

static void AppendFromBank(String *dst, int bank, int index) {
    String *entry = ReadMsgData_NewNarc_NewString(NARC_msgdata_msg, bank, index, HEAP_ID_FIELD1);
    String_Cat(dst, entry);
    String_Delete(entry);
}

static void Print(String *string, int x, int y, u32 color) {
    AddTextPrinterParameterizedWithColor(&sBar, FONT_SYSTEM, string, x, y, TEXT_SPEED_NOTRANSFER, color, NULL);
}

// right-aligned against the bar's inner edge
static void PrintRight(String *string, int y) {
    Print(string, BAR_W * 8 - FontID_String_GetWidth(FONT_SYSTEM, string, 0) - 2, y, BAR_TEXT);
}

static u32 BarKey(GunkDexNavState *s) {
    return s->personality ^ ((u32)s->chain << 16) ^ s->species ^ ((u32)s->level << 8);
}

static void DrawText(GunkDexNavState *s) {
    FillWindowPixelBuffer(&sBar, 15);

    // line 1: species and level (a red star if shiny), then the search level
    String *line = String_New(96, HEAP_ID_FIELD1);
    String *name = GetSpeciesName(s->species, HEAP_ID_FIELD1);
    String_Cat(line, name);
    String_Delete(name);
    AppendAscii(line, "  Lv.");
    AppendNumber(line, s->level);
    Print(line, 0, 0, BAR_TEXT);
    if (s->shiny) {
        u32 width = FontID_String_GetWidth(FONT_SYSTEM, line, 0);
        String_SetEmpty(line);
        String_AddChar(line, CHAR_STAR);
        Print(line, width + 6, 0, BAR_RED);
    }
    String_SetEmpty(line);
    AppendAscii(line, "Search Lv.");
    AppendNumber(line, s->searchLevel);
    PrintRight(line, 0);

    // line 2: nature and ability, then the chain and one star per perfect IV
    String_SetEmpty(line);
    AppendFromBank(line, msg_0034, GetNatureFromPersonality(s->personality));
    AppendAscii(line, "  ");
    int ability = GetMonBaseStat(s->species, BASE_ABILITY_1);
    if ((s->personality & 1) && GetMonBaseStat(s->species, BASE_ABILITY_2) != ABILITY_NONE) {
        ability = GetMonBaseStat(s->species, BASE_ABILITY_2);
    }
    AppendFromBank(line, msg_0720, ability);
    Print(line, 0, 16, BAR_TEXT);

    String_SetEmpty(line);
    AppendAscii(line, "Chain ");
    AppendNumber(line, s->chain);
    String_AddChar(line, CHAR_SPACE);
    String_AddChar(line, CHAR_SPACE);
    for (int i = 0; i < BAR_PERFECT_IVS; i++) {
        String_AddChar(line, i < s->perfectIVs ? CHAR_STAR : CHAR_HYPHEN);
    }
    PrintRight(line, 16);
    String_Delete(line);
}

static void Remove(void) {
    ClearFrameAndWindow2(&sBar, FALSE);
    RemoveWindow(&sBar);
    sBarShown = FALSE;
}

// Every frame (FieldSystem_Control). The bar is only drawn, redrawn or cleared while the field is free, so it
// never fights a script's message box for the same tiles.
void GunkDexNavBar_Update(FieldSystem *fieldSystem) {
    GunkDexNavState *s = &gGunkDexNav;
    if (fieldSystem->bgConfig == NULL || !fieldSystem->runningFieldMap || fieldSystem->taskman != NULL) {
        return;
    }
    BOOL want = s->active && fieldSystem->mapLoadType == MAP_LOAD_TYPE_OVERWORLD;
    if (!want) {
        if (sBarShown) {
            Remove();
        }
        return;
    }
    if (!sBarShown) {
        AddWindowParameterized(fieldSystem->bgConfig, &sBar, GF_BG_LYR_MAIN_3, BAR_X, BAR_Y, BAR_W, BAR_H, BAR_PALETTE, BAR_BASE_TILE);
        LoadUserFrameGfx2(fieldSystem->bgConfig, GF_BG_LYR_MAIN_3, BAR_FRAME_TILE, BAR_FRAME_PAL, Options_GetFrame(Save_PlayerData_GetOptionsAddr(fieldSystem->saveData)), HEAP_ID_FIELD1);
        sBarShown = TRUE;
        sBarKey = BarKey(s) + 1;
    }
    if (sBarKey != BarKey(s)) {
        sBarKey = BarKey(s);
        DrawText(s);
    }
    // a message box may have borrowed the tiles: the pixel buffer in RAM is untouched, so copy it back
    DrawFrameAndWindow2(&sBar, TRUE, BAR_FRAME_TILE, BAR_FRAME_PAL);
    ScheduleWindowCopyToVram(&sBar);
}

// FieldMap_Exit: the window's buffers are in the field's heap, which is about to go.
void GunkDexNavBar_OnFieldExit(FieldSystem *fieldSystem) {
    if (sBarShown) {
        RemoveWindow(&sBar);
        sBarShown = FALSE;
    }
}
