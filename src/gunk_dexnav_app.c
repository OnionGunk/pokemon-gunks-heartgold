#include "gunk_dexnav_app.h"

#include "global.h"

#include "constants/charcode.h"
#include "constants/sndseq.h"
#include "constants/species.h"

#include "bg_window.h"
#include "filesystem_files_def.h"
#include "font.h"
#include "gf_gfx_loader.h"
#include "gf_gfx_planes.h"
#include "gunk_dexnav.h"
#include "gunk_save.h"
#include "heap.h"
#include "main.h"
#include "overlay_manager.h"
#include "pm_string.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_icon_idx.h"
#include "screen_fade.h"
#include "script.h"
#include "task.h"
#include "sprite.h"
#include "sprite_system.h"
#include "string_util.h"
#include "system.h"
#include "text.h"
#include "touchscreen.h"
#include "unk_02005D10.h"
#include "vram_transfer_manager.h"

#include "gunk_dexnav_art.h"

// The Certificates app never runs at the same time, so its heap is free to borrow.
#define HEAP_ID_DEXNAV HEAP_ID_CERTIFICATES

// BG palette slots: 0 = art, 1 = art with the selected cell's colours, 2 = text
#define PAL_ART         0
#define PAL_SELECTED    1
#define PAL_TEXT        2
#define TEXT_DARK       MAKE_TEXT_COLOR(1, 2, 0)
#define TEXT_ON_BAR     MAKE_TEXT_COLOR(3, 4, 0)
#define TEXT_RED        MAKE_TEXT_COLOR(5, 6, 0)
#define GREY_PALETTE    8 // sub OBJ palettes 8-10: faded copies of the three icon palettes, for seen Pokémon

// shared with tools/gunk/dexnav_art.py
#define CELL_W          40
#define CELL_H          32
#define GRID_LEFT       8
#define LAND_TOP        16
#define WATER_TOP       104
#define FOOTER_TOP      176
#define CLOSE_LEFT      192
#define CLOSE_RIGHT     251

#define RES_ICON_CHAR   0 // + slot
#define RES_ICON_PLTT   0
#define RES_ICON_CELL   0
#define RES_ICON_ANIM   0

#define NO_SLOT         -1
#define TOUCH_CLOSE     -2

typedef struct GunkDexNavApp {
    enum HeapID heapID;
    GunkDexNavArgs *args;
    Pokedex *pokedex;
    BgConfig *bgConfig;
    Window topWindow;
    Window bottomWindow;
    SpriteSystem *spriteSystem;
    SpriteManager *spriteManager;
    ManagedSprite *icons[DEXNAV_TOTAL_SLOTS];
    int cursor;
} GunkDexNavApp;

static BOOL GunkDexNavApp_Init(OverlayManager *man, int *state);
static BOOL GunkDexNavApp_Main(OverlayManager *man, int *state);
static BOOL GunkDexNavApp_Exit(OverlayManager *man, int *state);
static void GunkDexNavApp_VBlank(void *data);
static void GunkDexNavApp_InitBgs(GunkDexNavApp *app);
static void GunkDexNavApp_FreeBgs(GunkDexNavApp *app);
static void GunkDexNavApp_InitSprites(GunkDexNavApp *app);
static void GunkDexNavApp_FreeSprites(GunkDexNavApp *app);
static void GunkDexNavApp_DrawGrid(GunkDexNavApp *app);
static void GunkDexNavApp_DrawDetails(GunkDexNavApp *app);
static BOOL GunkDexNavApp_HandleInput(GunkDexNavApp *app);

const OverlayManagerTemplate gOverlayTemplate_GunkDexNav = {
    .init = GunkDexNavApp_Init,
    .exec = GunkDexNavApp_Main,
    .exit = GunkDexNavApp_Exit,
    .ovy_id = FS_OVERLAY_ID_NONE,
};

// The script command's wait (ScrCmd_GunkDexNavMenu): once the screen has closed, the result var (ctx->data[0]) gets
// the land Pokémon chosen, or 0xFFFE.
BOOL GunkDexNavApp_ScriptWait(ScriptContext *ctx) {
    FieldSystem *fieldSystem = ctx->fieldSystem;
    GunkDexNavArgs **args = FieldSysGetAttrAddr(fieldSystem, SCRIPTENV_RUNNING_APP_DATA);

    if (FieldSystem_ApplicationIsRunning(fieldSystem)) {
        return FALSE;
    }
    *GetVarPointer(fieldSystem, ctx->data[0]) = (*args)->chosen != SPECIES_NONE ? (*args)->chosen : 0xFFFE;
    Heap_Free(*args);
    *args = NULL;
    return TRUE;
}

