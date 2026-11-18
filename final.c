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

void start_final_battle(const Player* player)
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

    /* Army combat, duel and endings: doc/final-assault-research.md#next-targets. */
    puts("Army combat and the final duel are not implemented yet.");
    exit(EXIT_SUCCESS);
}
