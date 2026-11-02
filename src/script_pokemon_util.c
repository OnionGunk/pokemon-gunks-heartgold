#include "script_pokemon_util.h"

#include "global.h"

#include "constants/battle.h"
#include "constants/items.h"

#include "bag.h"
#include "get_egg.h"
#include "item.h"
#include "party.h"
#include "pokemon_mood.h"
#include "unk_0206979C.h"
#include "update_dex_received.h"

static BOOL MonNotFaintedOrEgg(Pokemon *mon) {
    if (GetMonData(mon, MON_DATA_HP, NULL) == 0) {
        return FALSE;
    }
    return !GetMonData(mon, MON_DATA_IS_EGG, NULL);
}

BOOL GiveMon(enum HeapID heapID, SaveData *saveData, int species, int level, int form, u8 ability, u16 heldItem, int ball, int encounterType) {
    Party *party;
    Pokemon *mon;
    PlayerProfile *profile;
    u32 sp1C;
    BOOL result;

    profile = Save_PlayerData_GetProfile(saveData);
    party = SaveArray_Party_Get(saveData);
    {
        mon = AllocMonZeroed(heapID);
        ZeroMonData(mon);
        CreateMon(mon, species, level, 32, FALSE, 0, 0, 0);
        sub_020720FC(mon, profile, ITEM_POKE_BALL, ball, encounterType, heapID);
        sp1C = heldItem;
        SetMonData(mon, MON_DATA_HELD_ITEM, &sp1C);
        SetMonData(mon, MON_DATA_FORM, &form);
        if (ability != 0) {
            SetMonData(mon, MON_DATA_ABILITY, &ability);
        }
        result = Party_AddMon(party, mon);
        if (result) {
            UpdatePokedexWithReceivedSpecies(saveData, mon);
        }
        Heap_Free(mon);
    }
    return result;
}

BOOL GiveEgg(enum HeapID heapID, SaveData *saveData, int species, u8 metLocation, MapsecType mapsecType, int maploc) {
#pragma unused(heapID)
    PlayerProfile *profile;
    Party *party;
    Pokemon *mon;
    BOOL result;

    profile = Save_PlayerData_GetProfile(saveData);
    party = SaveArray_Party_Get(saveData);
    mon = AllocMonZeroed(HEAP_ID_FIELD3);
    ZeroMonData(mon);
    SetEggStats(mon, species, metLocation, profile, 4, sub_02017FE4(mapsecType, maploc));
    result = Party_AddMon(party, mon);
    Heap_Free(mon);
    return result;
}

void PartyMonSetMoveInSlot(Party *party, int mon_slot, int move_slot, u16 moveId) {
    MonSetMoveInSlot_ResetPpUp(Party_GetMonByIndex(party, mon_slot), moveId, move_slot);
}

int GetIdxOfFirstPartyMonWithMove(Party *party, u16 move) {
    int i;
    int n;
    Pokemon *mon;

    n = Party_GetCount(party);
    for (i = 0; i < n; i++) {
        mon = Party_GetMonByIndex(party, i);
        if (GetMonData(mon, MON_DATA_IS_EGG, NULL)) {
            continue;
        }
        if (GetMonData(mon, MON_DATA_MOVE1, NULL) == move
            || GetMonData(mon, MON_DATA_MOVE2, NULL) == move
            || GetMonData(mon, MON_DATA_MOVE3, NULL) == move
            || GetMonData(mon, MON_DATA_MOVE4, NULL) == move) {
            return i;
        }
    }
    return 0xFF;
}

// Gunk: field moves work for a Pokémon that could learn the move from an HM the player owns (badges are still
// checked as before).

// The HM that teaches `move` if the player owns it, else ITEM_NONE
u16 Bag_GetOwnedHMForMove(Bag *bag, u16 move, enum HeapID heapID) {
    for (u16 item = ITEM_HM01; item <= ITEM_HM08; item++) {
        if (TMHMGetMove(item) == move) {
            return Bag_HasItem(bag, item, 1, heapID) ? item : ITEM_NONE;
        }
    }
    return ITEM_NONE;
}

