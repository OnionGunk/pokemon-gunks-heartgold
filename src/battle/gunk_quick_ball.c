#include "battle/gunk_quick_ball.h"

#include "global.h"

#include "constants/battle.h"
#include "constants/charcode.h"
#include "constants/items.h"
#include "constants/sndseq.h"

#include "battle/battle_system.h"

#include "bag.h"
#include "bg_window.h"
#include "font.h"
#include "gunk_save.h"
#include "heap.h"
#include "item.h"
#include "filesystem.h"
#include "palette.h"
#include "party.h"
#include "pm_string.h"
#include "pokemon_storage_system.h"
#include "sprite_system.h"
#include "system.h"
#include "text.h"
#include "unk_02005D10.h"

#define QUICK_BALL_RES_ID    20500
#define QUICK_BALL_COUNT_MAX 99

// a small label just above the message box (BG1, tiles y 19-22): "L ×99", and the ball's icon left of it
#define LABEL_BG_LAYER  GF_BG_LYR_MAIN_1
#define LABEL_LEFT      4
#define LABEL_TOP       17
#define LABEL_WIDTH     6
#define LABEL_HEIGHT    2
#define LABEL_PALETTE   11
#define LABEL_BASE_TILE (31 + 27 * 4) // after the message box window's tiles (base 31, 27 x 4)
#define ICON_X          18
#define ICON_Y          144

typedef struct GunkQuickBall {
    SaveData *saveData; // the battle has copies of the Bag and party; the last ball goes to the real save
    BOOL shown;
    BOOL lHeld;
    BOOL picked;
    u16 ball;
    u16 pending; // the ball the controller throws for the BAG press that L made
    Window *label;
    ManagedSprite *icon;
} GunkQuickBall;

static GunkQuickBall sQuickBall;

// every ball a plain wild battle can take, in the order Left / Right steps through them (no Safari or Park Ball)
static const u16 sBalls[] = {
    ITEM_MASTER_BALL, ITEM_ULTRA_BALL, ITEM_GREAT_BALL, ITEM_POKE_BALL, ITEM_NET_BALL, ITEM_DIVE_BALL,
    ITEM_NEST_BALL, ITEM_REPEAT_BALL, ITEM_TIMER_BALL, ITEM_LUXURY_BALL, ITEM_PREMIER_BALL, ITEM_DUSK_BALL,
    ITEM_HEAL_BALL, ITEM_QUICK_BALL, ITEM_CHERISH_BALL, ITEM_FAST_BALL, ITEM_LEVEL_BALL, ITEM_LURE_BALL,
    ITEM_HEAVY_BALL, ITEM_LOVE_BALL, ITEM_FRIEND_BALL, ITEM_MOON_BALL, ITEM_SPORT_BALL,
};

static int BallIndex(u16 item) {
    for (int i = 0; i < (int)NELEMS(sBalls); i++) {
        if (sBalls[i] == item) {
            return i;
        }
    }
    return -1;
}

static BOOL IsPlainWildBattle(BattleSystem *battleSystem) {
    u32 type = BattleSystem_GetBattleType(battleSystem);

    if (type != BATTLE_TYPE_NONE && type != BATTLE_TYPE_ROAMER) {
        return FALSE; // trainers, doubles, Safari, Pal Park, Bug-Catching Contest, link, Frontier, catching tutorial
    }
    return !(BattleSystem_GetBattleSpecial(battleSystem) & (BATTLE_SPECIAL_RECORDING | BATTLE_SPECIAL_RECORDED));
}

static u16 BallCount(BattleSystem *battleSystem, u16 ball) {
    return Bag_GetQuantity(BattleSystem_GetBag(battleSystem), ball, HEAP_ID_BATTLE);
}

