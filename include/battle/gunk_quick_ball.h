#ifndef POKEHEARTGOLD_BATTLE_GUNK_QUICK_BALL_H
#define POKEHEARTGOLD_BATTLE_GUNK_QUICK_BALL_H

#include "battle/battle.h"
#include "save.h"

// Gunk: Quick Run and Quick Ball. At the action menu of a plain wild battle, R presses RUN and L throws the ball
// shown in a small widget at the top screen's bottom left (its icon, "L" and how many, up to 99); holding L and
// pressing Left or Right picks another ball. The throw goes through the Bag's path: the menu answers BAG and the
// controller takes the ball as if the Bag had returned it. Shared design with Gunk's Platinum (quick_ball.c).
enum GunkQuickBallInput {
    GUNK_QUICK_BALL_INPUT_NONE = 0, // nothing for the action menu: let it read its own input
    GUNK_QUICK_BALL_INPUT_HELD,     // L is down (or was used to pick a ball): the action menu ignores this frame
    GUNK_QUICK_BALL_INPUT_THROW,    // press BAG; the controller throws GunkQuickBall_TakePending()
    GUNK_QUICK_BALL_INPUT_BAG,      // the ball can't be thrown right now: open the Bag, which says why
    GUNK_QUICK_BALL_INPUT_RUN,      // R: press RUN
};

void GunkQuickBall_Reset(SaveData *saveData);
void GunkQuickBall_SetLastThrown(u16 ball);
void GunkQuickBall_Show(BattleSystem *battleSystem);
void GunkQuickBall_Hide(BattleSystem *battleSystem);
enum GunkQuickBallInput GunkQuickBall_HandleInput(BattleSystem *battleSystem);
u16 GunkQuickBall_TakePending(void);

#endif // POKEHEARTGOLD_BATTLE_GUNK_QUICK_BALL_H
