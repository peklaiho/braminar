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

typedef struct {
    double player_hp;
    double overlord_hp;
} FinalDuel;

static FinalDuel prepare_final_duel(const Player* player)
{
    FinalDuel duel;
    duel.player_hp = player->level * 10;
    duel.overlord_hp = final_roll((int)duel.player_hp) + duel.player_hp / 3;
    return duel;
}

static void final_duel_figure(int row, int col, const char* text)
{
    printf("\033[%d;%dH%s", row, col, text);
}

static void final_duel_animation(bool player, int kind)
{
    int home = player ? 39 : 41;
    const char* figure = player ? "☺" : "☻";
    if (kind == 1) {
        for (int i = 0; i < 3; ++i) {
            final_duel_figure(14, player ? 39 : 40, player ? " ☺" : "☻ ");
            final_delay(100);
            final_delay(100);
            final_duel_figure(14, player ? 39 : 40, player ? "☺ " : " ☻");
            final_delay(100);
        }
    } else if (kind == 2) {
        final_duel_figure(14, home, " ");
        for (int i = 0; i < 4; ++i) {
            int target = player ? 41 : 39;
            final_duel_figure(12, target, figure);
            final_delay(100);
            final_duel_figure(12, target, " ");
            final_duel_figure(13, target, figure);
            final_delay(100);
            final_duel_figure(13, target, " ");
            final_delay(100);
        }
        final_duel_figure(14, home, figure);
    } else {
        for (int i = 0; i < 5; ++i) {
            final_duel_figure(14, 40, "-");
            final_delay(100);
            final_duel_figure(14, 40, player ? "/" : "\\");
            final_delay(100);
        }
        final_duel_figure(14, 40, " ");
    }
}

static void draw_final_duel(const Player* player)
{
    printf("\033[2J\033[H\033[15;15H--------------------------------------------------");
    for (int col = 15; col <= 39; ++col) {
        final_duel_figure(14, col, "☺");
        final_delay(10);
        final_duel_figure(14, col, " ");
    }
    final_duel_figure(14, 39, "☺");
    final_duel_figure(16, 30, player->name);
    for (int col = 65; col >= 41; --col) {
        final_duel_figure(14, col, "☻");
        final_delay(10);
        final_duel_figure(14, col, " ");
    }
    final_duel_figure(14, 41, "☻");
    final_duel_figure(16, 50, "Overlord");
    final_delay(1000);
}

static void final_duel_attack(FinalDuel* duel, bool player)
{
    static const char* player_messages[] = {
        "You miss him.", "You slightly nicked him.", "You smack him in the face.",
        "You slug him in the gut.", "You beat him with a tree branch.", "You stomp on him.",
        "You slice into him with a dagger.", "Your sword cuts deep into the Overlord."
    };
    static const char* overlord_messages[] = {
        "He misses you", "He slightly nicks you.", "He hits you in the face.",
        "He kicks you in the shins.", "He tackles you.", "He stomps on you.",
        "He cuts your skin with a knife.", "He chops you up some."
    };
    static const double player_damage[] = {0, 0.3, 1.2, 2.4, 4.8, 7.5, 9.6, 12.7};
    static const double overlord_damage[] = {0, 0.3, 1.2, 2.2, 4.6, 6.8, 9.4, 11.2};
    int attack = final_roll(8) - 1;
    final_duel_figure(player ? 3 : 5, 10,
                      player ? player_messages[attack] : overlord_messages[attack]);
    if (player) {
        duel->overlord_hp -= player_damage[attack];
    } else {
        duel->player_hp -= overlord_damage[attack];
    }
    if (attack >= 2) {
        final_duel_animation(player, attack / 2);
    }
}

static void final_duel_round(FinalDuel* duel)
{
    final_duel_attack(duel, true);
    /* Retaliation still occurs after a killing blow; inventory is not consulted. */
    final_duel_attack(duel, false);
    printf("\033[17;30H%5.2f\033[17;50H%5.2f\033[20;1H", duel->player_hp, duel->overlord_hp);
    final_delay(2000);
    printf("\033[3;10H%50s\033[5;10H%50s", "", "");
}

static bool fight_overlord(const Player* player)
{
    /* FINASSLT.CHN 0x2059-0x2984; doc/duel-research.md. */
    FinalDuel duel = prepare_final_duel(player);
    draw_final_duel(player);
    if (player->talisman_of_braminar) {
        final_duel_figure(13, 40, "Φ");
        duel.overlord_hp = 0;
        final_duel_figure(3, 10, "You use the Talisman to defeat the Overlord!\n");
    } else {
        do {
            final_duel_round(&duel);
        } while (duel.player_hp > 0 && duel.overlord_hp > 0);
    }
    if (duel.overlord_hp <= 0) {
        final_duel_figure(14, 41, "⌂ r.i.p.");
    } else {
        final_duel_figure(14, 32, "r.i.p. ⌂");
    }
    printf("\033[23;1H");
    final_delay(1000);
    if (duel.player_hp <= 0) {
        puts("The Overlord has slain you in hand - hand combat.");
        return false;
    }
    return true;
}

void start_final_battle(const Player* player)
{
    FinalArmy army = prepare_final_army(player);
    if (!fight_final_armies(player, &army)) {
        puts("Your army perished.  You are captured by the Overlord.");
    } else if (fight_overlord(player)) {
        /* Victory rewards and endings: doc/final-assault-research.md#next-targets. */
        puts("Victory rewards and endings are not implemented yet.");
    }
    exit(EXIT_SUCCESS);
}