// the next ball the player has after `from` (wrapping), stepping by `step`, or ITEM_NONE
static u16 NextBall(BattleSystem *battleSystem, u16 from, int step) {
    int n = NELEMS(sBalls);
    int i = BallIndex(from);

    if (i < 0) {
        i = step > 0 ? -1 : 0;
    }
    for (int k = 0; k < n; k++) {
        i = (i + step + n) % n;
        if (BallCount(battleSystem, sBalls[i]) > 0) {
            return sBalls[i];
        }
    }
    return ITEM_NONE;
}

// the last ball thrown (kept in the save), else a Poké Ball, else the first ball the player has
static u16 DefaultBall(BattleSystem *battleSystem) {
    u16 last = sQuickBall.saveData != NULL ? Save_GunkSave_Get(sQuickBall.saveData)->quickBall : ITEM_NONE;

    if (BallIndex(last) >= 0 && BallCount(battleSystem, last) > 0) {
        return last;
    }
    if (BallCount(battleSystem, ITEM_POKE_BALL) > 0) {
        return ITEM_POKE_BALL;
    }
    return NextBall(battleSystem, ITEM_NONE, 1);
}

static void DrawLabel(BattleSystem *battleSystem) {
    u16 count = BallCount(battleSystem, sQuickBall.ball);
    String *string = String_New(8, HEAP_ID_BATTLE);

    if (count > QUICK_BALL_COUNT_MAX) {
        count = QUICK_BALL_COUNT_MAX;
    }
    String_AddChar(string, CHAR_L);
    String_AddChar(string, CHAR_SPACE);
    String_AddChar(string, CHAR_MULT);
    if (count >= 10) {
        String_AddChar(string, CHAR_0 + count / 10);
    }
    String_AddChar(string, CHAR_0 + count % 10);

    FillWindowPixelBuffer(sQuickBall.label, 15);
    AddTextPrinterParameterizedWithColor(sQuickBall.label, FONT_SYSTEM, string, 2, 0, TEXT_SPEED_NOTRANSFER, MAKE_TEXT_COLOR(1, 2, 15), NULL);
    CopyWindowToVram(sQuickBall.label);
    String_Delete(string);
}

static void LoadIcon(BattleSystem *battleSystem) {
    SpriteSystem *spriteSystem = BattleSystem_GetSpriteSystem(battleSystem);
    SpriteManager *spriteManager = BattleSystem_GetSpriteManager(battleSystem);
    PaletteData *plttData = BattleSystem_GetPaletteData(battleSystem);
    NARC *narc = NARC_New(NARC_itemtool_itemdata_item_icon, HEAP_ID_BATTLE);
    ManagedSpriteTemplate template;

    // with the battle's own OBJ mapping: the icon file's mapping type would switch the whole top screen's (found
    // in Gunk's Platinum: the healthboxes came out scrambled)
    SpriteSystem_LoadCharResObjFromOpenNarcWithHardwareMappingType(spriteSystem, spriteManager, narc, GetItemIndexMapping(sQuickBall.ball, ITEMNARC_NCGR), FALSE, NNS_G2D_VRAM_TYPE_2DMAIN, QUICK_BALL_RES_ID);
    u8 slot = SpriteSystem_LoadPaletteBufferFromOpenNarc(plttData, PLTTBUF_MAIN_OBJ, spriteSystem, spriteManager, narc, GetItemIndexMapping(sQuickBall.ball, ITEMNARC_NCLR), FALSE, 1, NNS_G2D_VRAM_TYPE_2DMAIN, QUICK_BALL_RES_ID);
    // the battle uploads its palette buffer every frame: this ball's colors go in it too, or a slot freed by the
    // last ball comes back with that ball's colors
    PaletteData_LoadNarc(plttData, NARC_itemtool_itemdata_item_icon, GetItemIndexMapping(sQuickBall.ball, ITEMNARC_NCLR), HEAP_ID_BATTLE, PLTTBUF_MAIN_OBJ, 0x20, slot * 16);
    SpriteSystem_LoadCellResObjFromOpenNarc(spriteSystem, spriteManager, narc, GetItemIconCell(), FALSE, QUICK_BALL_RES_ID);
    SpriteSystem_LoadAnimResObjFromOpenNarc(spriteSystem, spriteManager, narc, GetItemIconAnim(), FALSE, QUICK_BALL_RES_ID);
    NARC_Delete(narc);

    template.x = ICON_X;
    template.y = ICON_Y;
    template.z = 0;
    template.animation = 0;
    template.drawPriority = 0;
    template.pal = 0;
    template.vram = NNS_G2D_VRAM_TYPE_2DMAIN;
    for (int i = 0; i < GF_GFX_RES_TYPE_MAX; i++) {
        template.resIdList[i] = -1;
    }
    template.resIdList[GF_GFX_RES_TYPE_CHAR] = QUICK_BALL_RES_ID;
    template.resIdList[GF_GFX_RES_TYPE_PLTT] = QUICK_BALL_RES_ID;
    template.resIdList[GF_GFX_RES_TYPE_CELL] = QUICK_BALL_RES_ID;
    template.resIdList[GF_GFX_RES_TYPE_ANIM] = QUICK_BALL_RES_ID;
    template.bgPriority = 0;
    template.vramTransfer = FALSE;

    sQuickBall.icon = SpriteSystem_NewSprite(spriteSystem, spriteManager, &template);
    ManagedSprite_SetDrawFlag(sQuickBall.icon, TRUE);
}