static const char *const sMethodNames[] = {
    [DEXNAV_METHOD_GRASS] = "Tall grass",
    [DEXNAV_METHOD_SWARM] = "Tall grass - swarm",
    [DEXNAV_METHOD_HOENN_SOUND] = "Grass - Hoenn Sound",
    [DEXNAV_METHOD_SINNOH_SOUND] = "Grass - Sinnoh Sound",
    [DEXNAV_METHOD_SURF] = "Surfing",
    [DEXNAV_METHOD_SURF_SWARM] = "Surfing - swarm",
    [DEXNAV_METHOD_OLD_ROD] = "Old Rod",
    [DEXNAV_METHOD_GOOD_ROD] = "Good Rod",
    [DEXNAV_METHOD_SUPER_ROD] = "Super Rod",
};

static void AppendAscii(String *dst, const char *src) {
    for (; *src != '\0'; src++) {
        char c = *src;
        if (c >= 'A' && c <= 'Z') {
            String_AddChar(dst, CHAR_A + (c - 'A'));
        } else if (c >= 'a' && c <= 'z') {
            String_AddChar(dst, CHAR_a + (c - 'a'));
        } else if (c >= '0' && c <= '9') {
            String_AddChar(dst, CHAR_0 + (c - '0'));
        } else if (c == '/') {
            String_AddChar(dst, CHAR_SLASH);
        } else if (c == ':') {
            String_AddChar(dst, CHAR_COLON);
        } else if (c == '-') {
            String_AddChar(dst, CHAR_HYPHEN);
        } else if (c == '?') {
            String_AddChar(dst, CHAR_QMARK);
        } else if (c == '.') {
            String_AddChar(dst, CHAR_PERIOD);
        } else if (c == '\'') {
            String_AddChar(dst, CHAR_RAPOST);
        } else {
            String_AddChar(dst, CHAR_SPACE);
        }
    }
}

static void AppendNumber(GunkDexNavApp *app, String *dst, int num) {
    String *numStr = String_New(8, app->heapID);
    String16_FormatInteger(numStr, num, 3, PRINTING_MODE_LEFT_ALIGN, TRUE);
    String_Cat(dst, numStr);
    String_Delete(numStr);
}

static void PrintString(Window *window, String *string, int x, int y, u32 color) {
    AddTextPrinterParameterizedWithColor(window, FONT_SYSTEM, string, x, y, TEXT_SPEED_INSTANT, color, NULL);
}

static void PrintAscii(GunkDexNavApp *app, Window *window, const char *text, int x, int y, u32 color) {
    String *string = String_New(64, app->heapID);
    AppendAscii(string, text);
    PrintString(window, string, x, y, color);
    String_Delete(string);
}

static void SlotPosition(int slot, int *x, int *y) {
    int section = slot / DEXNAV_SECTION_SLOTS;
    int index = slot % DEXNAV_SECTION_SLOTS;

    *x = GRID_LEFT + (index % DEXNAV_GRID_COLS) * CELL_W;
    *y = (section == 0 ? LAND_TOP : WATER_TOP) + (index / DEXNAV_GRID_COLS) * CELL_H;
}

static u16 SlotSpecies(GunkDexNavApp *app, int slot) {
    return app->args->slots[slot].species;
}

static BOOL SlotIsSeen(GunkDexNavApp *app, int slot) {
    u16 species = SlotSpecies(app, slot);
    return species != SPECIES_NONE && Pokedex_CheckMonSeenFlag(app->pokedex, species);
}

static BOOL MethodIsSurf(u8 method) {
    return method == DEXNAV_METHOD_SURF || method == DEXNAV_METHOD_SURF_SWARM;
}

// Pokémon the player has seen in the grass or by surfing can be searched for (as in Gunk's Platinum); one found
// only by fishing can't.
static BOOL SlotCanSearch(GunkDexNavApp *app, int slot) {
    if (!SlotIsSeen(app, slot)) {
        return FALSE;
    }
    if (slot < DEXNAV_SECTION_SLOTS) {
        return TRUE;
    }
    return MethodIsSurf(app->args->slots[slot].method);
}

