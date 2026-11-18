#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "braminar.h"

extern int random_number(int limit);
extern int male_servant_price;
extern int female_servant_price;

static int final_roll(int limit)
{
    return limit > 0 ? random_number(limit) + 1 : 0;
}

FinalArmy convert_final_assets(const Player* player)
{
    /* FINASSLT.CHN 0x29BE-0x2C01; doc/final-assault-research.md. */
    double worth = player->gold;
    worth += player->male_servants * male_servant_price;
    worth += player->female_servants * female_servant_price;
    worth += player->medicine * 3;
    worth += player->poison * 100;
    worth += player->swords * final_roll(100);
    worth += player->bows * final_roll(100);
    worth += player->arrows * final_roll(20);
    worth += player->daggers * final_roll(100);
    if (player->horse) {
        worth += 100;
    }
    if (player->cart) {
        worth += 200;
    }
    if (player->level > 20) {
        worth += (player->level - 20) * 1000;
    }
    FinalArmy army;
    army.net_worth = worth;
    army.soldiers = (int)(worth / 7 + 0.5);
    army.enemy_soldiers = final_roll(army.soldiers) + army.soldiers / 2;
    return army;
}

static FinalArmy prepare_final_army(const Player* player)
{
    puts("\nNow attacking Overlord's castle!");
    puts("Now converting assets into an army.");
    const struct timespec conversion_delay = {1, 0};
    nanosleep(&conversion_delay, NULL);
    FinalArmy army = convert_final_assets(player);
    printf("Your net worth is %7.0f gold pieces.\n", army.net_worth);
    printf("You have %d soldiers.\n", army.soldiers);
    printf("The Overlord has %d soldiers.\n", army.enemy_soldiers);
    const struct timespec army_delay = {2, 0};
    nanosleep(&army_delay, NULL);

    return army;
}

static void final_delay(int milliseconds)
{
    struct timespec delay = {milliseconds / 1000, (milliseconds % 1000) * 1000000L};
    fflush(stdout);
    nanosleep(&delay, NULL);
}

static void final_army_event(FinalArmy* army)
{
    static const char* messages[] = {
        "Your soldiers are low on moral.",
        "Your soldiers are high on moral!",
        "You capture an emeny magic user!",
        "One of your Magic users is Captured.",
        "One of your assassins was successful!",
        "One of your captains was assassinated.",
        "A surprize attack executes successfully!",
        "The enemy surprised your troops.",
        "Your army utilized a magical weapon!",
        "The enemy uses a magical weapon against you."
    };
    final_delay(100);
    int side = final_roll(2);
    int event = final_roll(10);
    printf("\033[2;3H%s\033[3;3H%s", side == 1 ? "Player" : "Overlord", messages[event - 1]);
    int change;
    switch (event) {
    case 1: change = -final_roll(10); break;
    case 2: change = final_roll(10); break;
    case 3: change = final_roll(army->enemy_soldiers / 4); break;
    case 4: change = -final_roll(army->soldiers / 4); break;
    case 5: change = final_roll(army->enemy_soldiers / 2); break;
    case 6: change = -final_roll(army->soldiers / 2); break;
    case 7:
    case 9: change = final_roll(army->soldiers); break;
    default: change = -final_roll(army->enemy_soldiers); break;
    }
    /* Both labels use these same limits and messages; see army research. */
    if (side == 1) {
        if (change > 0) {
            army->enemy_soldiers -= change;
        } else {
            army->soldiers += change;
        }
    } else {
        if (change > 0) {
            army->soldiers -= change;
        } else {
            army->enemy_soldiers += change;
        }
    }
    final_delay(3000);
    printf("\033[3;3H%54s\033[2;3H%54s", "", "");
}

static void final_army_counts(const FinalArmy* army)
{
    printf("\033[11;25H%d   \033[11;55H%d   ", army->soldiers, army->enemy_soldiers);
}

static void final_army_bar(int row, int current, int initial)
{
    if (initial == 0) {
        fputs("Division by zero.\n", stderr);
        exit(EXIT_FAILURE);
    }
    int filled = (int)(75.0 * current / initial + 0.5);
    printf("\033[%d;1H", row);
    for (int i = 0; i < 75; ++i) {
        fputs(i < filled ? "▓" : "░", stdout);
    }
}

static void final_army_percentages(const FinalArmy* army, const FinalArmy* initial)
{
    final_army_bar(20, army->soldiers, initial->soldiers);
    final_army_bar(21, army->enemy_soldiers, initial->enemy_soldiers);
}

static void final_army_casualties(FinalArmy* army)
{
    for (int i = 0; i < 5; ++i) {
        int roll = final_roll(army->soldiers + army->enemy_soldiers);
        if (roll < army->soldiers) {
            --army->enemy_soldiers;
        } else {
            --army->soldiers;
        }
    }
}

static void final_army_round(FinalArmy* army, const FinalArmy* initial)
{
    final_army_counts(army);
    final_army_casualties(army);
    final_army_percentages(army, initial);
    if (final_roll(100) < 5) {
        final_army_event(army);
    }
    if (initial->soldiers < 10000) {
        final_delay(100 - initial->soldiers / 100);
    }
}

static bool fight_final_armies(const Player* player, FinalArmy* army)
{
    /* FINASSLT.CHN 0x025F-0x09DA; doc/army-research.md. */
    FinalArmy initial = *army;
    printf("\033[2J\033[H\033[10;20H%s's Army\033[10;50HOverlord's Army"
           "\033[19;1HPlayer's Percentage\033[22;1HOverlord's Percentage", player->name);
    do {
        final_army_round(army, &initial);
    } while (army->soldiers > 0 && army->enemy_soldiers > 0);
    if (army->soldiers < 0) {
        army->soldiers = 0;
    }
    if (army->enemy_soldiers < 0) {
        army->enemy_soldiers = 0;
    }
    final_army_counts(army);
    final_army_percentages(army, &initial);
    final_delay(100);
    final_delay(3000);
    printf("\033[2J\033[H");
    return army->soldiers > 0;
}

void start_final_battle(const Player* player)
{
    FinalArmy army = prepare_final_army(player);
    if (!fight_final_armies(player, &army)) {
        puts("Your army perished.  You are captured by the Overlord.");
    } else {
        /* Overlord duel and endings: doc/final-assault-research.md#next-targets. */
        puts("The final duel is not implemented yet.");
    }
    exit(EXIT_SUCCESS);
}