static void FreeIcon(BattleSystem *battleSystem) {
    if (sQuickBall.icon != NULL) {
        SpriteManager *spriteManager = BattleSystem_GetSpriteManager(battleSystem);

        // the sprite, then its resources: loading a resource ID still in use keeps the old ball's graphics
        Sprite_DeleteAndFreeResources(sQuickBall.icon);
        SpriteManager_UnloadCharObjById(spriteManager, QUICK_BALL_RES_ID);
        SpriteManager_UnloadPlttObjById(spriteManager, QUICK_BALL_RES_ID);
        SpriteManager_UnloadCellObjById(spriteManager, QUICK_BALL_RES_ID);
        SpriteManager_UnloadAnimObjById(spriteManager, QUICK_BALL_RES_ID);
        sQuickBall.icon = NULL;
    }
}

// at the start of each battle (battle_setup.c): the shown ball starts from the save again
void GunkQuickBall_Reset(SaveData *saveData) {
    MI_CpuClear8(&sQuickBall, sizeof(sQuickBall));
    sQuickBall.saveData = saveData;
}

// a ball was thrown, by the Bag or by L: show it next time, and keep it in the save
void GunkQuickBall_SetLastThrown(u16 ball) {
    if (BallIndex(ball) >= 0) {
        sQuickBall.ball = ball;
        if (sQuickBall.saveData != NULL) {
            Save_GunkSave_Get(sQuickBall.saveData)->quickBall = ball;
        }
    }
}

void GunkQuickBall_Show(BattleSystem *battleSystem) {
    GunkQuickBall_Hide(battleSystem);
    if (!IsPlainWildBattle(battleSystem)) {
        return;
    }
    if (BallIndex(sQuickBall.ball) < 0 || BallCount(battleSystem, sQuickBall.ball) == 0) {
        sQuickBall.ball = DefaultBall(battleSystem);
    }
    if (sQuickBall.ball == ITEM_NONE) {
        return;
    }

    sQuickBall.label = AllocWindows(HEAP_ID_BATTLE, 1);
    AddWindowParameterized(BattleSystem_GetBgConfig(battleSystem), sQuickBall.label, LABEL_BG_LAYER, LABEL_LEFT, LABEL_TOP, LABEL_WIDTH, LABEL_HEIGHT, LABEL_PALETTE, LABEL_BASE_TILE);
    DrawLabel(battleSystem);
    LoadIcon(battleSystem);
    sQuickBall.shown = TRUE;
    sQuickBall.lHeld = FALSE;
    sQuickBall.picked = FALSE;
}