// Where R finds the target here: tall grass, surfing, both, or neither (it lives elsewhere).
static const char *SearchHint(GunkDexNavApp *app, u16 species) {
    BOOL grass = FALSE, surf = FALSE;
    for (int i = 0; i < DEXNAV_TOTAL_SLOTS; i++) {
        if (SlotSpecies(app, i) == species) {
            grass |= i < DEXNAV_SECTION_SLOTS;
            surf |= i >= DEXNAV_SECTION_SLOTS && MethodIsSurf(app->args->slots[i].method);
        }
    }
    if (grass && surf) {
        return "Press R in grass or while Surfing.";
    } else if (surf) {
        return "Press R while Surfing to search.";
    } else if (grass) {
        return "Press R in tall grass to search.";
    }
    return "It isn't found here.";
}

static BOOL GunkDexNavApp_Init(OverlayManager *man, int *state) {
    // 24 icons and two full-screen text windows: well under this
    Heap_Create(HEAP_ID_3, HEAP_ID_DEXNAV, 0x20000);

    GunkDexNavApp *app = OverlayManager_CreateAndGetData(man, sizeof(GunkDexNavApp), HEAP_ID_DEXNAV);
    MI_CpuClear8(app, sizeof(GunkDexNavApp));
    app->heapID = HEAP_ID_DEXNAV;
    app->args = OverlayManager_GetArgs(man);
    app->args->chosen = SPECIES_NONE;
    app->pokedex = Save_Pokedex_Get(app->args->saveData);

    // start on the search target if it lives here, else on the first Pokémon
    u16 target = Save_GunkSave_Get(app->args->saveData)->dexNavSpecies;
    app->cursor = NO_SLOT;
    for (int i = 0; i < DEXNAV_TOTAL_SLOTS; i++) {
        u16 species = SlotSpecies(app, i);
        if (species != SPECIES_NONE && (app->cursor == NO_SLOT || (species == target && i < DEXNAV_SECTION_SLOTS))) {
            app->cursor = i;
        }
    }
    if (app->cursor == NO_SLOT) {
        app->cursor = 0;
    }

    sub_0200FBF4(PM_LCD_TOP, RGB_BLACK);
    sub_0200FBF4(PM_LCD_BOTTOM, RGB_BLACK);
    Main_SetVBlankIntrCB(NULL, NULL);
    Main_SetHBlankIntrCB(NULL, NULL);
    GfGfx_DisableEngineAPlanes();
    GfGfx_DisableEngineBPlanes();
    GX_SetVisiblePlane(GX_PLANEMASK_NONE);
    GXS_SetVisiblePlane(GX_PLANEMASK_NONE);
    SetKeyRepeatTimers(4, 8);

    GunkDexNavApp_InitBgs(app);
    GunkDexNavApp_InitSprites(app);

    Main_SetVBlankIntrCB(GunkDexNavApp_VBlank, app);
    GfGfx_BothDispOn();
    return TRUE;
}

static BOOL GunkDexNavApp_Main(OverlayManager *man, int *state) {
    GunkDexNavApp *app = OverlayManager_GetData(man);

    switch (*state) {
    case 0:
        GunkDexNavApp_DrawGrid(app);
        GunkDexNavApp_DrawDetails(app);
        ToggleBgLayer(GF_BG_LYR_MAIN_0, GF_PLANE_TOGGLE_ON);
        ToggleBgLayer(GF_BG_LYR_MAIN_1, GF_PLANE_TOGGLE_ON);
        ToggleBgLayer(GF_BG_LYR_SUB_0, GF_PLANE_TOGGLE_ON);
        ToggleBgLayer(GF_BG_LYR_SUB_1, GF_PLANE_TOGGLE_ON);
        BeginNormalPaletteFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, RGB_BLACK, 6, 1, app->heapID);
        *state = 1;
        break;
    case 1:
        if (IsPaletteFadeFinished() == TRUE) {
            *state = 2;
        }
        break;
    case 2:
        if (GunkDexNavApp_HandleInput(app) == TRUE) {
            BeginNormalPaletteFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_OUT, FADE_TYPE_BRIGHTNESS_OUT, RGB_BLACK, 6, 1, app->heapID);
            *state = 3;
        }
        break;
    case 3:
        if (IsPaletteFadeFinished() == TRUE) {
            return TRUE;
        }
        break;
    }

    for (int i = 0; i < DEXNAV_TOTAL_SLOTS; i++) {
        if (app->icons[i] != NULL) {
            ManagedSprite_TickFrame(app->icons[i]);
        }
    }
    SpriteSystem_DrawSprites(app->spriteManager);
    return FALSE;
}

