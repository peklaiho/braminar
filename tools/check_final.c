#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

static int final_nanosleep(const struct timespec* request, struct timespec* remaining);
#define nanosleep final_nanosleep
#define main braminar_program_main
#include "../braminar.c"
#include "../combat.c"
#include "../events.c"
#include "../town.c"
#include "../final.c"
#undef main
#undef nanosleep

static double delay_seconds;

static int final_nanosleep(const struct timespec* request, struct timespec* remaining)
{
    (void)remaining;
    delay_seconds += request->tv_sec + request->tv_nsec / 1000000000.0;
    return 0;
}

static void final_checkpoint(void)
{
    printf("\nDELAY %.3f\n", delay_seconds);
}

int main(int argc, char** argv)
{
    if (argc != 17) {
        return 2;
    }
    Player player = {0};
    random_state = (uint32_t)strtoul(argv[1], NULL, 10);
    player.gold = atoi(argv[2]);
    player.male_servants = atoi(argv[3]);
    player.female_servants = atoi(argv[4]);
    player.medicine = atoi(argv[5]);
    player.poison = atoi(argv[6]);
    player.swords = atoi(argv[7]);
    player.bows = atoi(argv[8]);
    player.arrows = atoi(argv[9]);
    player.daggers = atoi(argv[10]);
    player.horse = atoi(argv[11]);
    player.cart = atoi(argv[12]);
    player.level = atoi(argv[13]);
    male_servant_price = atoi(argv[14]);
    female_servant_price = atoi(argv[15]);
    if (atoi(argv[16])) {
        atexit(final_checkpoint);
        start_final_battle(&player);
    }
    Player before;
    memcpy(&before, &player, sizeof(player));
    FinalArmy army = convert_final_assets(&player);
    printf("RESULT {\"net_worth\":%.8f,\"army\":%d,\"enemy\":%d,\"seed\":%u,\"unchanged\":%d}\n",
           army.net_worth, army.soldiers, army.enemy_soldiers, (unsigned)random_state,
           memcmp(&before, &player, sizeof(player)) == 0);
    return 0;
}