void GunkQuickBall_Hide(BattleSystem *battleSystem) {
    if (!sQuickBall.shown) {
        return;
    }
    FreeIcon(battleSystem);
    ClearWindowTilemapAndCopyToVram(sQuickBall.label);
    RemoveWindow(sQuickBall.label);
    Heap_Free(sQuickBall.label);
    sQuickBall.label = NULL;
    sQuickBall.shown = FALSE;
}

static void PickBall(BattleSystem *battleSystem, int step) {
    u16 ball = NextBall(battleSystem, sQuickBall.ball, step);

    if (ball == ITEM_NONE || ball == sQuickBall.ball) {
        return;
    }
    sQuickBall.ball = ball;
    FreeIcon(battleSystem);
    LoadIcon(battleSystem);
    DrawLabel(battleSystem);
    PlaySE(SEQ_SE_DP_SELECT);
}

// the checks the Bag makes before it lets a ball be thrown: the foe out of sight (Fly, Dig...) or behind a
// Substitute, no room in the party or the boxes
static BOOL CanThrow(BattleSystem *battleSystem) {
    BattleContext *ctx = BattleSystem_GetBattleContext(battleSystem);

    if (ctx->battleMons[BATTLER_ENEMY].moveEffectFlags & MOVE_EFFECT_FLAG_SEMI_INVULNERABLE) {
        return FALSE;
    }
    if (ctx->battleMons[BATTLER_ENEMY].status2 & STATUS2_SUBSTITUTE) {
        return FALSE;
    }
    return !(Party_GetCount(BattleSystem_GetParty(battleSystem, BATTLER_PLAYER)) == PARTY_SIZE
             && PCStorage_FindFirstBoxWithEmptySlot(BattleSystem_GetPcStorage(battleSystem)) == NUM_BOXES);
}

// each frame the action menu reads input (BattleInput_CheckTouch)
enum GunkQuickBallInput GunkQuickBall_HandleInput(BattleSystem *battleSystem) {
    // R is RUN in every battle with this menu: the RUN path says when running isn't allowed
    if ((gSystem.newKeys & PAD_BUTTON_R) && !sQuickBall.lHeld) {
        return GUNK_QUICK_BALL_INPUT_RUN;
    }
    if (!sQuickBall.shown) {
        return GUNK_QUICK_BALL_INPUT_NONE;
    }
    if (gSystem.heldKeys & PAD_BUTTON_L) {
        if (!sQuickBall.lHeld) {
            sQuickBall.lHeld = TRUE;
            sQuickBall.picked = FALSE;
        }
        if (gSystem.newKeys & (PAD_KEY_LEFT | PAD_KEY_RIGHT)) {
            PickBall(battleSystem, (gSystem.newKeys & PAD_KEY_LEFT) ? -1 : 1);
            sQuickBall.picked = TRUE;
        }
        return GUNK_QUICK_BALL_INPUT_HELD;
    }
    if (sQuickBall.lHeld) {
        // L let go: a tap throws, a hold that picked a ball doesn't
        sQuickBall.lHeld = FALSE;
        if (sQuickBall.picked) {
            return GUNK_QUICK_BALL_INPUT_HELD;
        }
        if (!CanThrow(battleSystem)) {
            return GUNK_QUICK_BALL_INPUT_BAG;
        }
        sQuickBall.pending = sQuickBall.ball;
        return GUNK_QUICK_BALL_INPUT_THROW;
    }
    return GUNK_QUICK_BALL_INPUT_NONE;
}

// the controller's BAG case: the ball L asked for (once), or ITEM_NONE for a real Bag press
u16 GunkQuickBall_TakePending(void) {
    u16 ball = sQuickBall.pending;
    sQuickBall.pending = ITEM_NONE;
    return ball;
}