static BOOL GunkDexNavApp_Exit(OverlayManager *man, int *state) {
    GunkDexNavApp *app = OverlayManager_GetData(man);
    enum HeapID heapID = app->heapID;

    Main_SetVBlankIntrCB(NULL, NULL);
    GunkDexNavApp_FreeSprites(app);
    GunkDexNavApp_FreeBgs(app);
    OverlayManager_FreeData(man);
    Heap_Destroy(heapID);
    return TRUE;
}

static void GunkDexNavApp_VBlank(void *data) {
    GunkDexNavApp *app = data;

    DoScheduledBgGpuUpdates(app->bgConfig);
    SpriteSystem_TransferOam();
    GF_RunVramTransferTasks();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

// The grid cell (or the Close button) under a new touch.
static int TouchedSlot(void) {
    if (!gSystem.touchNew) {
        return NO_SLOT;
    }
    int x = gSystem.touchX, y = gSystem.touchY;
    if (y >= FOOTER_TOP && x >= CLOSE_LEFT && x <= CLOSE_RIGHT) {
        return TOUCH_CLOSE;
    }
    for (int slot = 0; slot < DEXNAV_TOTAL_SLOTS; slot++) {
        int sx, sy;
        SlotPosition(slot, &sx, &sy);
        if (x >= sx + 2 && x < sx + CELL_W - 3 && y >= sy + 1 && y < sy + CELL_H - 3) {
            return slot;
        }
    }
    return NO_SLOT;
}

static BOOL GunkDexNavApp_Choose(GunkDexNavApp *app) {
    if (!SlotCanSearch(app, app->cursor)) {
        PlaySE(SEQ_SE_DP_SELECT); // unseen and fishing-only Pokémon are shown, not searched for
        return FALSE;
    }
    PlaySE(SEQ_SE_DP_DECIDE);
    app->args->chosen = SlotSpecies(app, app->cursor);
    return TRUE;
}

static BOOL GunkDexNavApp_HandleInput(GunkDexNavApp *app) {
    int section = app->cursor / DEXNAV_SECTION_SLOTS;
    int index = app->cursor % DEXNAV_SECTION_SLOTS;
    int col = index % DEXNAV_GRID_COLS;
    int row = section * DEXNAV_GRID_ROWS + index / DEXNAV_GRID_COLS;
    int oldCursor = app->cursor;
    int touched = TouchedSlot();

    // touch: the first tap highlights a Pokémon, a second tap on it chooses it
    if (touched == TOUCH_CLOSE) {
        PlaySE(SEQ_SE_DP_DECIDE);
        return TRUE;
    }
    if (touched != NO_SLOT) {
        if (touched == app->cursor) {
            return GunkDexNavApp_Choose(app);
        }
        app->cursor = touched;
    } else if (gSystem.newKeys & PAD_BUTTON_A) {
        return GunkDexNavApp_Choose(app);
    } else if (gSystem.newKeys & PAD_BUTTON_B) {
        PlaySE(SEQ_SE_DP_DECIDE);
        return TRUE;
    } else {
        if (gSystem.newAndRepeatedKeys & PAD_KEY_LEFT) {
            col = (col + DEXNAV_GRID_COLS - 1) % DEXNAV_GRID_COLS;
        } else if (gSystem.newAndRepeatedKeys & PAD_KEY_RIGHT) {
            col = (col + 1) % DEXNAV_GRID_COLS;
        } else if (gSystem.newAndRepeatedKeys & PAD_KEY_UP) {
            row = (row + DEXNAV_GRID_ROWS * 2 - 1) % (DEXNAV_GRID_ROWS * 2);
        } else if (gSystem.newAndRepeatedKeys & PAD_KEY_DOWN) {
            row = (row + 1) % (DEXNAV_GRID_ROWS * 2);
        }
        app->cursor = (row / DEXNAV_GRID_ROWS) * DEXNAV_SECTION_SLOTS + (row % DEXNAV_GRID_ROWS) * DEXNAV_GRID_COLS + col;
    }

    if (app->cursor != oldCursor) {
        PlaySE(SEQ_SE_DP_SELECT5);
        GunkDexNavApp_DrawGrid(app);
        GunkDexNavApp_DrawDetails(app);
    }
    return FALSE;
}

static void GunkDexNavApp_DrawGrid(GunkDexNavApp *app) {
    Window *window = &app->bottomWindow;
    u16 target = Save_GunkSave_Get(app->args->saveData)->dexNavSpecies;
    int x, y;

    // the cursor is a palette swap on the art layer: same tiles, warm fill and a red outline
    LoadRectToBgTilemapRect(app->bgConfig, GF_BG_LYR_SUB_1, sDexNavBottomMap, 0, 0, 32, 24);
    SlotPosition(app->cursor, &x, &y);
    BgTilemapRectChangePalette(app->bgConfig, GF_BG_LYR_SUB_1, x / 8, y / 8, CELL_W / 8, CELL_H / 8, PAL_SELECTED);
    ScheduleBgTilemapBufferTransfer(app->bgConfig, GF_BG_LYR_SUB_1);

    FillWindowPixelBuffer(window, 0);
    PrintAscii(app, window, "LAND", 12, LAND_TOP - 13, TEXT_DARK);
    PrintAscii(app, window, "WATER", 12, WATER_TOP - 13, TEXT_DARK);
    PrintAscii(app, window, "A: Search  B: Close", 8, FOOTER_TOP + 1, TEXT_ON_BAR);
    PrintAscii(app, window, "CLOSE", CLOSE_LEFT + 16, FOOTER_TOP + 1, TEXT_RED);

    for (int slot = 0; slot < DEXNAV_TOTAL_SLOTS; slot++) {
        u16 species = SlotSpecies(app, slot);
        if (species == SPECIES_NONE) {
            continue;
        }
        SlotPosition(slot, &x, &y);
        if (!SlotIsSeen(app, slot)) {
            PrintAscii(app, window, "?", x + 17, y + 7, TEXT_DARK);
        }
        // a mark on caught Pokémon in the top left corner, a red star on the search target in the top right
        String *mark = String_New(4, app->heapID);
        if (Pokedex_CheckMonCaughtFlag(app->pokedex, species)) {
            String_AddChar(mark, CHAR_CIRCLE_DOT);
            PrintString(window, mark, x + 3, y + 1, TEXT_DARK);
        }
        if (species == target && slot < DEXNAV_SECTION_SLOTS) {
            String_SetEmpty(mark);
            String_AddChar(mark, CHAR_STAR);
            PrintString(window, mark, x + 27, y + 1, TEXT_RED);
        }
        String_Delete(mark);
    }
    ScheduleWindowCopyToVram(window);
}

static void GunkDexNavApp_DrawDetails(GunkDexNavApp *app) {
    Window *window = &app->topWindow;
    const GunkDexNavEntry *entry = &app->args->slots[app->cursor];
    GunkSave *gunkSave = Save_GunkSave_Get(app->args->saveData);
    u16 target = gunkSave->dexNavSpecies;
    int numCaught = 0, numSpecies = 0;

    for (int i = 0; i < DEXNAV_TOTAL_SLOTS; i++) {
        u16 species = SlotSpecies(app, i);
        if (species != SPECIES_NONE) {
            numSpecies++;
            numCaught += Pokedex_CheckMonCaughtFlag(app->pokedex, species) ? 1 : 0;
        }
    }

    FillWindowPixelBuffer(window, 0);
    PrintAscii(app, window, "DexNav", 12, 3, TEXT_ON_BAR);

    String *string = String_New(64, app->heapID);
    AppendAscii(string, "Caught ");
    AppendNumber(app, string, numCaught);
    AppendAscii(string, "/");
    AppendNumber(app, string, numSpecies);
    PrintString(window, string, 172, 3, TEXT_ON_BAR);

    // info card: the highlighted Pokémon
    if (numSpecies == 0) {
        PrintAscii(app, window, "No wild Pokemon live here.", 20, 38, TEXT_DARK);
    } else if (entry->species == SPECIES_NONE) {
        PrintAscii(app, window, "-----", 20, 38, TEXT_DARK);
    } else {
        BOOL seen = Pokedex_CheckMonSeenFlag(app->pokedex, entry->species);
        BOOL caught = Pokedex_CheckMonCaughtFlag(app->pokedex, entry->species);

        if (seen) {
            String *name = GetSpeciesName(entry->species, app->heapID);
            PrintString(window, name, 20, 38, TEXT_DARK);
            String_Delete(name);
        } else {
            PrintAscii(app, window, "?????", 20, 38, TEXT_DARK);
        }
        PrintAscii(app, window, caught ? "Caught" : seen ? "Seen" : "Not seen yet", 150, 38, TEXT_DARK);
        PrintAscii(app, window, "Found", 20, 66, TEXT_DARK);
        PrintAscii(app, window, sMethodNames[entry->method], 84, 66, TEXT_DARK);

        String_SetEmpty(string);
        AppendAscii(string, "Lv. ");
        AppendNumber(app, string, entry->minLevel);
        if (entry->maxLevel != entry->minLevel) {
            AppendAscii(string, "-");
            AppendNumber(app, string, entry->maxLevel);
        }
        PrintAscii(app, window, "Level", 20, 88, TEXT_DARK);
        PrintString(window, string, 84, 88, TEXT_DARK);

        String_SetEmpty(string);
        AppendAscii(string, "Search Lv. ");
        AppendNumber(app, string, gunkSave->dexNavSearchLevel[entry->species]);
        PrintString(window, string, 20, 108, TEXT_DARK);
    }

    // search card: what R looks for, and the chain on it here
    PrintAscii(app, window, "Search", 16, 136, TEXT_DARK);
    if (target != SPECIES_NONE) {
        String *name = GetSpeciesName(target, app->heapID);
        PrintString(window, name, 20, 152, TEXT_DARK);
        String_Delete(name);
        if (gGunkDexNav.chain != 0 && gGunkDexNav.chainSpecies == target) {
            String_SetEmpty(string);
            AppendAscii(string, "Chain ");
            AppendNumber(app, string, gGunkDexNav.chain);
            PrintString(window, string, 150, 152, TEXT_RED);
        }
        PrintAscii(app, window, SearchHint(app, target), 20, 168, TEXT_DARK);
    } else {
        PrintAscii(app, window, "Tap a Pokemon twice to search.", 20, 156, TEXT_DARK);
    }

    String_Delete(string);
    ScheduleWindowCopyToVram(window);
}

static void GunkDexNavApp_InitBgs(GunkDexNavApp *app) {
    GraphicsBanks banks = {
        .bg = GX_VRAM_BG_128_A,
        .bgextpltt = GX_VRAM_BGEXTPLTT_NONE,
        .subbg = GX_VRAM_SUB_BG_128_C,
        .subbgextpltt = GX_VRAM_SUB_BGEXTPLTT_NONE,
        .obj = GX_VRAM_OBJ_128_B,
        .objextpltt = GX_VRAM_OBJEXTPLTT_NONE,
        .subobj = GX_VRAM_SUB_OBJ_128_D,
        .subobjextpltt = GX_VRAM_SUB_OBJEXTPLTT_NONE,
        .tex = GX_VRAM_TEX_NONE,
        .texpltt = GX_VRAM_TEXPLTT_NONE,
    };
    GfGfx_SetBanks(&banks);

    app->bgConfig = BgConfig_Alloc(app->heapID);

    GraphicsModes modes = {
        .dispMode = GX_DISPMODE_GRAPHICS,
        .bgMode = GX_BGMODE_0,
        .subMode = GX_BGMODE_0,
        ._2d3dMode = GX_BG0_AS_2D,
    };
    SetBothScreensModesAndDisable(&modes);

    BgTemplate textTemplate = {
        .x = 0,
        .y = 0,
        .bufferSize = 0x800,
        .baseTile = 0,
        .size = GF_BG_SCR_SIZE_256x256,
        .colorMode = GX_BG_COLORMODE_16,
        .screenBase = GX_BG_SCRBASE_0x0000,
        .charBase = GX_BG_CHARBASE_0x04000,
        .bgExtPltt = GX_BG_EXTPLTT_01,
        .priority = 0,
        .areaOver = GX_BG_AREAOVER_XLU,
        .mosaic = FALSE,
    };
    // the art layer under the text: one tileset for both screens
    BgTemplate artTemplate = textTemplate;
    artTemplate.screenBase = GX_BG_SCRBASE_0x0800;
    artTemplate.charBase = GX_BG_CHARBASE_0x10000;
    artTemplate.priority = 1;

    InitBgFromTemplate(app->bgConfig, GF_BG_LYR_MAIN_1, &artTemplate, GF_BG_TYPE_TEXT);
    InitBgFromTemplate(app->bgConfig, GF_BG_LYR_SUB_1, &artTemplate, GF_BG_TYPE_TEXT);
    BG_LoadCharTilesData(app->bgConfig, GF_BG_LYR_MAIN_1, sDexNavTiles, sizeof(sDexNavTiles), 0);
    BG_LoadCharTilesData(app->bgConfig, GF_BG_LYR_SUB_1, sDexNavTiles, sizeof(sDexNavTiles), 0);
    LoadRectToBgTilemapRect(app->bgConfig, GF_BG_LYR_MAIN_1, sDexNavTopMap, 0, 0, 32, 24);
    BgCommitTilemapBufferToVram(app->bgConfig, GF_BG_LYR_MAIN_1);

    InitBgFromTemplate(app->bgConfig, GF_BG_LYR_MAIN_0, &textTemplate, GF_BG_TYPE_TEXT);
    BgClearTilemapBufferAndCommit(app->bgConfig, GF_BG_LYR_MAIN_0);
    InitBgFromTemplate(app->bgConfig, GF_BG_LYR_SUB_0, &textTemplate, GF_BG_TYPE_TEXT);
    BgClearTilemapBufferAndCommit(app->bgConfig, GF_BG_LYR_SUB_0);

    BG_LoadPlttData(GF_PAL_LOCATION_MAIN_BG, sDexNavPalBase, sizeof(sDexNavPalBase), PAL_ART * 32);
    BG_LoadPlttData(GF_PAL_LOCATION_MAIN_BG, sDexNavPalSelected, sizeof(sDexNavPalSelected), PAL_SELECTED * 32);
    BG_LoadPlttData(GF_PAL_LOCATION_MAIN_BG, sDexNavPalText, sizeof(sDexNavPalText), PAL_TEXT * 32);
    BG_LoadPlttData(GF_PAL_LOCATION_SUB_BG, sDexNavPalBase, sizeof(sDexNavPalBase), PAL_ART * 32);
    BG_LoadPlttData(GF_PAL_LOCATION_SUB_BG, sDexNavPalSelected, sizeof(sDexNavPalSelected), PAL_SELECTED * 32);
    BG_LoadPlttData(GF_PAL_LOCATION_SUB_BG, sDexNavPalText, sizeof(sDexNavPalText), PAL_TEXT * 32);

    WindowTemplate windowTemplate = {
        .bgId = GF_BG_LYR_MAIN_0,
        .left = 0,
        .top = 0,
        .width = 32,
        .height = 24,
        .palette = PAL_TEXT,
        .baseTile = 1,
    };
    ResetAllTextPrinters();
    AddWindow(app->bgConfig, &app->topWindow, &windowTemplate);
    windowTemplate.bgId = GF_BG_LYR_SUB_0;
    AddWindow(app->bgConfig, &app->bottomWindow, &windowTemplate);
}

static void GunkDexNavApp_FreeBgs(GunkDexNavApp *app) {
    RemoveWindow(&app->topWindow);
    RemoveWindow(&app->bottomWindow);
    ToggleBgLayer(GF_BG_LYR_MAIN_0, GF_PLANE_TOGGLE_OFF);
    ToggleBgLayer(GF_BG_LYR_MAIN_1, GF_PLANE_TOGGLE_OFF);
    ToggleBgLayer(GF_BG_LYR_SUB_0, GF_PLANE_TOGGLE_OFF);
    ToggleBgLayer(GF_BG_LYR_SUB_1, GF_PLANE_TOGGLE_OFF);
    FreeBgTilemapBuffer(app->bgConfig, GF_BG_LYR_MAIN_0);
    FreeBgTilemapBuffer(app->bgConfig, GF_BG_LYR_MAIN_1);
    FreeBgTilemapBuffer(app->bgConfig, GF_BG_LYR_SUB_0);
    FreeBgTilemapBuffer(app->bgConfig, GF_BG_LYR_SUB_1);
    Heap_Free(app->bgConfig);
}

static void GunkDexNavApp_InitSprites(GunkDexNavApp *app) {
    const OamManagerParam oamParam = { 0, 128, 0, 32, 0, 128, 0, 32 };
    const OamCharTransferParam transferParam = { DEXNAV_TOTAL_SLOTS + 4, 0x4000, 0x20000, GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K }; // the transfer area is the end of the bank: all of it, so the icons start within 1D 32K's reach
    const SpriteResourceCountsListUnion capacities = {
        .numChar = DEXNAV_TOTAL_SLOTS,
        .numPltt = 1,
        .numCell = 1,
        .numAnim = 1,
        .numMcel = 1,
        .numManm = 1,
    };

    GF_CreateVramTransferManager(DEXNAV_TOTAL_SLOTS + 8, app->heapID); // the icons' tiles go to VRAM through it
    app->spriteSystem = SpriteSystem_Alloc(app->heapID);
    SpriteSystem_Init(app->spriteSystem, &oamParam, &transferParam, 16);
    app->spriteManager = SpriteManager_New(app->spriteSystem);
    SpriteSystem_InitSprites(app->spriteSystem, app->spriteManager, DEXNAV_TOTAL_SLOTS);
    SpriteSystem_InitManagerWithCapacities(app->spriteSystem, app->spriteManager, &capacities);

    NARC *narc = NARC_New(NARC_poketool_icongra_poke_icon, app->heapID);
    SpriteSystem_LoadPlttResObjFromOpenNarc(app->spriteSystem, app->spriteManager, narc, sub_02074490(), FALSE, 3, NNS_G2D_VRAM_TYPE_2DSUB, RES_ICON_PLTT);
    SpriteSystem_LoadCellResObjFromOpenNarc(app->spriteSystem, app->spriteManager, narc, sub_02074494(), FALSE, RES_ICON_CELL); // the cells and animations for 1D 32K mapping
    SpriteSystem_LoadAnimResObjFromOpenNarc(app->spriteSystem, app->spriteManager, narc, sub_020744A0(), FALSE, RES_ICON_ANIM);

    // faded copies of the three icon palettes: seen-but-not-caught Pokémon are drawn with them
    NNSG2dPaletteData *paletteData;
    void *paletteFile = GfGfxLoader_GetPlttDataFromOpenNarc(narc, sub_02074490(), &paletteData, app->heapID);
    const u16 *colors = paletteData->pRawData;
    u16 *grey = Heap_Alloc(app->heapID, sizeof(u16) * 16 * 3); // heap, not stack: DMA can't read DTCM
    for (int i = 0; i < 16 * 3; i++) {
        int r = colors[i] & 0x1F, g = (colors[i] >> 5) & 0x1F, b = (colors[i] >> 10) & 0x1F;
        int luma = (r * 77 + g * 151 + b * 28) >> 8;
        int level = 12 + luma / 3;
        grey[i] = RGB(level, level, level);
    }
    DC_FlushRange(grey, sizeof(u16) * 16 * 3);
    GXS_LoadOBJPltt(grey, GREY_PALETTE * 32, sizeof(u16) * 16 * 3);
    Heap_Free(grey);
    Heap_Free(paletteFile);

    for (int slot = 0; slot < DEXNAV_TOTAL_SLOTS; slot++) {
        u16 species = SlotSpecies(app, slot);
        if (!SlotIsSeen(app, slot)) {
            continue;
        }
        // the icon files don't say 1D mapping: take the hardware's
        SpriteSystem_LoadCharResObjFromOpenNarcWithHardwareMappingType(app->spriteSystem, app->spriteManager, narc, GetMonIconNaixEx(species, FALSE, 0), FALSE, NNS_G2D_VRAM_TYPE_2DSUB, RES_ICON_CHAR + slot);

        int x, y;
        SlotPosition(slot, &x, &y);
        ManagedSpriteTemplate template = {
            .x = x + CELL_W / 2 - 1,
            .y = y + CELL_H / 2 - 2,
            .z = 0,
            .animation = 0,
            .drawPriority = 0,
            .pal = 0,
            .vram = NNS_G2D_VRAM_TYPE_2DSUB,
            .resIdList = { RES_ICON_CHAR + slot, RES_ICON_PLTT, RES_ICON_CELL, RES_ICON_ANIM, -1, -1 },
            .bgPriority = 0,
            .vramTransfer = FALSE,
        };
        app->icons[slot] = SpriteSystem_NewSprite(app->spriteSystem, app->spriteManager, &template);
        ManagedSprite_SetAnim(app->icons[slot], 1);
        u8 pal = GetMonIconPaletteEx(species, 0, FALSE);
        if (Pokedex_CheckMonCaughtFlag(app->pokedex, species)) {
            Sprite_SetPalOffsetRespectVramOffset(app->icons[slot]->sprite, pal);
        } else {
            Sprite_SetPalOffset(app->icons[slot]->sprite, GREY_PALETTE + pal);
        }
    }

    NARC_Delete(narc);
    GfGfx_EngineBTogglePlanes(GX_PLANEMASK_OBJ, GF_PLANE_TOGGLE_ON);
}

static void GunkDexNavApp_FreeSprites(GunkDexNavApp *app) {
    for (int slot = 0; slot < DEXNAV_TOTAL_SLOTS; slot++) {
        if (app->icons[slot] != NULL) {
            Sprite_DeleteAndFreeResources(app->icons[slot]);
        }
    }
    SpriteSystem_FreeResourcesAndManager(app->spriteSystem, app->spriteManager);
    SpriteSystem_Free(app->spriteSystem);
    GF_DestroyVramTransferManager();
}
