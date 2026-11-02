#include "constants/items.h"

#include "bag.h"
#include "field_system.h"
#include "item.h"
#include "roamer.h"
#include "scrcmd.h"

extern BOOL ItemIsTMOrHM(u16 item_id);

BOOL ScrCmd_GiveItem(ScriptContext *ctx) {
    FieldSystem *sav_ptr = ctx->fieldSystem;
    u16 item_id = ScriptGetVar(ctx);
    u16 quantity = ScriptGetVar(ctx);
    u16 *ret_ptr = ScriptGetVarPointer(ctx);

    Bag *bag = Save_Bag_Get(sav_ptr->saveData);
    *ret_ptr = Bag_AddItem(bag, item_id, quantity, HEAP_ID_FIELD1);

    return FALSE;
}

BOOL ScrCmd_TakeItem(ScriptContext *ctx) {
    FieldSystem *sav_ptr = ctx->fieldSystem;
    u16 item_id = ScriptGetVar(ctx);
    u16 quantity = ScriptGetVar(ctx);
    u16 *ret_ptr = ScriptGetVarPointer(ctx);

    Bag *bag = Save_Bag_Get(sav_ptr->saveData);
    *ret_ptr = Bag_TakeItem(bag, item_id, quantity, HEAP_ID_FIELD1);

    return FALSE;
}

BOOL ScrCmd_HasSpaceForItem(ScriptContext *ctx) {
    FieldSystem *sav_ptr = ctx->fieldSystem;
    u16 item_id = ScriptGetVar(ctx);
    u16 quantity = ScriptGetVar(ctx);
    u16 *ret_ptr = ScriptGetVarPointer(ctx);

    Bag *bag = Save_Bag_Get(sav_ptr->saveData);
    *ret_ptr = Bag_HasSpaceForItem(bag, item_id, quantity, HEAP_ID_FIELD1);

    return FALSE;
}

BOOL ScrCmd_HasItem(ScriptContext *ctx) {
    FieldSystem *sav_ptr = ctx->fieldSystem;
    u16 item_id = ScriptGetVar(ctx);
    u16 quantity = ScriptGetVar(ctx);
    u16 *ret_ptr = ScriptGetVarPointer(ctx);

    Bag *bag = Save_Bag_Get(sav_ptr->saveData);
    *ret_ptr = Bag_HasItem(bag, item_id, quantity, HEAP_ID_FIELD2);

    return FALSE;
}

BOOL ScrCmd_GetItemQuantity(ScriptContext *ctx) {
    FieldSystem *sav_ptr = ctx->fieldSystem;
    u16 item_id = ScriptGetVar(ctx);
    u16 *ret_ptr = ScriptGetVarPointer(ctx);

    Bag *bag = Save_Bag_Get(sav_ptr->saveData);
    *ret_ptr = Bag_GetQuantity(bag, item_id, HEAP_ID_FIELD2);

    return FALSE;
}

BOOL ScrCmd_ItemIsTMOrHM(ScriptContext *ctx) {
    u16 item_id = ScriptGetVar(ctx);
    u16 *ret_ptr = ScriptGetVarPointer(ctx);

    *ret_ptr = ItemIsTMOrHM(item_id);

    return FALSE;
}

BOOL ScrCmd_GetItemPocket(ScriptContext *ctx) {
    u16 item_id = ScriptGetVar(ctx);
    u16 *ret_ptr = ScriptGetVarPointer(ctx);

    *ret_ptr = GetItemAttr(item_id, ITEMATTR_FIELD_POCKET, HEAP_ID_FIELD2);

    return FALSE;
}

// Gunk: uses one Repel of the given kind from the bag (the prompt when a Repel wears off)
BOOL ScrCmd_GunkUseRepel(ScriptContext *ctx) {
    u16 item = ScriptGetVar(ctx);
    SaveData *saveData = ctx->fieldSystem->saveData;

    if (Bag_TakeItem(Save_Bag_Get(saveData), item, 1, HEAP_ID_FIELD2)) {
        *RoamerSave_GetRepelAddr(Save_Roamers_Get(saveData)) = GetItemAttr(item, ITEMATTR_HOLD_EFFECT_PARAM, HEAP_ID_FIELD2);
    }
    return FALSE;
}