// A Pokémon can use a field move if it knows it, or if it could learn it from an HM the player owns
BOOL Mon_CanUseFieldMove(Pokemon *mon, Bag *bag, u16 move, enum HeapID heapID) {
    if (GetMonData(mon, MON_DATA_IS_EGG, NULL)) {
        return FALSE;
    }
    if (MonHasMove(mon, move)) {
        return TRUE;
    }
    u16 hm = Bag_GetOwnedHMForMove(bag, move, heapID);
    return hm != ITEM_NONE && GetMonTMHMCompat(mon, ItemToTMHMId(hm));
}

// The party slot to use a field move with: a Pokémon that knows it first, else one that could learn it from an HM
// the player owns; 0xFF if there is none
int Party_FindMonForFieldMove(Party *party, Bag *bag, u16 move, enum HeapID heapID) {
    int slot = GetIdxOfFirstPartyMonWithMove(party, move);
    if (slot != 0xFF) {
        return slot;
    }
    for (int i = 0; i < Party_GetCount(party); i++) {
        if (Mon_CanUseFieldMove(Party_GetMonByIndex(party, i), bag, move, heapID)) {
            return i;
        }
    }
    return 0xFF;
}

int CountAlivePokemon(Party *party) {
    int i;
    int n;
    int c;
    Pokemon *mon;

    n = Party_GetCount(party);
    c = 0;
    for (i = 0; i < n; i++) {
        mon = Party_GetMonByIndex(party, i);
        if (MonNotFaintedOrEgg(mon)) {
            c++;
        }
    }
    return c;
}

Pokemon *GetFirstAliveMonInParty_CrashIfNone(Party *party) {
    int i;
    int n;
    Pokemon *mon;

    n = Party_GetCount(party);
    for (i = 0; i < n; i++) {
        mon = Party_GetMonByIndex(party, i);
        if (MonNotFaintedOrEgg(mon)) {
            return mon;
        }
    }
    GF_ASSERT(FALSE);
    return NULL;
}

u8 GetIdxOfFirstAliveMonInParty_CrashIfNone(Party *party) {
    int i;
    int n;
    Pokemon *mon;

    n = Party_GetCount(party);
    for (i = 0; i < n; i++) {
        mon = Party_GetMonByIndex(party, i);
        if (MonNotFaintedOrEgg(mon)) {
            return i;
        }
    }
    GF_ASSERT(FALSE);
    return 0;
}

Pokemon *GetFirstNonEggInParty(Party *party) {
    u16 i;
    u16 n;
    Pokemon *mon;

    n = Party_GetCount(party);
    for (i = 0; i < n; i++) {
        mon = Party_GetMonByIndex(party, i);
        if (!GetMonData(mon, MON_DATA_IS_EGG, NULL)) {
            return mon;
        }
    }
    return NULL;
}

BOOL HasEnoughAlivePokemonForDoubleBattle(Party *party) {
    return CountAlivePokemon(party) >= 2;
}

BOOL ApplyPoisonStep(Party *party, u16 location) {
    int n;
    int i;
    int n_poisoned;
    int n_fainted;
    u32 hp;
    Pokemon *mon;

    n_poisoned = 0;
    n_fainted = 0;
    n = Party_GetCount(party);
    for (i = 0; i < n; i++) {
        mon = Party_GetMonByIndex(party, i);
        if (!MonNotFaintedOrEgg(mon)) {
            continue;
        }
        if (!(GetMonData(mon, MON_DATA_STATUS, NULL) & (STATUS_POISON | STATUS_BAD_POISON))) {
            continue;
        }
        hp = GetMonData(mon, MON_DATA_HP, NULL);
        if (hp > 1) {
            hp--;
        }
        SetMonData(mon, MON_DATA_HP, &hp);
        if (hp == 1) {
            n_fainted++;
            MonApplyFriendshipMod(mon, FRIENDSHIP_EVENT_HEAL_FIELD_PSN, location);
            ApplyMonMoodModifier(mon, MON_MOOD_MODIFIER_SURVIVED_PSN);
        }
        n_poisoned++;
    }
    if (n_fainted != 0) {
        return 2;
    } else if (n_poisoned != 0) {
        return 1;
    } else {
        return 0;
    }
}

BOOL SurvivePoisoning(Pokemon *mon) {
    u32 status;
    if ((GetMonData(mon, MON_DATA_STATUS, NULL) & (STATUS_POISON | STATUS_BAD_POISON)) && GetMonData(mon, MON_DATA_HP, NULL) == 1) {
        status = 0;
        SetMonData(mon, MON_DATA_STATUS, &status);
        return TRUE;
    } else {
        return FALSE;
    }
}
